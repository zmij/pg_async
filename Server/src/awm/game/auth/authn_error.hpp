/*
 * auth_error.hpp
 *
 *  Created on: Oct 6, 2015
 *      Author: zmij
 */

#ifndef TIP_GAME_AUTH_AUTHN_ERROR_HPP_
#define TIP_GAME_AUTH_AUTHN_ERROR_HPP_

#include <tip/http/server/error.hpp>

namespace awm {
namespace game {
namespace authn {

class authn_error : public tip::http::server::client_error {
public:
	authn_error(std::string const& message) :
		client_error("AUTHN", message, tip::http::response_status::ok) {}
	virtual ~authn_error() {}
};

}  // namespace authn
}  // namespace game
}  // namespace awm



#endif /* TIP_GAME_AUTH_AUTHN_ERROR_HPP_ */
