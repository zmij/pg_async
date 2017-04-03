/*
 * future_config.hpp
 *
 *  Created on: Mar 29, 2017
 *      Author: zmij
 */

#ifndef TIP_DB_PG_FUTURE_CONFIG_HPP_
#define TIP_DB_PG_FUTURE_CONFIG_HPP_

#ifdef WITH_BOOST_FIBERS
#include <boost/fiber/future.hpp>
#include <boost/fiber/fiber.hpp>
#else
#include <future>
#endif

namespace tip {
namespace db {
namespace pg {

#ifdef WITH_BOOST_FIBERS
template < typename _Res >
using promise = ::boost::fibers::promise< _Res >;
using fiber = ::boost::fibers::fiber;
#else
template < typename _Res >
using promise = ::std::promise< _Res >;
#endif

} /* namespace pg */
} /* namespace db */
} /* namespace tip */


#endif /* TIP_DB_PG_FUTURE_CONFIG_HPP_ */
