/*
 * user_language.cpp
 *
 *  Created on: Oct 12, 2015
 *      Author: zmij
 */

#include <awm/game/auth/user_language.hpp>
#include <awm/game/auth/session.hpp>
#include <awm/game/auth/user.hpp>

namespace awm {
namespace game {
namespace authn {


tip::http::server::locale_manager::request_language
user_language::operator()(tip::http::server::reply& r) const
{
	session_context& sctx = tip::http::server::use_context< session_context >(r);
	if ((bool)sctx) {
		user_ptr u = sctx.get_user();
		if (u && u->locale().is_initialized() && !u->locale().value().empty()) {
			return std::make_pair(u->locale().value(), true);
		}
	}
	return std::make_pair("", false);
}

} /* namespace authn */
} /* namespace game */
} /* namespace awm */
