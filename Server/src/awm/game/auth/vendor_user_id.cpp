/*
 * nsuuid.cpp
 *
 *  Created on: Aug 31, 2015
 *      Author: zmij
 */

#include <tip/log.hpp>

#include <tip/db/pg.hpp>
#include <tip/db/pg/io/uuid.hpp>

#include <awm/game/auth/session.hpp>
#include <awm/game/auth/vendor_user_id.hpp>
#include <awm/game/auth/user.hpp>
#include <awm/game/auth/token.hpp>
#include <awm/game/auth/protocol.hpp>

#include <tip/http/server/reply_context.hpp>
#include <tip/http/server/json_body_context.hpp>
#include <tip/http/server/remote_address.hpp>

namespace awm {
namespace game {
namespace authn {
namespace ios {


LOCAL_LOGGING_FACILITY(NSUUID, TRACE);

namespace {

const tip::db::pg::dbalias db = "main"_db;

const std::string FIELDS = "uid, ctime, mtime, apple_uid";

const std::string CREATE_SQL = R"~(
insert into auth.apple_vendor_uids(uid, method, apple_uid)
values ($1, 'apple_vendor_uid', $2)
returning )~" + FIELDS;

const std::string GET_SQL =
"select " + FIELDS +
" from auth.apple_vendor_uids where apple_uid = $1";

}  // namespace

vendor_user_id::vendor_user_id()
{
}

vendor_user_id::vendor_user_id(tip::db::pg::resultset::row r)
{
	read(r);
}

vendor_user_id::~vendor_user_id()
{
}

void
vendor_user_id::read(tip::db::pg::resultset::row r)
{
	r.to( uid_, ctime_, mtime_, vendor_uid_);
}

void
vendor_user_id::create(identity_type const& uid,
		identity_type const& vendor_uid,
		transaction_ptr tran, transaction_callback tcb)
{
	using namespace tip::db::pg;
	if (tran && tcb) {
		local_log() << "Store Apple vendor uid " << vendor_uid
				<< " for user " << uid;
		query(tran, CREATE_SQL, uid, vendor_uid)
		([tcb](transaction_ptr t, resultset res, bool complete) {
			if (res.size() == 1) {
				vendor_user_id_ptr v = std::make_shared< vendor_user_id >(res.front());
				local_log() << "Apple vendor uid " << v->vendor_uid()
						<< " added to user " << v->uid();
				tcb(t, v);
			}
		}, [uid](error::db_error const& e) {
			local_log(logger::ERROR) << "Database error when adding Apple "
					"vendor uid for user " << uid << ": " << e.what();
		});
	}
}

void
vendor_user_id::get(identity_type const& vendor_uid, transaction_callback tcb)
{
	using namespace tip::db::pg;
	if (!vendor_uid.is_nil() && tcb) {
		local_log() << "Lookup Apple vendor uid " << vendor_uid;
		query(db, GET_SQL, vendor_uid)
		([tcb](transaction_ptr trx, resultset res, bool complete) {
			if (res.size() == 1) {
				tcb(trx, std::make_shared<vendor_user_id>(res.front()));
			} else {
				tcb(trx, vendor_user_id_ptr{});
			}
		}, [vendor_uid](error::db_error const& e){
			local_log(logger::ERROR) << "Database error when looking up Apple "
					"vendor id " << vendor_uid << ": " << e.what();
		});
	}
}

//****************************************************************************
void
vendor_uid_reg_handler::do_handle_request(tip::http::server::reply r,
		request_pointer req)
{
	using tip::http::server::reply;
	if (req && *req) {
		session_context& sctx = tip::http::server::use_context< session_context >(r);
		if ((bool)sctx) {
			// Online and registered - add new vendor id to the user account
			r.server_error(tip::http::response_status::not_implemented);
			return;
		} else {
			vendor_user_id::get(req->userid,
				std::bind(
					&vendor_uid_reg_handler::vendor_uid_lookup_handler,
						shared_this< vendor_uid_reg_handler >(), r,
						std::placeholders::_1, std::placeholders::_2,
						req));
			return;
		}
	} else {
		local_log() << "Invalid vendor id registration request ";
	}
	r.client_error(tip::http::response_status::expectation_failed);
}

void
vendor_uid_reg_handler::vendor_uid_lookup_handler(tip::http::server::reply r,
			transaction_ptr tran, vendor_user_id_ptr v,
			request_pointer req)
{
	if (v) {
		user::get(v->uid(), tran,
			std::bind(&vendor_uid_reg_handler::vendor_uid_created,
				shared_this< vendor_uid_reg_handler >(), r,
				std::placeholders::_1, std::placeholders::_2, v,
				tip::http::response_status::accepted));
	} else {
		user::create_user( req->locale, req->tz, tran,
			std::bind(&vendor_uid_reg_handler::user_created,
				shared_this< vendor_uid_reg_handler >(), r,
				std::placeholders::_1, std::placeholders::_2, req->userid));
	}
}

void
vendor_uid_reg_handler::user_created(tip::http::server::reply r,
		transaction_ptr tran, user_ptr u, identity_type const& vendor_uid)
{
	session_context& sctx = tip::http::server::use_context< session_context >(r);
	vendor_user_id::create(u->uid(), vendor_uid, tran,
			std::bind(&vendor_uid_reg_handler::vendor_uid_created,
				shared_this< vendor_uid_reg_handler >(), r,
				std::placeholders::_1, u, std::placeholders::_2,
				tip::http::response_status::created));
}

void
vendor_uid_reg_handler::vendor_uid_created(tip::http::server::reply r,
		transaction_ptr tran, user_ptr u, vendor_user_id_ptr v,
		tip::http::response_status::status_type s)
{
	token::create(u->uid(), tran,
		std::bind(&vendor_uid_reg_handler::token_created,
				shared_this< vendor_uid_reg_handler >(), r,
				std::placeholders::_1, u, std::placeholders::_2, s));
}

void
vendor_uid_reg_handler::token_created(tip::http::server::reply r,
		transaction_ptr tran, user_ptr user, token_ptr tkn,
		tip::http::response_status::status_type s)
{
	// create session, mark user online
	using tip::http::server::remote_address;
	remote_address& remote = tip::http::server::use_context<remote_address>(r);
	session::create(user, remote.address(), tran,
			std::bind(&vendor_uid_reg_handler::session_started,
				shared_this< vendor_uid_reg_handler >(), r,
				std::placeholders::_1, std::placeholders::_2,
				user, tkn, s));
}

void
vendor_uid_reg_handler::session_started(tip::http::server::reply r,
			transaction_ptr tran, session_ptr sess, user_ptr user,
			token_ptr tkn, tip::http::response_status::status_type s)
{
	using tip::http::server::json_body_context;
	tran->commit();

	session_context& sctx = tip::http::server::use_context< session_context >(r);
	sctx.set_session(sess);

	json_body_context& json = tip::http::server::use_context< json_body_context >(r);
	authn_response resp { tkn->uid(), user->name(), tkn->value() };
	resp.serialize( json.outgoing() );

	r.done(s);
}

}  // namespace ios
} /* namespace authn */
} /* namespace game */
} /* namespace awm */
