/*
 * test-environment.hpp
 *
 *  Created on: Jul 27, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_TEST_DB_TEST_ENVIRONMENT_HPP_
#define LIB_PG_ASYNC_TEST_DB_TEST_ENVIRONMENT_HPP_

#include <string>

namespace tip {
namespace db {
namespace pg {
namespace test {

class environment {
public:

	static std::string test_database;
	static int deadline;

	static int num_requests;
	static int num_threads;

	static int connection_pool;
};

} /* namespace test */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* LIB_PG_ASYNC_TEST_DB_TEST_ENVIRONMENT_HPP_ */
