/*
 * User_fwd.hpp
 *
 *  Created on: Aug 10, 2015
 *      Author: zmij
 */

#ifndef TIP_GAME_AUTH_USER_FWD_HPP_
#define TIP_GAME_AUTH_USER_FWD_HPP_

#include <memory>
#include <string>
#include <functional>

#include <awm/game/common.hpp>

namespace awm {
namespace game {
namespace authn {

class user;
typedef std::shared_ptr< user > user_ptr;

typedef game::callback< user > user_callback;
typedef game::transaction_callback< user > transaction_user_callback;

} /* namespace authn */
} /* namespace game */
} /* namespace awm */


#endif /* TIP_GAME_AUTH_USER_FWD_HPP_ */
