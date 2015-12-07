/*
 * version.in.hpp
 *
 *  Created on: Jul 8, 2015
 *      Author: zmij
 */

#ifndef VERSION_IN_HPP_
#define VERSION_IN_HPP_

#include <string>

namespace tip {

/** Version number */
const std::string VERSION	= "@GIT_VERSION@";
/** Git branch of the build */
const std::string BRANCH	= "@GIT_BRANCH@";

}  // namespace tip

#endif /* VERSION_IN_HPP_ */
