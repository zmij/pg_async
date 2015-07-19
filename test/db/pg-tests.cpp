/**
 * @file /tip-server/test/db/pg-tests.cpp
 * @brief
 * @date Jul 10, 2015
 * @author: zmij
 */


//#define BOOST_TEST_MODULE PostgreSQLTest

#include <tip/db/pg/connection.hpp>
#include <tip/db/pg/resultset.hpp>
#include <tip/db/pg/resultset.inl>
#include <tip/db/pg/database.hpp>
#include <tip/db/pg/query.hpp>
#include <tip/db/pg/error.hpp>

#include <tip/db/pg/detail/protocol.hpp>

#include <tip/db/pg/detail/basic_connection.hpp>
#include <tip/db/pg/detail/connection_pool.hpp>
#include <tip/db/pg/detail/connection_lock.hpp>

#include <tip/db/pg/log.hpp>

#include <boost/test/unit_test.hpp>

#include <boost/program_options.hpp>

#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>

#include "db/config.hpp"

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

namespace {

std::string test_database	= "";
int num_requests			= 10;
int num_threads				= 4;
int connection_pool			= 4;
int deadline				= 5;
#ifdef WITH_TIP_LOG
logger::event_severity log_level = logger::DEBUG;
#endif

}  // namespace

// Initialize the test suite
boost::unit_test::test_suite*
init_unit_test_suite( int argc, char* argv[] )
{
	namespace po = boost::program_options;
	#ifdef WITH_TIP_LOG
	logger::set_proc_name(argv[0]);
	logger::set_stream(std::cerr);
	#endif
	po::options_description desc("Test options");

	desc.add_options()
			("database,b", po::value<std::string>(&test_database), "database connection string")
			("pool-size,s", po::value<int>(&connection_pool)->default_value(4), "connection pool size")
			("threads,x", po::value<int>(&num_threads)->default_value(10), "requests threads number")
			("requests,q", po::value<int>(&num_requests)->default_value(10), "number of requests per thread")
			#ifdef WITH_TIP_LOG
			("tip-log-level,v", po::value<logger::event_severity>(&log_level)->default_value(logger::INFO),
					"log level (TRACE, DEBUG, INFO, WARNING, ERROR)")
			#endif
			("run-deadline", po::value<int>(&deadline)->default_value(5),
					"Maximum time to execute requests")
			("log-colors", "output colored log")
			("help,h", "show options description")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << "\n";
	}

	if (num_requests < 1) num_requests = 1;
	if (num_threads < 1) num_threads = 1;
	if (connection_pool < 1) connection_pool = 1;

	#ifdef WITH_TIP_LOG
	logger::min_severity(log_level);
	logger::use_colors(vm.count("log-colors"));
	#endif

	return nullptr;
}

BOOST_AUTO_TEST_CASE( TestAliasLiteral )
{
	tip::db::pg::dbalias al = "main"_db;
	BOOST_CHECK_EQUAL(al, tip::db::pg::dbalias{ std::string("main") });
}

BOOST_AUTO_TEST_CASE( TestConnectionParsing )
{
	using namespace tip::db::pg;
	connection_options opts = "main=tcp://user:password@localhost:5432[db]"_pg;

	BOOST_CHECK_EQUAL(opts.alias, "main");
	BOOST_CHECK_EQUAL(opts.schema, "tcp");
	BOOST_CHECK_EQUAL(opts.uri, "localhost:5432");
	BOOST_CHECK_EQUAL(opts.database, "db");
	BOOST_CHECK_EQUAL(opts.user, "user");
	BOOST_CHECK_EQUAL(opts.password, "password");

	opts = "ssl://localhost[db]"_pg;
	BOOST_CHECK_EQUAL(opts.alias, "");
	BOOST_CHECK_EQUAL(opts.schema, "ssl");
	BOOST_CHECK_EQUAL(opts.uri, "localhost");
	BOOST_CHECK_EQUAL(opts.database, "db");
	BOOST_CHECK_EQUAL(opts.user, "");
	BOOST_CHECK_EQUAL(opts.password, "");

	opts = "ssl://localhost:5432[db]"_pg;
	BOOST_CHECK_EQUAL(opts.alias, "");
	BOOST_CHECK_EQUAL(opts.schema, "ssl");
	BOOST_CHECK_EQUAL(opts.uri, "localhost:5432");
	BOOST_CHECK_EQUAL(opts.database, "db");
	BOOST_CHECK_EQUAL(opts.user, "");
	BOOST_CHECK_EQUAL(opts.password, "");

	opts = "log=socket:///tmp/.s.PGSQL.5432[db]"_pg;
	BOOST_CHECK_EQUAL(opts.alias, "log");
	BOOST_CHECK_EQUAL(opts.schema, "socket");
	BOOST_CHECK_EQUAL(opts.uri, "/tmp/.s.PGSQL.5432");
	BOOST_CHECK_EQUAL(opts.database, "db");
	BOOST_CHECK_EQUAL(opts.user, "");
	BOOST_CHECK_EQUAL(opts.password, "");
}

BOOST_AUTO_TEST_CASE( ConnectDatabaseTest )
{
	if (!test_database.empty()) {
		using namespace tip::db::pg;
		connection_options opts = connection_options::parse(test_database);

		boost::asio::io_service io_service;
		connection_ptr conn_ptr;
		connection_ptr conn(connection::create(io_service,
		[&](connection_ptr c) {
			conn_ptr = c;
			conn->terminate();
		}, [] (connection_ptr c) {
		}, [](connection_ptr c, connection_error const& ec) {
			BOOST_FAIL(ec.what());
		},  opts, {
			{"client_encoding", "UTF8"},
			{"application_name", "pg_async"}
		}));

		io_service.run();
		BOOST_CHECK_MESSAGE(conn_ptr, "Connected");
	}
}

BOOST_AUTO_TEST_CASE(TransactionCleanExitTest)
{
	if (!test_database.empty()) {
		using namespace tip::db::pg;
		connection_options opts = connection_options::parse(test_database);

		#ifdef WITH_TIP_LOG
		local_log(logger::INFO) << "Transactions clean exit test";
		#endif
		boost::asio::io_service io_service;
		int transactions = 0;
		connection_ptr conn(connection::create(io_service,
		[&](connection_ptr c) {
			BOOST_CHECK_THROW(
					c->commit_transaction( connection_lock_ptr(), connection_lock_callback(), error_callback() ),
					db_error
			);
			{
				bool error_callback_fired = false;
				BOOST_CHECK_NO_THROW(c->commit_transaction( connection_lock_ptr(), connection_lock_callback(),
				[&]( db_error const& err ) {
					error_callback_fired = true;
				}));
				BOOST_CHECK(error_callback_fired);
			}
			BOOST_CHECK_THROW(
					c->rollback_transaction( connection_lock_ptr(), connection_lock_callback(), error_callback() ),
					db_error
			);
			{
				bool error_callback_fired = false;
				BOOST_CHECK_NO_THROW(c->rollback_transaction( connection_lock_ptr(), connection_lock_callback(),
				[&]( db_error const& err ) {
					error_callback_fired = true;
				}));
				BOOST_CHECK(error_callback_fired);
			}
			if (!transactions) {
				BOOST_CHECK_NO_THROW(c->begin_transaction(
				[&](connection_lock_ptr c_lock){
					BOOST_CHECK(c_lock);
					BOOST_CHECK((*c_lock)->in_transaction());
					BOOST_CHECK_NO_THROW((*c_lock)->commit_transaction(c_lock,
					[&](connection_lock_ptr c_lock){
						(*c_lock)->terminate();
					}, [&](db_error const&) {
						(*c_lock)->terminate();
					} ));
				},
				[](db_error const&){

				}));
				transactions++;
			}
		}, [] (connection_ptr c) {
		}, [](connection_ptr c, connection_error const& ec) {
			BOOST_FAIL(ec.what());
		},  opts, {
			{"client_encoding", "UTF8"},
			{"application_name", "pg_async"}
		}));
		io_service.run();
	}
}

BOOST_AUTO_TEST_CASE(TransactionDirtyTerminateTest)
{
	if (!test_database.empty()) {
		using namespace tip::db::pg;
		connection_options opts = connection_options::parse(test_database);

		#ifdef WITH_TIP_LOG
		local_log(logger::INFO) << "Transactions dirty terminate exit test";
		#endif
		boost::asio::io_service io_service;
		bool transaction_error = false;
		connection_ptr conn(connection::create(io_service,
		[&](connection_ptr c) {
			BOOST_CHECK_NO_THROW(c->begin_transaction(
			[&](connection_lock_ptr c_lock){
				BOOST_CHECK(c_lock);
				BOOST_CHECK((*c_lock)->in_transaction());

				(*c_lock)->terminate();
			},
			[&](db_error const&){
				transaction_error = true;
			}));
		}, [] (connection_ptr c) {
		}, [](connection_ptr c, connection_error const& ec) {
			BOOST_FAIL(ec.what());
		},  opts, {
			{"client_encoding", "UTF8"},
			{"application_name", "pg_async"}
		}));
		io_service.run();
		BOOST_CHECK(transaction_error);
	}
}
BOOST_AUTO_TEST_CASE(TransactionAutocommitTest)
{
	if (!test_database.empty()) {
		using namespace tip::db::pg;
		connection_options opts = connection_options::parse(test_database);
		#ifdef WITH_TIP_LOG
		local_log(logger::INFO) << "Transactions dirty terminate autocommit exit test";
		#endif
		boost::asio::io_service io_service;
		bool transaction_error = false;
		connection_ptr conn(connection::create(io_service,
		[&](connection_ptr c) {
			BOOST_CHECK_NO_THROW(c->begin_transaction(
			[&](connection_lock_ptr c_lock){
				BOOST_CHECK(c_lock);
				BOOST_CHECK((*c_lock)->in_transaction());

				(*c_lock)->terminate();
			},
			[&](db_error const&){
				transaction_error = false;
			}, true));
		}, [] (connection_ptr c) {
		}, [](connection_ptr c, connection_error const& ec) {
			BOOST_FAIL(ec.what());
		},  opts, {
			{"client_encoding", "UTF8"},
			{"application_name", "pg_async"}
		}));
		io_service.run();
		BOOST_CHECK(!transaction_error);
	}
}

BOOST_AUTO_TEST_CASE(TransactionDirtyUnlockTest)
{
	if (!test_database.empty()) {
		using namespace tip::db::pg;
		connection_options opts = connection_options::parse(test_database);
		#ifdef WITH_TIP_LOG
		local_log(logger::INFO) << "Transactions dirty unlock exit test";
		#endif
		boost::asio::io_service io_service;

		boost::asio::deadline_timer timer(io_service, boost::posix_time::milliseconds(100));

		bool transaction_error = false;
		int transactions = 0;
		connection_ptr conn (connection::create(io_service,
		[&](connection_ptr c) {
			if (!transactions) {
				BOOST_CHECK_NO_THROW(c->begin_transaction(
				[&](connection_lock_ptr c_lock){
					#ifdef WITH_TIP_LOG
					local_log() << "Transaction begin callback";
					#endif
					BOOST_CHECK(c_lock);
					BOOST_CHECK((*c_lock)->in_transaction());
					timer.async_wait([&](boost::system::error_code const& ec){
						if (!ec) {
							#ifdef WITH_TIP_LOG
							local_log(logger::WARNING) << "Transaction dirty unlock test timer expired";
							#endif
							conn->terminate();
						}
					});
				},
				[&](db_error const&){
					transaction_error = true;
				}));
				transactions++;
			}
		}, [] (connection_ptr c) {
		}, [](connection_ptr c, connection_error const& ec) {
			BOOST_FAIL(ec.what());
		},  opts, {
			{"client_encoding", "UTF8"},
			{"application_name", "pg_async"}
		}));
		io_service.run();
		BOOST_CHECK(transaction_error);
	}
}


BOOST_AUTO_TEST_CASE( ConnectionPoolTest )
{
	typedef std::shared_ptr< std::thread > thread_ptr;
	typedef std::chrono::high_resolution_clock clock_type;
	if (!test_database.empty()) {
		using namespace tip::db::pg;

		connection_options opts = connection_options::parse(test_database);
		boost::asio::io_service io_service;
		size_t pool_size = connection_pool;
		int req_count = num_requests;
		int thread_count = num_threads;

		clock_type::time_point tp = clock_type::now();
		clock_type::duration start = tp.time_since_epoch();

		std::shared_ptr< tip::db::pg::detail::connection_pool > pool(
				tip::db::pg::detail::connection_pool::create(io_service, pool_size, opts));

		BOOST_CHECK(pool);

		int sent_count = 0;
		int res_count = 0;
		int fail_count = 0;

		boost::asio::deadline_timer timer(io_service, boost::posix_time::seconds(deadline));
		timer.async_wait([&](boost::system::error_code const& ec){
			if (!ec) {
				#ifdef WITH_TIP_LOG
				local_log(logger::WARNING) << "Connection pool test timer expired";
				#endif
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
					[&] (connection_lock_ptr c) {
						int req_no = i;
						BOOST_CHECK(c);
						#ifdef WITH_TIP_LOG
						local_log(logger::TRACE) << "Obtained connection thread  "
								<< t_no << " request " << req_no;
						#endif
						(*c)->execute_query( "select * from pg_catalog.pg_class",
						[&] (connection_lock_ptr c, resultset r, bool complete) {
							BOOST_CHECK(c);
							if (complete)
								++res_count;
							#ifdef WITH_TIP_LOG
							local_log() << "Received a resultset columns: " << r.columns_size()
									<< " rows: " << r.size()
									<< " completed: " << std::boolalpha << complete;
							local_log() << "Sent requests: " << sent_count
									<< " Received results: " << res_count;
							#endif

							if (res_count >= req_count * thread_count) {
								pool->close();
								timer.cancel();
							}
						}, [](db_error const&){},
						c);
						++sent_count;
					},
					[&] (db_error const& ec) {
						++fail_count;
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
			BOOST_CHECK_EQUAL(fail_count, req_count * thread_count);
		} else {
			BOOST_CHECK_EQUAL(sent_count, req_count * thread_count);
			BOOST_CHECK_EQUAL(res_count, req_count * thread_count);
		}
		tp = clock_type::now();
		clock_type::duration run = tp.time_since_epoch() - start;
		double seconds = (double)run.count() * clock_type::period::num / clock_type::period::den;;
		#ifdef WITH_TIP_LOG
		local_log(logger::INFO) << "Running "
				<< req_count * thread_count << " requests in "
				<< thread_count << " threads with "
				<< pool_size << " connections took " << seconds << "s";
		#endif
	} else {
		BOOST_MESSAGE("Not running database connected tests");
	}
}

BOOST_AUTO_TEST_CASE(TransactionQueryTest)
{
	if (!test_database.empty()) {
		using namespace tip::db::pg;
		connection_options opts = connection_options::parse(test_database);
		#ifdef WITH_TIP_LOG
		local_log(logger::INFO) << "Transaction query test";
		#endif
		boost::asio::io_service io_service;
		bool transaction_error = false;
		connection_ptr conn(connection::create(io_service,
		[&](connection_ptr c) {
			BOOST_CHECK_NO_THROW(c->begin_transaction(
			[&](connection_lock_ptr c_lock){
				BOOST_CHECK(c_lock);
				BOOST_CHECK((*c_lock)->in_transaction());
				(*c_lock)->execute_query( "select * from pg_catalog.pg_class",
				[&] (connection_lock_ptr c_lock, resultset r, bool complete) {
					#ifdef WITH_TIP_LOG
					local_log() << "Received a resultset columns: " << r.columns_size()
							<< " rows: " << r.size()
							<< " completed: " << std::boolalpha << complete;
					#endif
					if (complete)
						(*c_lock)->terminate();
				}, [] (db_error const&) {}, c_lock);
			},
			[&](db_error const&){
				transaction_error = false;
			}, true));
		}, [] (connection_ptr c) {
		}, [](connection_ptr c, connection_error const& ec) {
			BOOST_FAIL(ec.what());
		},  opts, {
			{"client_encoding", "UTF8"},
			{"application_name", "pg_async"}
		}));
		io_service.run();
		BOOST_CHECK(!transaction_error);
	}
}

BOOST_AUTO_TEST_CASE(DatabaseServiceTest)
{
	typedef std::shared_ptr< std::thread > thread_ptr;
	typedef std::chrono::high_resolution_clock clock_type;
	using namespace tip::db::pg;
	BOOST_CHECK_THROW(
			db_service::get_connection_async("notthere"_db,
					connection_lock_callback(), error_callback()),
					std::runtime_error);

	if (!test_database.empty()) {
		// not initialized yet
		BOOST_CHECK_NO_THROW(db_service::add_connection(test_database));

		size_t pool_size = connection_pool;

		boost::asio::deadline_timer timer(db_service::io_service(),
				boost::posix_time::seconds(deadline));
		timer.async_wait([&](boost::system::error_code const& ec){
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

		db_service::get_connection_async(test_database,
		[&](connection_lock_ptr c){
            timer.cancel();
			db_service::stop();
		}, [](db_error const& ec) {

		});

		db_service::run();
	} else {
		BOOST_MESSAGE("Not running database connected tests");
	}
}

BOOST_AUTO_TEST_CASE(QueryTest)
{
	using namespace tip::db::pg;
	if (!test_database.empty()) {

		boost::asio::deadline_timer timer(db_service::io_service(),
				boost::posix_time::seconds(deadline));
		timer.async_wait([&](boost::system::error_code const& ec){
			if (!ec) {
				#ifdef WITH_TIP_LOG
				local_log(logger::WARNING) << "Run query test timer expired";
				#endif
				db_service::stop();
			}
		});

		resultset res;

		BOOST_CHECK_NO_THROW(db_service::add_connection(test_database));
		connection_options opts = connection_options::parse(test_database);
		{
			query (opts.alias, "create temporary table pg_async_test(b bigint)", true).run_async(
			[&](connection_lock_ptr c, resultset, bool){
				#ifdef WITH_TIP_LOG
				local_log() << "Query one finished";
				#endif
				BOOST_CHECK(c);
				query(c, "insert into pg_async_test values(1),(2),(3)").run_async(
				[&](connection_lock_ptr c, resultset, bool){
					#ifdef WITH_TIP_LOG
					local_log() << "Query two finished";
					#endif
					BOOST_CHECK(c);
					query(c, "select * from pg_async_test").run_async(
					[&](connection_lock_ptr c, resultset r, bool) {
						#ifdef WITH_TIP_LOG
						local_log() << "Query three finished";
						#endif
						BOOST_CHECK(c);
						res = r;
						query(c, "drop table pg_async_test").run_async(
						[&](connection_lock_ptr c, resultset r, bool) {
							#ifdef WITH_TIP_LOG
							local_log() << "Query four finished";
							#endif
							BOOST_CHECK(c);
                            timer.cancel();
							db_service::stop();
						}, [](db_error const&){});
					}, [](db_error const&){});
				}, [](db_error const&){});
			}, [](db_error const&){});
		}
		db_service::run();

		BOOST_CHECK_EQUAL(res.columns_size(), 1);
		BOOST_CHECK_EQUAL(res.size(), 3);
	} else {
		BOOST_MESSAGE("Not running database connected tests");
	}
}

BOOST_AUTO_TEST_CASE(ResultParsingTest)
{
	using namespace tip::db::pg;
	if (!test_database.empty() && !test::SCRIPT_SOURCE_DIR.empty()) {

		resultset res;

		BOOST_CHECK_NO_THROW(db_service::add_connection(test_database));
		std::string script_name = test::SCRIPT_SOURCE_DIR + "results-parse-test.sql";
		std::ifstream script(script_name);
		#ifdef WITH_TIP_LOG
		local_log() << "Script file name " << script_name;
		#endif
		if (script) {
			boost::asio::deadline_timer timer(db_service::io_service(),
					boost::posix_time::seconds(deadline));
			timer.async_wait([&](boost::system::error_code const& ec){
                timer.cancel();
				if (!ec) {
					#ifdef WITH_TIP_LOG
					local_log(logger::WARNING) << "Parse result set test timer expired";
					#endif
					db_service::stop();
				}
			});

			std::string script_str, line;
			while (getline(script, line)) {
				script_str += line;
			}
			connection_options opts = connection_options::parse(test_database);
			query(opts.alias, script_str).run_async(
			[&](connection_lock_ptr c, resultset res, bool complete) {
				#ifdef WITH_TIP_LOG
				{
					local_log() << "Received a resultset. Columns: " << res.columns_size()
							<< " rows: " << res.size() << " empty: " << res.empty();
				}
				#endif
				if (!res.empty()) {
					for (resultset::const_iterator row = res.begin(); row != res.end(); ++row) {
						#ifdef WITH_TIP_LOG
						tip::log::local local = local_log();
						local << "Row " << (row - res.begin()) << ": ";

						long id;
						std::string ctime, ctimetz;
						row.to(std::tie(id, ctime, ctimetz));

						local << "id: " << id << " ctime: " << ctime << " ";

						for (resultset::const_field_iterator f = row.begin(); f != row.end(); ++f) {
							local << f.as< std::string >() << " ";
						}
						#endif
					}
				}
			}, [&](db_error const&) {});

			db_service::run();
		}
	} else {
		BOOST_MESSAGE("Not running database connected tests");
	}
}

BOOST_AUTO_TEST_CASE(BoolValueTextParseTest)
{
	using namespace tip::db::pg;
	typedef tip::util::input_iterator_buffer buffer_type;

	std::string true_values = "TRUE t true y yes on 1";
	std::string false_values = "FALSE f false n no off 0";
	std::string non_bool_values = "foo bar trololo";

	{
		std::istringstream tv(true_values);
		std::string curr;
		while (tv >> curr) {
			{
				local_log() << "Parse bool value " << curr;
			}
			std::istringstream is(curr);

			bool val;
			BOOST_CHECK(query_parse< TEXT_DATA_FORMAT >(val)(is));
			BOOST_CHECK(val);

			std::vector<char> data(curr.begin(), curr.end());
			buffer_type buffer(data.begin(), data.end());
			BOOST_CHECK(query_parse< TEXT_DATA_FORMAT >(val)(buffer));
			BOOST_CHECK(val);
		}
	}

	{
		std::istringstream fv(false_values);
		std::string curr;
		while (fv >> curr) {
			{
				local_log() << "Parse bool value " << curr;
			}
			std::istringstream is(curr);

			bool val;
			BOOST_CHECK(query_parse< TEXT_DATA_FORMAT >(val)(is));
			BOOST_CHECK(!val);

			std::vector<char> data(curr.begin(), curr.end());
			buffer_type buffer(data.begin(), data.end());
			BOOST_CHECK(query_parse< TEXT_DATA_FORMAT >(val)(buffer));
			BOOST_CHECK(!val);
		}
	}

	{
		std::istringstream nv(non_bool_values);
		std::string curr;
		while (nv >> curr) {
			{
				local_log() << "Parse non-bool value " << curr;
			}
			std::istringstream is(curr);

			bool val = true;
			BOOST_CHECK(!query_parse< TEXT_DATA_FORMAT >(val)(is));
			BOOST_CHECK(val);

			std::vector<char> data(curr.begin(), curr.end());
			buffer_type buffer(data.begin(), data.end());
			BOOST_CHECK(!query_parse< TEXT_DATA_FORMAT >(val)(buffer));
			BOOST_CHECK(val);
		}
	}
}


BOOST_AUTO_TEST_CASE(ByteaValueTextParse)
{
	using namespace tip::db::pg;
	std::vector< std::pair< std::string, int > > valid_strings {
		{ "\\xdeadbeef", 4 },
		{ "\\x5c784445414442454546", 10 },
		{ "\\x", 0 }
	};

	std::vector< std::string > invalid_strings {
		"\\xdeadbee",
		"\\x5c78444g414442454546",
		"\\",
		""
	};
	for (auto vs : valid_strings) {
		{
			local_log() << "Parse bytea string '" << vs.first << "'";
		}
		std::istringstream is(vs.first);
		bytea val;

		BOOST_CHECK(query_parse< TEXT_DATA_FORMAT >(val)(is));
		BOOST_CHECK_EQUAL(val.data.size(), vs.second);
	}

	for (auto ivs : invalid_strings) {
		{
			local_log() << "Parse invalid bytea string '" << ivs << "'";
		}
		std::istringstream is(ivs);
		bytea val { {1, 2, 3, 4} };

		BOOST_CHECK(!query_parse< TEXT_DATA_FORMAT >(val)(is));
		BOOST_CHECK_EQUAL(val.data.size(), 4); // Not modified
	}
}
