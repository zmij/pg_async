/**
 * session.hpp
 *
 *  Created on: 31 авг. 2015 г.
 *      Author: zmij
 */

#ifndef TIP_GAME_AUTH_SESSION_HPP_
#define TIP_GAME_AUTH_SESSION_HPP_

#include <awm/game/auth/user_fwd.hpp>
#include <awm/game/auth/session_fwd.hpp>

#include <tip/http/server/reply_context.hpp>
#include <tip/lru-cache/lru_cache_service.hpp>
#include <tip/db/pg/resultset.hpp>

#include <boost/uuid/uuid.hpp>
#include <tip/util/uuid_hash.hpp>

#include <boost/asio/ip/address.hpp>
#include <boost/date_time/posix_time/ptime.hpp>

namespace awm {
namespace game {
namespace authn {

typedef tip::lru::lru_cache_service<
	session_ptr,
	std::function< boost::uuids::uuid( session_ptr ) >,
	std::function< timestamp_type(session_ptr) >,
	std::function< void(session_ptr, timestamp_type) >
> session_lru_service;

class session {
public:
	typedef game::callback< session > session_callback;
	typedef game::transaction_callback< session > transaction_session_callback;
	typedef boost::asio::ip::address ip_address;
public:
	session();
	session(tip::db::pg::resultset::row);
	virtual ~session();

	boost::uuids::uuid const&
	sid() const
	{ return sid_; }

	timestamp_type const&
	mtime() const
	{ return mtime_; }
	void
	mtime(timestamp_type const& v)
	{ mtime_ = v; }

	ip_address
	ip() const
	{ return ip_; }

	user_ptr
	get_user() const;
	void
	set_user(user_ptr u);

	static void
	create(user_ptr, ip_address const&,
			transaction_ptr, transaction_session_callback);

	static session_lru_service&
	LRU();
	static void
	register_lru(size_t cleanup, size_t timeout);
	static void
	clear_lru();
private:
	void
	read(tip::db::pg::resultset::row);
	void
	update();
private:
	boost::uuids::uuid	sid_;
	timestamp_type		ctime_;
	timestamp_type		mtime_;
	boost::uuids::uuid	uid_;
	ip_address			ip_;

	user_ptr			user_;
};

class session_context: public tip::http::server::reply::context {
public:
	typedef tip::http::server::reply::context base_type;
	enum status_type {
		no_session,
		session_ok,
		session_expired,
		invalid_session
	};
	static tip::http::server::reply::id id;
public:
	session_context( tip::http::server::reply r );
	virtual ~session_context();

	session_ptr
	get_session() const;
	void
	set_session(session_ptr);

	user_ptr
	get_user() const;
	void
	set_user(user_ptr);

	status_type
	status() const
	{ return status_; }
	explicit operator bool()
	{ return status_ == session_ok; }
private:
	status_type status_;
	boost::uuids::uuid sid_;
};

struct authorized {
	bool
	operator()(tip::http::server::reply r) const;
};

} /* namespace authn */
} /* namespace game */
} /* namespace awm */
#endif /* TIP_GAME_AUTH_SESSION_HPP_ */

