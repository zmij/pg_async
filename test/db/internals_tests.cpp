/**
 * @file /tip-server/test/db/pg-tests.cpp
 * @brief
 * @date Jul 10, 2015
 * @author: zmij
 */

#include <tip/db/pg/resultset.hpp>
#include <tip/db/pg/resultset.inl>
#include <tip/db/pg/database.hpp>
#include <tip/db/pg/query.hpp>
#include <tip/db/pg/query.inl>
#include <tip/db/pg/error.hpp>

#include <tip/db/pg/detail/protocol.hpp>

#include <tip/db/pg/detail/basic_connection.hpp>
#include <tip/db/pg/detail/connection_pool.hpp>
#include <tip/db/pg/transaction.hpp>

#include <tip/db/pg/log.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <atomic>

#include <tip/db/pg/asio_config.hpp>

#ifndef WITH_BOOST_ASIO
#include <asio/deadline_timer.hpp>
#endif

#include "db/config.hpp"
#include "test-environment.hpp"

namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGTEST";
logger::event_severity DEFAULT_SEVERITY = logger::DEBUG;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
// For more convenient changing severity, eg local_log(logger::WARNING)
using tip::log::logger;
//using tip::db::pg::test::environment;


TEST( LiteralsTest, Alias )
{
	tip::db::pg::dbalias al = "main"_db;
	EXPECT_EQ(tip::db::pg::dbalias{ std::string("main") }, al);
}

TEST( LiteralsTest, ConnectionString )
{
	using namespace tip::db::pg;
	connection_options opts = "main=tcp://user:password@localhost:5432[db]"_pg;

	EXPECT_EQ("main", opts.alias);
	EXPECT_EQ("tcp", opts.schema);
	EXPECT_EQ("localhost:5432", opts.uri);
	EXPECT_EQ("db", opts.database);
	EXPECT_EQ("user", opts.user);
	EXPECT_EQ("password", opts.password);

	opts = "ssl://localhost[db]"_pg;
	EXPECT_EQ("", opts.alias);
	EXPECT_EQ("ssl", opts.schema);
	EXPECT_EQ("localhost", opts.uri);
	EXPECT_EQ("db", opts.database);
	EXPECT_EQ("", opts.user);
	EXPECT_EQ("", opts.password);

	opts = "ssl://localhost:5432[db]"_pg;
	EXPECT_EQ("", opts.alias);
	EXPECT_EQ("ssl", opts.schema);
	EXPECT_EQ("localhost:5432", opts.uri);
	EXPECT_EQ("db", opts.database);
	EXPECT_EQ("", opts.user);
	EXPECT_EQ("", opts.password);

	opts = "log=socket:///tmp/.s.PGSQL.5432[db]"_pg;
	EXPECT_EQ("log", opts.alias);
	EXPECT_EQ("socket", opts.schema);
	EXPECT_EQ("/tmp/.s.PGSQL.5432", opts.uri);
	EXPECT_EQ("db", opts.database);
	EXPECT_EQ("", opts.user);
	EXPECT_EQ("", opts.password);
}

TEST( ConnectionTest, Connect)
{
	using namespace tip::db::pg;
	if (! test::environment::test_database.empty() ) {
		connection_options opts = connection_options::parse(test::environment::test_database);

		asio_config::io_service io_service;
		connection_ptr conn_ptr;
		connection_ptr conn(basic_connection::create(
		io_service, opts, {
				{"client_encoding", "UTF8"},
				{"application_name", "pg_async"}
		},
		{
		[&](connection_ptr c) {
			conn_ptr = c;
			conn->terminate();
		}, [] (connection_ptr c) {
		}, [](connection_ptr c, error::connection_error const& ec) {
			FAIL();
		}}));

		io_service.run();
		EXPECT_TRUE(conn_ptr.get());
	}
}

TEST( ConnectionTest, ConnectionPool )
{
	typedef std::shared_ptr< std::thread > thread_ptr;
	typedef std::chrono::high_resolution_clock clock_type;
	using namespace tip::db::pg;
	if (!test::environment::test_database.empty()) {

		connection_options opts = connection_options::parse(test::environment::test_database);
		asio_config::io_service io_service;
		size_t pool_size = test::environment::connection_pool;
		int req_count = test::environment::num_requests;
		int thread_count = test::environment::num_threads;

		clock_type::time_point tp = clock_type::now();
		clock_type::duration start = tp.time_since_epoch();

		std::shared_ptr< tip::db::pg::detail::connection_pool > pool(
				tip::db::pg::detail::connection_pool::create(io_service, pool_size, opts));

		ASSERT_TRUE(pool.get());

		std::atomic<int> sent_count(0);
		std::atomic<int> res_count(0);
		std::atomic<int> fail_count(0);

		ASIO_NAMESPACE::deadline_timer timer(io_service,
				boost::posix_time::seconds(test::environment::deadline));
		timer.async_wait([&](asio_config::error_code const& ec){
			if (!ec) {
				local_log(logger::WARNING) << "Connection pool test timer expired";
				pool->close();
				timer.cancel();
				if (!io_service.stopped())
					io_service.stop();
			}
		});

		std::vector< thread_ptr > threads;
		for (int t = 0; t < thread_count; ++t) {
			threads.push_back(thread_ptr(new std::thread(
			[&]() {
				int t_no = t;
				for (int i = 0; i < req_count; ++i) {
					pool->get_connection(
					[&] (transaction_ptr tran) {
						int req_no = i;
						ASSERT_TRUE(tran.get());
						local_log(logger::DEBUG) << "Obtained connection thread  "
								<< t_no << " request " << req_no;
						++sent_count;
						tran->execute( "select * from pg_catalog.pg_class limit 10",
						[&] (transaction_ptr t1, resultset r, bool complete) {
							ASSERT_TRUE(t1.get());
							if (complete) {
								++res_count;
							}
							local_log(logger::DEBUG) << "Received a resultset columns: " << r.columns_size()
									<< " rows: " << r.size()
									<< " completed: " << std::boolalpha << complete;
							local_log(logger::DEBUG) << "Sent requests: " << sent_count
									<< " Received results: " << res_count;
							if (complete)
								t1->commit();

							if (res_count >= req_count * thread_count) {
								pool->close();
								timer.cancel();
							}
						}, [](error::db_error const&){} );
					},
					[&] (error::db_error const& ec) {
						++fail_count; // transaction rolled back
					});
				}
				io_service.run();
			}
			)));
		}

		for (auto t : threads) {
			t->join();
		}
		if (fail_count) {
			EXPECT_EQ(pool_size, fail_count); // Transaction will fail in each connection
		}
		EXPECT_EQ(req_count * thread_count, sent_count);
		EXPECT_EQ(req_count * thread_count, res_count);
		tp = clock_type::now();
		clock_type::duration run = tp.time_since_epoch() - start;
		double seconds = (double)run.count() * clock_type::period::num / clock_type::period::den;;
		local_log(logger::INFO) << "Running "
				<< req_count * thread_count << " requests in "
				<< thread_count << " threads with "
				<< pool_size << " connections took " << seconds << "s";
	}
}

TEST( ConnectionTest, ExecutePrepared )
{
	using namespace tip::db::pg;
	if (!test::environment::test_database.empty()) {
		connection_options opts = connection_options::parse(test::environment::test_database);

		asio_config::io_service io_service;
		int queries = 0;

		std::vector< oids::type::oid_type > param_types;
		std::vector<char> params;
		tip::db::pg::detail::write_params(param_types, params, 10, 20);

		connection_ptr conn(basic_connection::create(
			io_service, opts,
			{
				{"client_encoding", "UTF8"},
				{"application_name", "pg_async"},
				{"client_min_messages", "debug5"}
			},
		{
		[&](connection_ptr c) {
			if (queries == 0) {
				++queries;
				c->begin({
				[&](transaction_ptr tran){
					tran->execute(
					"select * from pg_catalog.pg_type where typelem > $1 limit $2",
					param_types, params,
					[&](transaction_ptr tran, resultset r, bool complete) {
						EXPECT_TRUE(r);
						EXPECT_TRUE(r.size());
						EXPECT_TRUE(r.columns_size());
						EXPECT_FALSE(r.empty());
						tran->commit();
					}, [&](error::db_error const& ) {
					});
				}, [](error::db_error const&) {
				}});
			} else {
				c->terminate();
			}
		}, [](connection_ptr c) {
		}, [](connection_ptr c, error::connection_error const& ec) {

		}}));
		io_service.run();
	}
}

TEST( TransactionTest, CleanExit )
{
	using namespace tip::db::pg;
	if (!test::environment::test_database.empty()) {
		connection_options opts = connection_options::parse(test::environment::test_database);

		local_log(logger::INFO) << "Transactions clean exit test";
		asio_config::io_service io_service;
		int transactions = 0;
		bool transaction_error = false;
		connection_ptr conn(basic_connection::create(
			io_service, opts,
			{
				{"client_encoding", "UTF8"},
				{"application_name", "pg_async"}
			},
		{[&](connection_ptr c) {
			if (transactions < test::environment::num_requests) {
				ASSERT_NO_THROW(c->begin({
				[&](transaction_ptr tran){
					//EXPECT_THROW( c->begin(events::begin()), db_error );
					EXPECT_TRUE(tran.get());
					EXPECT_TRUE(tran->in_transaction());
					ASSERT_NO_THROW(tran->commit());
				},
				[&](error::db_error const&){
					transaction_error = true;
				}}));
				transactions++;
			} else {
				c->terminate();
			}
		}, [] (connection_ptr c) {
		}, [](connection_ptr c, error::connection_error const& ec) {
			FAIL();
		}}));
		io_service.run();
		EXPECT_FALSE(transaction_error);
	}
}

TEST(TransactionTest, DirtyTerminate)
{
	using namespace tip::db::pg;
	if (!test::environment::test_database.empty()) {
		connection_options opts = connection_options::parse(test::environment::test_database);

		local_log(logger::INFO) << "Transactions dirty terminate exit test";
		asio_config::io_service io_service;
		int transaction_error = 0;
		int transactions = 0;
		connection_ptr conn(basic_connection::create(io_service,  opts,
		{
				{"client_encoding", "UTF8"},
				{"application_name", "pg_async"}
		},
		{[&](connection_ptr c) {
			if (transactions < test::environment::num_requests) {
				transactions++;
				local_log() << "Obtained connection";
				EXPECT_FALSE(c->in_transaction());
				ASSERT_NO_THROW(c->begin(
				{[&](transaction_ptr tran){
					EXPECT_TRUE(tran.get());
					EXPECT_TRUE(tran->in_transaction());
				},
				[&](error::db_error const&){
					transaction_error++;
				}}));
			} else {
				c->terminate();
			}
		}, [] (connection_ptr c) {
		}, [](connection_ptr c, error::connection_error const& ec) {
			FAIL();
		}}));
		io_service.run();
		EXPECT_EQ(test::environment::num_requests, transaction_error);
	}
}

TEST(TransactionTest, DirtyUnlock)
{
	using namespace tip::db::pg;
	if (!test::environment::test_database.empty()) {
		connection_options opts = connection_options::parse(test::environment::test_database);
		local_log(logger::INFO) << "Transactions dirty unlock exit test";
		asio_config::io_service io_service;

		ASIO_NAMESPACE::deadline_timer timer(io_service, boost::posix_time::milliseconds(100));

		int transaction_error = false;
		int transactions = 0;
		connection_ptr conn (basic_connection::create(io_service, opts,
		{
			{"client_encoding", "UTF8"},
			{"application_name", "pg_async"}
		},
		{[&](connection_ptr c) {
			if (!transactions) {
				ASSERT_NO_THROW(c->begin({
				[&](transaction_ptr tran){
					local_log() << "Transaction begin callback";
					EXPECT_TRUE(tran.get());
					EXPECT_TRUE(tran->in_transaction());
					timer.async_wait([&](asio_config::error_code const& ec){
						if (!ec) {
							local_log(logger::WARNING) << "Transaction dirty unlock test timer expired";
							conn->terminate();
						}
					});
				},
				[&](error::db_error const&){
					transaction_error = true;
				}}));
				transactions++;
			}
		}, [] (connection_ptr c) {
		}, [](connection_ptr c, error::connection_error const& ec) {
			FAIL();
		}}));
		io_service.run();
		EXPECT_TRUE(transaction_error);
	}
}



TEST(TransactionTest, Query)
{
	using namespace tip::db::pg;
	if (!test::environment::test_database.empty()) {
		connection_options opts = connection_options::parse(test::environment::test_database);
		local_log(logger::INFO) << "Transaction query test";
		asio_config::io_service io_service;
		int transaction_error = 0;
		int transactions = 0;
		int result_count = 0;
		connection_ptr conn(basic_connection::create(io_service, opts,
		{
			{"client_encoding", "UTF8"},
			{"application_name", "pg_async"}
		},
		{[&](connection_ptr c) {
			if (transactions < test::environment::num_requests) {
				transactions++;
				ASSERT_NO_THROW(c->begin({
				[&](transaction_ptr tran){
					EXPECT_TRUE(tran.get());
					EXPECT_TRUE(tran->in_transaction());
					tran->execute( "select * from pg_catalog.pg_class",
					[&] (transaction_ptr tran, resultset r, bool complete) {
						if (complete)
							result_count++;
						local_log() << "Received a resultset columns: " << r.columns_size()
								<< " rows: " << r.size()
								<< " completed: " << std::boolalpha << complete;
					}, [] (error::db_error const&) {});
				},
				[&](error::db_error const&){
					transaction_error++;
				}}));
			} else {
				c->terminate();
			}
		}, [] (connection_ptr c) {
		}, [](connection_ptr c, error::connection_error const& ec) {
			FAIL();
		}}));
		io_service.run();

		EXPECT_EQ(test::environment::num_requests, transaction_error);
		EXPECT_EQ(test::environment::num_requests, result_count);
	}
}

TEST(DatabaseTest, Service)
{
	typedef std::shared_ptr< std::thread > thread_ptr;
	typedef std::chrono::high_resolution_clock clock_type;
	using namespace tip::db::pg;
	ASSERT_THROW(
			db_service::begin("notthere"_db,
					transaction_callback(), error_callback()),
					std::runtime_error);

	if (!test::environment::test_database.empty()) {
		// not initialized yet
		ASSERT_NO_THROW(db_service::add_connection(test::environment::test_database));

		size_t pool_size = test::environment::connection_pool;

		ASIO_NAMESPACE::deadline_timer timer(db_service::io_service(),
				boost::posix_time::seconds(test::environment::deadline));
		timer.async_wait([&](asio_config::error_code const& ec){
			if (!ec) {
				#ifdef WITH_TIP_LOG
				local_log(logger::WARNING) << "Database service test timer expired";
				#endif
				db_service::stop();
			}
		});

		db_service::initialize(pool_size, {
			{"client_encoding", "UTF8"},
			{"application_name", "test-pg-async"}
		});
		connection_options opts = connection_options::parse(test::environment::test_database);
		db_service::begin(opts.alias,
		[&](transaction_ptr c){
            timer.cancel();
			db_service::stop();
		}, [](error::db_error const& ec) {

		});

		db_service::run();
	}
}

