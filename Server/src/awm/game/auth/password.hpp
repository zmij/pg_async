/*
 * Password.hpp
 *
 *  Created on: Aug 10, 2015
 *      Author: zmij
 */

#ifndef TIP_GAME_AUTH_PASSWORD_HPP_
#define TIP_GAME_AUTH_PASSWORD_HPP_

#include <tip/db/pg/common.hpp>
#include <awm/game/auth/user.hpp>


namespace awm {
namespace game {
namespace authn {

class password {
public:
	typedef tip::db::pg::error_callback error_callback;
public:
	password();

	static void
	createPassword(user_ptr user, std::string const& login,
			tip::db::pg::bytea const& pwd_hash, user_callback cb,
			error_callback = error_callback());
	static void
	login(std::string const& login, tip::db::pg::bytea const& pwd_hash,
			user_callback cb);
};

} /* namespace authn */
} /* namespace game */
} /* namespace awm */

#endif /* TIP_GAME_AUTH_PASSWORD_HPP_ */
