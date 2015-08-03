/*
 * test_main.cpp
 *
 *  Created on: Jul 27, 2015
 *      Author: zmij
 */

#include <boost/program_options.hpp>
#include <gtest/gtest.h>
#include <tip/db/pg/log.hpp>

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

namespace {

#ifdef WITH_TIP_LOG
logger::event_severity log_level = logger::DEBUG;
#endif

}  // namespace

// Initialize the test suite
int
main( int argc, char* argv[] )
{
	::testing::InitGoogleTest(&argc, argv);

	using namespace tip::db::pg;
	namespace po = boost::program_options;
	logger::set_proc_name(argv[0]);
	logger::set_stream(std::cerr);
	po::options_description desc("Test options");

	desc.add_options()
			("database,b", po::value<std::string>(&test::environment::test_database),
					"database connection string")
			("pool-size,s", po::value<int>(&test::environment::connection_pool)->default_value(4),
					"connection pool size")
			("threads,x", po::value<int>(&test::environment::num_threads)->default_value(10),
					"requests threads number")
			("requests,q", po::value<int>(&test::environment::num_requests)->default_value(10),
					"number of requests per thread")
			#ifdef WITH_TIP_LOG
			("tip-log-level,v", po::value<logger::event_severity>(&log_level)->default_value(logger::INFO),
					"log level (TRACE, DEBUG, INFO, WARNING, ERROR)")
			#endif
			("run-deadline", po::value<int>(&test::environment::deadline)->default_value(5),
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

	if (test::environment::num_requests < 1) test::environment::num_requests = 1;
	if (test::environment::num_threads < 1) test::environment::num_threads = 1;
	if (test::environment::connection_pool < 1) test::environment::connection_pool = 1;

	#ifdef WITH_TIP_LOG
	logger::min_severity(log_level);
	logger::use_colors(vm.count("log-colors"));
	#endif

	return RUN_ALL_TESTS();
}

