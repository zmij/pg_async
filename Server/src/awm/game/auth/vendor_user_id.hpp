/*
 * nsuuid.hpp
 *
 *  Created on: Aug 31, 2015
 *      Author: zmij
 */

#ifndef TIP_GAME_AUTH_VENDOR_USER_ID_HPP_
#define TIP_GAME_AUTH_VENDOR_USER_ID_HPP_

#include <boost/uuid/uuid.hpp>
#include <tip/db/pg/resultset.hpp>

#include <awm/game/auth/user_fwd.hpp>
#include <awm/game/auth/session_fwd.hpp>
#include <awm/game/auth/token_fwd.hpp>

#include <awm/game/auth/protocol.hpp>

#include <tip/http/server/json_body_context.hpp>
#include <tip/http/server/request_transformer.hpp>

//#include

namespace awm {
namespace game {
namespace authn {
namespace ios {

class vendor_user_id {
public:
	typedef game::callback< vendor_user_id > callback;
	typedef game::transaction_callback< vendor_user_id > transaction_callback;
public:
	vendor_user_id();
	vendor_user_id(tip::db::pg::resultset::row r);
//	vendor_user_id(uuid const& uid, uuid const& vendor_uid);

	virtual ~vendor_user_id();

	identity_type const&
	uid() const
	{ return uid_; }

	user_ptr
	user() const;

	identity_type const&
	vendor_uid() const
	{ return vendor_uid_; }

	void
	update(callback);
	void
	update(transaction_callback);

	static void
	create(identity_type const& uid, identity_type const& vendor_uid,
			transaction_ptr, transaction_callback);

	static void
	get(identity_type const& vendor_uid, transaction_callback);

private:
	void
	read(tip::db::pg::resultset::row r);
private:
	identity_type		uid_;

	timestamp_type		ctime_;
	timestamp_type		mtime_;

	identity_type		vendor_uid_;
};

typedef std::shared_ptr< vendor_user_id > vendor_user_id_ptr;

class vendor_uid_reg_handler :
		public tip::http::server::request_transformer<
				tip::http::server::json_transformer< vendor_uid_req > > {
public:
	vendor_uid_reg_handler() {}
	virtual ~vendor_uid_reg_handler() {}
private:
	virtual void
	do_handle_request(tip::http::server::reply r, request_pointer);

	void
	vendor_uid_lookup_handler(tip::http::server::reply r,
			transaction_ptr tran, vendor_user_id_ptr,
			request_pointer);

	void
	user_created(tip::http::server::reply r,
			transaction_ptr tran, user_ptr user,
			identity_type const& vendor_uid);
	void
	vendor_uid_created(tip::http::server::reply r,
			transaction_ptr tran, user_ptr user, vendor_user_id_ptr,
			tip::http::response_status::status_type);

	void
	token_created(tip::http::server::reply r,
			transaction_ptr tran, user_ptr user, token_ptr tkn,
			tip::http::response_status::status_type);

	void
	session_started(tip::http::server::reply r,
			transaction_ptr tran, session_ptr s, user_ptr user,
			token_ptr tkn, tip::http::response_status::status_type);
};

}  // namespace ios
} /* namespace authn */
} /* namespace game */
} /* namespace awm */

#endif /* TIP_GAME_AUTH_VENDOR_USER_ID_HPP_ */
