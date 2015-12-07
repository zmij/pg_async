/*
 * token.hpp
 *
 *  Created on: Sep 1, 2015
 *      Author: zmij
 */

#ifndef TIP_GAME_AUTH_TOKEN_HPP_
#define TIP_GAME_AUTH_TOKEN_HPP_

#include <boost/uuid/uuid.hpp>
#include <tip/db/pg/resultset.hpp>
#include <awm/game/common.hpp>
#include <awm/game/auth/token_fwd.hpp>
#include <awm/game/auth/user_fwd.hpp>
#include <awm/game/auth/session_fwd.hpp>
#include <awm/game/auth/protocol.hpp>

#include <tip/http/server/json_body_context.hpp>
#include <tip/http/server/request_transformer.hpp>

namespace awm {
namespace game {
namespace authn {

class token {
public:
	typedef callback< token > token_callback;
	typedef transaction_callback< token > token_transaction_callback;
public:
	token();
	token(tip::db::pg::resultset::row r);
	virtual ~token();

	identity_type const&
	uid() const
	{ return uid_; }

	identity_type const&
	value() const
	{ return token_; }

	bool
	validate( std::string const& auth ) const;

	static void
	create(identity_type const& uid,
			transaction_ptr, token_transaction_callback);

	static void
	lookup(identity_type const& tok, token_transaction_callback);
	static void
	lookup(identity_type const& tok,
			transaction_ptr, token_transaction_callback);
private:
	void
	read(tip::db::pg::resultset::row r);
private:
	identity_type		uid_;
	timestamp_type		ctime_;
	timestamp_type		mtime_;
	identity_type		token_;
};

class token_authn_handler :
		public tip::http::server::request_transformer<
			tip::http::server::json_transformer< token_authn_req > > {
public:
	token_authn_handler() {}
	virtual ~token_authn_handler() {}
private:
	virtual void
	do_handle_request(tip::http::server::reply, request_pointer);

	void
	token_lookup_handler( tip::http::server::reply, transaction_ptr trx,
		token_ptr tkn, request_pointer);
	void
	user_lookup_handler( tip::http::server::reply, transaction_ptr trx,
		user_ptr u, token_ptr tkn);

	void
	session_started(tip::http::server::reply r,
			transaction_ptr trx, session_ptr s, user_ptr user,
			token_ptr tkn);

	virtual std::string const&
	category() const;
};

} /* namespace authn */
} /* namespace game */
} /* namespace awm */

#endif /* TIP_GAME_AUTH_TOKEN_HPP_ */
