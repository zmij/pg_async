/*
 * token.cpp
 *
 *  Created on: Sep 1, 2015
 *      Author: zmij
 */

#include <boost/uuid/uuid_io.hpp>

#include <awm/game/auth/token.hpp>
#include <awm/game/auth/protocol.hpp>
#include <awm/game/auth/session.hpp>
#include <awm/game/auth/user.hpp>
#include <tip/db/pg.hpp>
#include <tip/db/pg/io/uuid.hpp>
#include <tip/db/pg/io/boost_date_time.hpp>

#include <tip/log.hpp>

#include <tip/http/server/json_body_context.hpp>
#include <tip/http/server/remote_address.hpp>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include <md5.h>
#include <hex.h>
#include <awm/game/auth/authn_error.hpp>
#include <algorithm>

namespace awm {
namespace game {
namespace authn {

LOCAL_LOGGING_FACILITY(TOKEN, TRACE);

namespace {

const tip::db::pg::dbalias db = "main"_db;
const std::string FIELDS = "uid, ctime, mtime, token";

const std::string CREATE_SQL = R"~(
insert into auth.tokens(uid, method)
values ($1, 'token')
returning )~" + FIELDS;

const std::string LOOKUP_SQL =
	"select " + FIELDS + " from auth.tokens where token = $1";

}  // namespace

token::token()
{
}

token::token(tip::db::pg::resultset::row r)
{
	read(r);
}

token::~token()
{
}

void
token::read(tip::db::pg::resultset::row r)
{
	r.to(uid_, ctime_, mtime_, token_);
}

bool
token::validate(std::string const& auth) const
{
	std::ostringstream os;
	os << uid_ << token_;
	std::string auth_data = os.str();
	uint8_t digest[CryptoPP::Weak::MD5::DIGESTSIZE];
	CryptoPP::Weak::MD5().CalculateDigest(
		digest,
		reinterpret_cast< unsigned char const* >(auth_data.data()),
		auth_data.size()
	);

	std::string hex_str;
	CryptoPP::HexEncoder encoder(new CryptoPP::StringSink(hex_str), false);
	encoder.Put(digest, sizeof(digest));
	encoder.MessageEnd();

	std::string auth_lc;
	auth_lc.reserve(auth.size());
	std::transform(auth.begin(), auth.end(),
			std::back_inserter(auth_lc), ::tolower);

	if (hex_str != auth_lc) {
		local_log(logger::ERROR) << "Invalid auth string " << auth
				<< " expected " << hex_str;
		return false;
	}
	return true;
}

void
token::create(identity_type const& uid, transaction_ptr tran, token_transaction_callback tcb)
{
	using namespace tip::db::pg;
	if (!uid.is_nil() && tcb) {
		local_log() << "Add token for user " << uid;
		query(tran, CREATE_SQL, uid)
		([tcb](transaction_ptr trx, resultset res, bool) {
			if (res.size() == 1) {
				token_ptr tkn = std::make_shared< token >(res.front());
				tcb(trx, tkn);
			}
		}, [uid](error::db_error const& e) {
			local_log(logger::ERROR) << "Error creating token for user " << uid;
		});
	}
}

void
token::lookup(identity_type const& tok, token_transaction_callback tcb)
{
	using namespace tip::db::pg;
	if (!tok.is_nil() && tcb) {
		db_service::begin(db,
		[tok, tcb](transaction_ptr trx) {
			lookup(tok, trx, tcb);
		},
		[](error::db_error const& e){});
	}
}

void
token::lookup(identity_type const& tok,
			transaction_ptr trx, token_transaction_callback tcb)
{
	using namespace tip::db::pg;
	if (!tok.is_nil() && tcb) {
		query(trx, LOOKUP_SQL, tok)(
		[tcb](transaction_ptr trx, resultset res, bool) {
			if (res.size() == 1) {
				token_ptr tkn = std::make_shared< token >( res.front() );
				tcb(trx, tkn);
			} else {
				tcb(trx, token_ptr());
			}
		}, [tok](error::db_error const& e) {
			local_log(logger::ERROR) << "Error looking up token " << tok;
		});
	}
}

//****************************************************************************

std::string const&
token_authn_handler::category() const
{
	return LOG_CATEGORY;
}

void
token_authn_handler::do_handle_request(tip::http::server::reply r, request_pointer req)
{
	if (req && *req) {
		session_context& sctx = tip::http::server::use_context< session_context >(r);
		if ((bool)sctx) {
			// Online and registered
		} else {
			token::lookup(req->token,
					std::bind(&token_authn_handler::token_lookup_handler,
						shared_this< token_authn_handler >(), r,
						std::placeholders::_1, std::placeholders::_2, req));
		}
		return;
	}
	throw authn_error("Invalid token authn request");
}

void
token_authn_handler::token_lookup_handler(tip::http::server::reply r,
		transaction_ptr trx, token_ptr tkn,
		request_pointer req)
{
	if (tkn) {
		if (tkn->validate(req->auth)) {
			user::get(tkn->uid(), trx,
				std::bind( &token_authn_handler::user_lookup_handler,
						shared_this< token_authn_handler >(), r,
						std::placeholders::_1, std::placeholders::_2, tkn));
		} else {
			send_error(r, authn_error( "Invalid auth string" ));
		}
	} else {
		// Token not found
		send_error(r, authn_error( "Token not found" ));
	}
}

void
token_authn_handler::user_lookup_handler(tip::http::server::reply r,
		transaction_ptr trx, user_ptr u, token_ptr tkn)
{
	if (u) {
		using tip::http::server::remote_address;
		remote_address& remote = tip::http::server::use_context<remote_address>(r);
		session::create(u, remote.address(), trx,
				std::bind(&token_authn_handler::session_started,
					shared_this< token_authn_handler >(), r,
					std::placeholders::_1, std::placeholders::_2, u, tkn));
	} else {
		send_error(r, authn_error( "No user for token" ));
	}
}

void
token_authn_handler::session_started(tip::http::server::reply r,
			transaction_ptr trx, session_ptr sess, user_ptr user,
			token_ptr tkn)
{
	using tip::http::server::json_body_context;
	trx->commit();

	session_context& sctx = tip::http::server::use_context< session_context >(r);
	sctx.set_session(sess);

	json_body_context& json = tip::http::server::use_context< json_body_context >(r);
	authn_response resp { tkn->uid(), user->name(), tkn->value() };
	resp.serialize( json.outgoing() );

	r.done(tip::http::response_status::accepted);
}

} /* namespace authn */
} /* namespace game */
} /* namespace awm */
