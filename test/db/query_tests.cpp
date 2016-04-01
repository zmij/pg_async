/*
 * query_tests.cpp
 *
 *  Created on: Jul 27, 2015
 *      Author: zmij
 */

#include <tip/db/pg.hpp>

#include <tip/db/pg/log.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <fstream>

#include "db/config.hpp"
#include "test-environment.hpp"

LOCAL_LOGGING_FACILITY(PGTEST, TRACE);

using namespace tip::db::pg;

TEST(QueryTest, QueryInlay)
{
	using namespace tip::db::pg;
	if (!test::environment::test_database.empty()) {

		ASIO_NAMESPACE::deadline_timer timer(*db_service::io_service(),
				boost::posix_time::seconds(test::environment::deadline));
		timer.async_wait([&](asio_config::error_code const& ec){
			if (!ec) {
				#ifdef WITH_TIP_LOG
				local_log(logger::WARNING) << "Run query test timer expired";
				#endif
				db_service::stop();
			}
		});

		resultset res;

		ASSERT_NO_THROW(db_service::add_connection(test::environment::test_database));
		connection_options opts = connection_options::parse(test::environment::test_database);
		{
			query (opts.alias, "create temporary table pg_async_test(b bigint)").run_async(
			[&](transaction_ptr c, resultset, bool){
				local_log() << "Query one finished";
				EXPECT_TRUE(c.get());
				query(c, "insert into pg_async_test values(1),(2),(3)").run_async(
				[&](transaction_ptr c1, resultset, bool){
					local_log() << "Query two finished";
					EXPECT_TRUE(c1.get());
					query(c1, "select * from pg_async_test").run_async(
					[&](transaction_ptr c2, resultset r, bool) {
						local_log() << "Query three finished. Result columns "
								<< r.columns_size() << " rows " << r.size();
						EXPECT_TRUE(c2.get());
						res = r;
						query(c2, "drop table pg_async_test").run_async(
						[&](transaction_ptr c3, resultset, bool) {
							local_log() << "Query four finished";
							EXPECT_TRUE(c3.get());
                            timer.cancel();
                            //c->commit();
							db_service::stop();
						}, [](error::db_error const&){
							FAIL();
						});
					}, [](error::db_error const&){
						FAIL();
					});
				}, [](error::db_error const&){
					FAIL();
				});
			}, [](error::db_error const&){
				SUCCEED();
			});
		}
		db_service::run();

		local_log() << "Queries done";
		EXPECT_EQ(1, res.columns_size());
		EXPECT_EQ(3, res.size());
	}
}

TEST(QueryTest, QueryQueue)
{
	using namespace tip::db::pg;
	if (!test::environment::test_database.empty()) {

		ASIO_NAMESPACE::deadline_timer timer(*db_service::io_service(),
				boost::posix_time::seconds(test::environment::deadline));
		timer.async_wait([&](asio_config::error_code const& ec){
			if (!ec) {
				#ifdef WITH_TIP_LOG
				local_log(logger::WARNING) << "Run query test timer expired";
				#endif
				db_service::stop();
			}
		});

		resultset res;

		ASSERT_NO_THROW(db_service::add_connection(test::environment::test_database,
				test::environment::connection_pool));
		connection_options opts = connection_options::parse(test::environment::test_database);
		{
			db_service::begin(opts.alias,
			[&]( transaction_ptr tran ){
				query (tran, "create temporary table pg_async_test(b bigint)")(
				[&](transaction_ptr c, resultset, bool){
					local_log(logger::DEBUG) << "Query one finished";
					EXPECT_TRUE(c.get());
				}, [](error::db_error const&){
					SUCCEED();
				});
				for (int i = 0; i < test::environment::num_requests; ++i) {
					query(tran, "insert into pg_async_test values($1)", i)
					([&](transaction_ptr c, resultset, bool){
						local_log(logger::DEBUG) << "Query two finished";
						EXPECT_TRUE(c.get());
					}, [](error::db_error const&){
						FAIL();
					});
				}
				query(tran, "select * from pg_async_test")
				([&](transaction_ptr c, resultset r, bool) {
					local_log(logger::DEBUG) << "Query three finished";
					EXPECT_TRUE(c.get());
					EXPECT_EQ(test::environment::num_requests, r.size());
					EXPECT_EQ(1, r.columns_size());
					res = r;
				}, [](error::db_error const&){
					FAIL();
				});
				query(tran, "drop table pg_async_test")
				([&](transaction_ptr c, resultset, bool) {
					local_log(logger::DEBUG) << "Query four finished";
					EXPECT_TRUE(c.get());
	                timer.cancel();
	                //c->commit();
					db_service::stop();
				}, [](error::db_error const&){
					FAIL();
				});
				tran->commit();
			}, [](error::db_error const&){});
		}
		db_service::run();

		EXPECT_EQ(1, res.columns_size());
		EXPECT_EQ(test::environment::num_requests, res.size());
	}
}

TEST(QueryTest, BasicResultParsing)
{
	using namespace tip::db::pg;
	if (!test::environment::test_database.empty() && !test::SCRIPT_SOURCE_DIR.empty()) {

		//resultset res;

		ASSERT_NO_THROW(db_service::add_connection(test::environment::test_database));
		std::string script_name = test::SCRIPT_SOURCE_DIR + "results-parse-test.sql";
		std::ifstream script(script_name);
		local_log() << "Script file name " << script_name;
		if (script) {
			ASIO_NAMESPACE::deadline_timer timer(*db_service::io_service(),
					boost::posix_time::seconds(test::environment::deadline));
			timer.async_wait([&](asio_config::error_code const& ec){
                timer.cancel();
				if (!ec) {
					local_log(logger::WARNING) << "Parse result set test timer expired";
					db_service::stop();
				}
			});

			std::string script_str, line;
			while (getline(script, line)) {
				script_str += line;
			}
			connection_options opts = connection_options::parse(test::environment::test_database);
			query(opts.alias, script_str).run_async(
			[&](transaction_ptr c, resultset res, bool) {
				{
					local_log() << "Received a resultset. Columns: " << res.columns_size()
							<< " rows: " << res.size() << " empty: " << res.empty();
				}
				EXPECT_TRUE(c.get());
				if (!res.empty()) {
					EXPECT_TRUE(res);
					EXPECT_FALSE(res.empty());
					EXPECT_TRUE(res.size());
					EXPECT_TRUE(res.columns_size());

					for (int i = 0; i < res.columns_size(); ++i) {
						field_description const& fd = res.field(i);
						local_log() << "Field " << fd.name << " type "
								<< fd.type_oid << " type mod " << fd.type_mod;
					}

					for (resultset::const_iterator row = res.begin(); row != res.end(); ++row) {
						tip::log::local local = local_log();
						local << "Row " << (row - res.begin()) << ": ";

						long id;
						std::string ctime, ctimetz;
						row.to(std::tie(id, ctime, ctimetz));

						local << "id: " << id << " ctime: " << ctime << " ";

						for (resultset::const_field_iterator f = row.begin(); f != row.end(); ++f) {
							local << f.as< std::string >() << " ";
						}
					}
				}
			}, [&](error::db_error const&) {});

			db_service::run();
		}
	}
}
