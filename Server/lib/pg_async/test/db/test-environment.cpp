/*
 * test-environment.cpp
 *
 *  Created on: Jul 27, 2015
 *      Author: zmij
 */

#include "test-environment.hpp"

namespace tip {
namespace db {
namespace pg {
namespace test {

std::string environment::test_database	= "";
int environment::deadline				= 3;

int environment::num_requests			= 10;
int environment::num_threads			= 4;

int environment::connection_pool		= 4;
} /* namespace test */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */
