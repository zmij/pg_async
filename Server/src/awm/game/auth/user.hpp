/*
 * User.hpp
 *
 *  Created on: Aug 7, 2015
 *      Author: zmij
 */

#ifndef TIP_GAME_AUTH_USER_HPP_
#define TIP_GAME_AUTH_USER_HPP_

#include <memory>
#include <string>
#include <functional>
#include <iosfwd>

#include <tip/util/uuid_hash.hpp>

#include <tip/db/pg/resultset.hpp>

#include <awm/game/common.hpp>
#include <awm/game/auth/user_fwd.hpp>

#include <tip/lru-cache/lru_cache_service.hpp>
#include <tip/util/facet.hpp>

namespace awm {
namespace game {
namespace authn {

class user_facet;

typedef tip::lru::lru_cache_service<
	user_ptr,
	std::function< identity_type( user_ptr ) >,
	std::function< timestamp_type(user_ptr) >,
	std::function< void(user_ptr, timestamp_type) >
> user_lru_service;


class user : public std::enable_shared_from_this< user >,
		private boost::noncopyable  {
public:
	typedef boost::optional< std::string > string_opt;
	typedef tip::db::pg::error_callback error_callback;
public:
	user();
	user(tip::db::pg::resultset::row r);
	virtual ~user();

	identity_type const&
	uid() const
	{ return uid_; }

	std::string const&
	name() const
	{ return name_; }

	timestamp_type const&
	mtime() const
	{ return mtime_; }
	void
	mtime(timestamp_type const& tm)
	{ mtime_ = tm; }

	bool
	online() const
	{ return online_; }
	void
	online(bool);

	string_opt const&
	locale() const
	{ return locale_; }

	identity_type
	current_session() const
	{ return current_session_; }
	void
	current_session(identity_type const&);

	void
	wipe(simple_callback cb = simple_callback());

	// TODO Error callbacks
	void
	update(user_callback callback);
	void
	update(transaction_ptr tran, user_callback callback);
	void
	update(transaction_ptr tran, transaction_user_callback callback);

	static user_lru_service&
	LRU();
	static void
	register_lru(size_t cleanup, size_t timeout);
	static void
	clear_lru();
	//@{
	/** @name Create user with a random name */
	static void
	create_user(std::string const& locale, std::string const& tz,
			user_callback callback, error_callback = error_callback());
	static void
	create_user(std::string const& locale, std::string const& tz,
			transaction_user_callback callback,
			error_callback = error_callback());
	static void
	create_user(std::string const& locale, std::string const& tz,
			transaction_ptr, transaction_user_callback callback,
			error_callback = error_callback());
	//@}
	//@{
	/** @name Create user with a given name */
	static void
	create_user(std::string const& name,
			std::string const& locale, std::string const& tz,
			user_callback callback,
			error_callback = error_callback());
	static void
	create_user(std::string const& name,
			std::string const& locale, std::string const& tz,
			transaction_user_callback callback,
			error_callback = error_callback());
	static void
	create_user(std::string const& name,
			std::string const& locale, std::string const& tz,
			transaction_ptr, transaction_user_callback callback,
			error_callback = error_callback());
	//@}

	static void
	get(identity_type const& uid, user_callback);

	static void
	get(identity_type const& uid, transaction_ptr,
			transaction_user_callback);

	static std::string
	random_name();

	static std::string const&
	fields();
private:
	void
	read(tip::db::pg::resultset::row r);
private:
	identity_type	uid_;

	timestamp_type		ctime_;
	timestamp_type		mtime_;

	std::string			name_;
	bool				online_;

	string_opt			locale_;
	string_opt			timezone_;

	identity_type		current_session_;
private:
	//@{
	/** @name User facets */
	typedef tip::util::facet_registry< user_facet, identity_type > facet_registry_type;

	facet_registry_type facet_registry_;

	template < typename Facet >
	friend Facet&
	use_facet(user_ptr);
	//@}
};

std::ostream&
operator << (std::ostream&, user const&);

class user_facet : private boost::noncopyable {
public:
	user_facet(identity_type const& uid) : uid_(uid) {}
	virtual ~user_facet() {}

	identity_type const&
	uid() const
	{
		return uid_;
	}
	user_ptr
	user();
private:
	identity_type uid_;
};

template < typename Facet >
Facet&
use_facet(user_ptr u)
{
	return use_facet< Facet >( u->facet_registry_ );
}

} /* namespace authn */
} /* namespace game */
} /* namespace awm */

#endif /* TIP_GAME_AUTH_USER_HPP_ */
