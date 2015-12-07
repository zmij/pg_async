/**
 * session.cpp
 *
 *  Created on: 31 авг. 2015 г.
 *      Author: zmij
 */

#include <awm/game/auth/session.hpp>
#include <awm/game/auth/user.hpp>
#include <awm/game/auth/authn_error.hpp>

#include <tip/db/pg.hpp>
#include <tip/db/pg/io/uuid.hpp>
#include <tip/db/pg/io/ip_address.hpp>
#include <tip/db/pg/io/boost_date_time.hpp>
#include <tip/http/server/json_body_context.hpp>

#include <boost/asio/io_service.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <tip/log.hpp>

namespace awm {
namespace game {
namespace authn {

LOCAL_LOGGING_FACILITY(SESSION, TRACE)

namespace {

const tip::db::pg::dbalias db = "main"_db;
const std::string FIELDS = "sid, ctime, mtime, uid, ip";

const std::string CREATE_SQL = R"~(
insert into auth.sessions(uid, ip)
values($1, $2)
returning )~" + FIELDS;

const std::string UPDATE_SQL = R"~(
update auth.sessions
    set mtime = $2,
        ip = $3
where sid = $1
returning )~" + FIELDS;

}  // namespace

session::session()
{
}

session::session(tip::db::pg::resultset::row r)
{
	read(r);
}

session::~session()
{
	local_log(logger::DEBUG) << "session::~session " << sid_
			<< " duration: " << (mtime_ - ctime_);
	update();
	if (user_ && user_->current_session() == sid_) {
		user_->current_session(identity_type{{0}});
		user_->update([](user_ptr) {
			local_log() << "User updated";
		});
	}
}

void
session::read(tip::db::pg::resultset::row r)
{
	r.to(sid_, ctime_, mtime_, uid_, ip_);
}

user_ptr
session::get_user() const
{
	if (user_)
		user::LRU().put(user_);
	return user_;
}
void
session::set_user(user_ptr u)
{
	if (!u)
		return;
	if (u->uid() != uid_)
		throw std::runtime_error("Wrong user id when assigning to a session");
	user::LRU().put(u);
	user_ = u;
}

void
session::update()
{
	using namespace tip::db::pg;
	query(db, UPDATE_SQL, sid_, mtime_, ip_)
	([](transaction_ptr t, resultset res, bool) {
		local_log() << "Session data updated";
		t->commit();
	}, [](error::db_error const& e) {
		local_log(logger::ERROR) << "Database error when updating a session: "
				<< e.what();
	});
}

void
session::create(user_ptr u, ip_address const& ip,
		transaction_ptr tran, transaction_session_callback tcb)
{
	using namespace tip::db::pg;
	if (u && tran && tcb) {
		user::LRU().put(u);
		if (u->online() && LRU().exists(u->current_session())) {
			// Try to find a session
			session_ptr s = LRU().get(u->current_session());
			if (s->ip() == ip) {
				local_log() << "User " << u->uid() << " already has a session";
				tcb(tran, s);
				return;
			}
			// Access from another IP, change session
			local_log() << "Delete invalid session for user " << u->uid();
			LRU().erase(u->current_session());
		}

		local_log() << "Start session for user " << u->uid();
		query(tran, CREATE_SQL, u->uid(), ip)
		([u, tcb](transaction_ptr t, resultset res, bool) {
			if (res.size() == 1) {
				local_log() << "Started session for user " << u->uid();
				session_ptr s = std::make_shared< session >(res.front());
				s->set_user(u);
				u->current_session(s->sid());
				u->update(t,
				[s, tcb](transaction_ptr t, user_ptr u) {
					tcb(t, s);
				});
			}
		}, [u](error::db_error const& e) {
			local_log(logger::ERROR) << "Database error when creating a session "
					"for user " << u->uid();
		});
	}
}

session_lru_service&
session::LRU()
{
	boost::asio::io_service& svc = *tip::db::pg::db_service::io_service();
	if (!boost::asio::has_service< session_lru_service >(svc)) {
		register_lru(60, 15);
	}
	return boost::asio::use_service< session_lru_service >(svc);
}

void
session::register_lru(size_t cleanup, size_t timeout)
{
	LRU_CACHE_TIMEKEYINTRUSIVE_SERVICE(
			*tip::db::pg::db_service::io_service(),
			awm::game::authn, session,
			cleanup, timeout,
			obj->sid(), obj->mtime(), obj->mtime(tm));
}

void
session::clear_lru()
{
	LRU().clear();
}

//****************************************************************************
tip::http::server::reply::id session_context::id;

session_context::session_context(tip::http::server::reply r) :
		base_type(r), status_(no_session), sid_{{0}}
{
	auto hdrs = r.request_headers().equal_range(tip::http::XSessionID);
	if (hdrs.first != hdrs.second) {
		std::string session_str = hdrs.first->second;
		try {
			boost::uuids::uuid sid = boost::lexical_cast<boost::uuids::uuid>(session_str);

			tip::http::server::reply::io_service_ptr io_service = r.io_service();
			session_lru_service& session_lru = session::LRU();

			if (session_lru.exists(sid)) {
				local_log() << "Got session " << sid << " from request header";
				sid_ = sid;
				session_ptr session = session_lru.get(sid_); // update session in cache
				set_user(session->get_user()); // update user in cache
				r.add_header({tip::http::XSessionID, session_str});
				status_ = session_ok;
			} else {
				local_log(logger::WARNING) << "Got session " << sid
						<< " from request header, but it's not in the cache";
				status_ = session_expired;
			}
		} catch (boost::bad_lexical_cast const& e) {
			// Header value is not castable to uuid
			local_log() << "Failed to convert session header value " << session_str
					<< " to uuid";
			status_ = invalid_session;
		}
	} else {
		local_log() << "No X-Session-ID header in request";
	}
}

session_context::~session_context()
{
}

session_ptr
session_context::get_session() const
{
	if (!sid_.is_nil()) {
		tip::http::server::reply r = get_reply();
		session_lru_service& session_lru = session::LRU();
		if (session_lru.exists(sid_)) {
			return session_lru.get(sid_);
		}
	}
	return session_ptr();
}

void
session_context::set_session(session_ptr s)
{
	tip::http::server::reply r = get_reply();
	tip::http::server::reply::io_service_ptr io_service = r.io_service();
	session::LRU().put(s);

	sid_ = s->sid();
	set_user(s->get_user());
	status_ = session_ok;
	r.add_header({ tip::http::XSessionID, boost::lexical_cast< std::string >( sid_ ) });
}

user_ptr
session_context::get_user() const
{
	auto session = get_session();
	if (session)
		return session->get_user();
	return user_ptr();
}

void
session_context::set_user(user_ptr u)
{
	if (u) {
		auto session = get_session();
		if (session)
			session->set_user(u);
	} else {
		local_log(logger::WARNING) << "NULL user in session " << sid_;
	}
}

bool
authorized::operator ()(tip::http::server::reply r) const
{
	session_context& sctx = tip::http::server::use_context< session_context >(r);
	if (!(bool)sctx) {
		using tip::http::server::json_body_context;
		json_body_context& json = tip::http::server::use_context< json_body_context >(r);
		authn_error err("Not authorized");
		save(json.outgoing(), static_cast<tip::http::server::error const&>(err));
		r.done(err.status());
		return false;
	}
	user_ptr u = sctx.get_user();
	if (!u) {
		local_log(logger::WARNING) << "No user in session " << sctx.get_session()->sid();
		using tip::http::server::json_body_context;
		json_body_context& json = tip::http::server::use_context< json_body_context >(r);
		authn_error err("Not authorized");
		save(json.outgoing(), static_cast<tip::http::server::error const&>(err));
		r.done(err.status());
		return false;
	}
	return true;
}

} /* namespace authn */
} /* namespace game */
} /* namespace awm */
