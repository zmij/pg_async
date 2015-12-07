/*
 * common.hpp
 *
 *  Created on: Aug 10, 2015
 *      Author: zmij
 */

#ifndef TIP_GAME_COMMON_HPP_
#define TIP_GAME_COMMON_HPP_

#include <functional>
#include <tip/db/pg/common.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/uuid/uuid.hpp>

namespace awm {
namespace game {

typedef std::function< void () > simple_callback;
typedef std::function< void (bool) > bool_callback;

typedef tip::db::pg::transaction_ptr transaction_ptr;

template < typename T >
using callback = std::function< void(std::shared_ptr<T>) >;
template < typename T >
using transaction_callback =
		std::function< void( transaction_ptr, std::shared_ptr<T>) >;

typedef boost::posix_time::ptime		timestamp_type;
typedef boost::uuids::uuid				identity_type;

typedef tip::db::pg::integer			integer;

}  // namespace game
}  // namespace awm


#endif /* TIP_GAME_COMMON_HPP_ */
