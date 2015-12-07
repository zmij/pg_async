/*
 * version.hpp
 *
 *  Created on: Nov 30, 2015
 *      Author: zmij
 */

#ifndef PG_ASYNC_SRC_TIP_DB_PG_VERSION_IN_HPP_
#define PG_ASYNC_SRC_TIP_DB_PG_VERSION_IN_HPP_

#ifdef VERSION_FILE
#include VERSION_FILE
#else
#include <string>

namespace tip {

/** Version number */
const std::string VERSION		= "@PROJECT_VERSION@";
/** Git revision */
const std::string GIT_VERSION	= "@GIT_VERSION@";
/** Git branch of the build */
const std::string BRANCH		= "@GIT_BRANCH@";

}  // namespace tip
#endif



#endif /* PG_ASYNC_SRC_TIP_DB_PG_VERSION_IN_HPP_ */
