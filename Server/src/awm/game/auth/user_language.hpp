/*
 * user_language.hpp
 *
 *  Created on: Oct 12, 2015
 *      Author: zmij
 */

#ifndef TIP_GAME_AUTH_USER_LANGUAGE_HPP_
#define TIP_GAME_AUTH_USER_LANGUAGE_HPP_

#include <tip/http/server/reply.hpp>
#include "../../../../lib/http/include/tip/http/server/locale_manager.hpp"

namespace awm {
namespace game {
namespace authn {

struct user_language {
	tip::http::server::locale_manager::request_language
	operator()(tip::http::server::reply& r) const;
};

} /* namespace authn */
} /* namespace game */
} /* namespace awm */

#endif /* TIP_GAME_AUTH_USER_LANGUAGE_HPP_ */
