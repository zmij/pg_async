/*
 * PasswordAuthHandler.hpp
 *
 *  Created on: Aug 30, 2015
 *      Author: zmij
 */

#ifndef TIP_GAME_AUTH_PASSWORD_AUTHN_HANDLER_HPP_
#define TIP_GAME_AUTH_PASSWORD_AUTHN_HANDLER_HPP_

#include <tip/http/server/request_handler.hpp>

namespace awm {
namespace game {
namespace authn {

class password_authn_handler : public tip::http::server::request_handler {
public:
	password_authn_handler();
	virtual ~password_authn_handler();
private:
	virtual void
	do_handle_request(tip::http::server::reply);
};

} /* namespace authn */
} /* namespace game */
} /* namespace awm */

#endif /* TIP_GAME_AUTH_PASSWORD_AUTHN_HANDLER_HPP_ */
