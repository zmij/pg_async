/*
 * token_fwd.hpp
 *
 *  Created on: Sep 1, 2015
 *      Author: zmij
 */

#ifndef TIP_GAME_AUTH_TOKEN_FWD_HPP_
#define TIP_GAME_AUTH_TOKEN_FWD_HPP_

#include <memory>

namespace awm {
namespace game {
namespace authn {

class token;
typedef std::shared_ptr< token > token_ptr;

} /* namespace authn */
} /* namespace game */
} /* namespace awm */

#endif /* TIP_GAME_AUTH_TOKEN_FWD_HPP_ */
