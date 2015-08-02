/*
 * query_tests.cpp
 *
 *  Created on: Jul 27, 2015
 *      Author: zmij
 */


#include <tip/db/pg.hpp>
#include <tip/db/pg/transaction.hpp>

#include <tip/db/pg/log.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <fstream>

#include "db/config.hpp"
#include "test-environment.hpp"

namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGTEST";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
// For more convenient changing severity, eg local_log(logger::WARNING)
using tip::log::logger;
using namespace tip::db::pg;

TEST(QueryTest, QueryInlay)
{
	using namespace tip::db::pg;
	if (!test::environment::test_database.empty()) {

		boost::asio::deadline_timer timer(db_service::io_service(),
				boost::posix_time::seconds(test::environment::deadline));
		timer.async_wait([&](boost::system::error_code const& ec){
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
				[&](transaction_ptr c, resultset, bool){
					local_log() << "Query two finished";
					EXPECT_TRUE(c.get());
					query(c, "select * from pg_async_test").run_async(
					[&](transaction_ptr c, resultset r, bool) {
						local_log() << "Query three finished";
						EXPECT_TRUE(c.get());
						res = r;
						query(c, "drop table pg_async_test").run_async(
						[&](transaction_ptr c, resultset r, bool) {
							local_log() << "Query four finished";
							EXPECT_TRUE(c.get());
                            timer.cancel();
                            //c->commit();
							db_service::stop();
						}, [](db_error const&){
							FAIL();
						});
					}, [](db_error const&){
						FAIL();
					});
				}, [](db_error const&){
					FAIL();
				});
			}, [](db_error const&){
				SUCCEED();
			});
		}
		db_service::run();

		EXPECT_EQ(1, res.columns_size());
		EXPECT_EQ(3, res.size());
	}
}

TEST(QueryTest, QueryQueue)
{
	using namespace tip::db::pg;
	if (!test::environment::test_database.empty()) {

		boost::asio::deadline_timer timer(db_service::io_service(),
				boost::posix_time::seconds(test::environment::deadline));
		timer.async_wait([&](boost::system::error_code const& ec){
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
				}, [](db_error const&){
					SUCCEED();
				});
				for (int i = 0; i < test::environment::num_requests; ++i) {
					query(tran, "insert into pg_async_test values($1)", i)
					([&](transaction_ptr c, resultset, bool){
						local_log(logger::DEBUG) << "Query two finished";
						EXPECT_TRUE(c.get());
					}, [](db_error const&){
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
				}, [](db_error const&){
					FAIL();
				});
				query(tran, "drop table pg_async_test")
				([&](transaction_ptr c, resultset r, bool) {
					local_log(logger::DEBUG) << "Query four finished";
					EXPECT_TRUE(c.get());
	                timer.cancel();
	                //c->commit();
					db_service::stop();
				}, [](db_error const&){
					FAIL();
				});
			}, [](db_error const&){});
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

		resultset res;

		ASSERT_NO_THROW(db_service::add_connection(test::environment::test_database));
		std::string script_name = test::SCRIPT_SOURCE_DIR + "results-parse-test.sql";
		std::ifstream script(script_name);
		local_log() << "Script file name " << script_name;
		if (script) {
			boost::asio::deadline_timer timer(db_service::io_service(),
					boost::posix_time::seconds(test::environment::deadline));
			timer.async_wait([&](boost::system::error_code const& ec){
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
			[&](transaction_ptr c, resultset res, bool complete) {
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
					for (auto row: res) {
						for (auto f : row) {
							;
						}
					}
				}
			}, [&](db_error const&) {});

			db_service::run();
		}
	}
}
