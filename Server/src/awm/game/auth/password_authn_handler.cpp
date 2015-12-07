/*
 * PasswordAuthHandler.cpp
 *
 *  Created on: Aug 30, 2015
 *      Author: zmij
 */

#include <awm/game/auth/password_authn_handler.hpp>
#include <tip/log.hpp>

namespace awm {
namespace game {
namespace authn {

LOCAL_LOGGING_FACILITY(PWDAUTHN, TRACE);

password_authn_handler::password_authn_handler()
{
	// TODO Auto-generated constructor stub

}

password_authn_handler::~password_authn_handler()
{
	// TODO Auto-generated destructor stub
}

void
password_authn_handler::do_handle_request(tip::http::server::reply r)
{
	r.server_error(tip::http::response_status::not_implemented);
}

} /* namespace authn */
} /* namespace game */
} /* namespace awm */
