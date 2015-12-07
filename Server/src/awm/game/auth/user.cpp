/*
 * User.cpp
 *
 *  Created on: Aug 7, 2015
 *      Author: zmij
 */
#include <random>
#include <chrono>
#include <algorithm>

#include <awm/game/auth/user.hpp>

#include <tip/log.hpp>
#include <tip/db/pg.hpp>
#include <tip/db/pg/io/uuid.hpp>
#include <tip/db/pg/io/boost_date_time.hpp>

namespace awm {
namespace game {
namespace authn {

LOCAL_LOGGING_FACILITY(USER, DEBUG);

namespace {

const tip::db::pg::dbalias db = "main"_db;

const std::string USER_FIELDS = "uid, ctime, mtime, name, online, locale, timezone";

const std::string CREATE_USER_SQL = R"~(
insert into auth.users(name, locale, timezone)
values($1, $2, $3)
returning )~" + USER_FIELDS;

const std::string GET_USER_SQL =
"select " + USER_FIELDS +
" from auth.users where uid = $1";

const std::string UPDATE_SQL = R"~(
update auth.users
set mtime  = $2,
    name   = $3,
    online = $4,
    locale = $5,
    timezone = $6
where uid  = $1
returning )~" + USER_FIELDS;

// TODO Replace with moving to history or so
const std::string WIPE_USER_SQL = R"~(
delete from auth.users where uid = $1
)~";

const std::vector<std::string> NAME_ROOT	{ "Woaba", "Wom", "Futga", "Zunug", "Digd" };
const std::vector<std::string> NAME_SUFFIX 	{ "dug", "ugh", "urg", "agh", "url" };

unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
std::default_random_engine rndzr (seed1);
std::uniform_int_distribution<int> name_root(0,NAME_ROOT.size()-1);
std::uniform_int_distribution<int> name_suffix(0,NAME_SUFFIX.size()-1);


}  // namespace

user::user() : online_(false), current_session_({{0}})
{
	local_log() << "Create empty user";
}

user::user(tip::db::pg::resultset::row r) : online_(false), current_session_({{0}})
{
	local_log() << "Create user from db";
	read(r);
}

user::~user()
{
	local_log() << "user::~user " << (*this);
}

void
user::read(tip::db::pg::resultset::row r)
{
	r.to( uid_, ctime_, mtime_, name_, online_, locale_, timezone_ );
	facet_registry_.set_construction_args(uid_);
}

void
user::online(bool online)
{
	local_log(logger::INFO) << "User " << name_
			<< "{" << uid_ << "} is " << (online ? "online" : "offline");
	online_ = online;
}

void
user::current_session(identity_type const& sid)
{
	current_session_ = sid;
	online(!current_session_.is_nil());
}

void
user::wipe(simple_callback cb)
{
	using namespace tip::db::pg;
	local_log() << "Wipe user " << name_ << " (" << uid_ << ")";
	LRU().erase(uid_);
	user_ptr u = shared_from_this();
	query(db, WIPE_USER_SQL, uid_)
	([u, cb](transaction_ptr tran, resultset r, bool complete) {
		local_log() << "Wiped user " << u->name_ << " (" << u->uid_ << ")";
		tran->commit();
		if (cb) cb();
	},[u](error::db_error const& err) {
		local_log(logger::ERROR) << "Error when wiping user "
				<< u->name_ << " (" << u->uid_ << "): " << err.what();
	});
}

void
user::update(user_callback cb)
{
	using namespace tip::db::pg;
	user_ptr u = shared_from_this();
	db_service::begin(db,
	[u, cb](transaction_ptr tran) {
		u->update(tran, cb);
	}, [u](error::db_error const& err) {
		local_log(logger::ERROR) << "Error when updating user "
				<< *u << ": " << err.what();
	});
}

void
user::update(transaction_ptr tran, user_callback cb)
{
	using namespace tip::db::pg;
	update(tran, [cb](transaction_ptr trx, user_ptr u) {
		trx->commit();
		if (cb)
			cb(u);
	});
}

void
user::update(transaction_ptr tran, transaction_user_callback tcb)
{
	using namespace tip::db::pg;
	local_log() << "Update user " << *this;
	user_ptr u = shared_from_this();
	query(tran, UPDATE_SQL, uid_, mtime_, name_, online_, locale_, timezone_)
	([u, tcb](transaction_ptr tran, resultset res, bool complete) {
		if (res.size() > 0)
			u->read(res.front());
		if (tcb) {
			tcb(tran, u);
		}
	}, [u](error::db_error const& err) {
		local_log(logger::ERROR) << "Error when updating user "
				<< *u << ": " << err.what();
	});
}


void
user::create_user(std::string const& locale, std::string const& tz,
		user_callback cb, error_callback ecb)
{
	create_user(random_name(), locale, tz, cb, ecb);
}

void
user::create_user(std::string const& locale, std::string const& tz,
		transaction_user_callback tcb, error_callback ecb)
{
	create_user(random_name(), locale, tz, tcb, ecb);
}

void
user::create_user(std::string const& locale, std::string const& tz,
		transaction_ptr tran, transaction_user_callback tcb,
		error_callback ecb)
{
	create_user(random_name(), locale, tz, tran, tcb, ecb);
}

void
user::create_user(std::string const& name, std::string const& locale,
		std::string const& tz, user_callback cb, error_callback ecb)
{
	using namespace tip::db::pg;
	if (cb) {
		create_user(name, locale, tz,
		[cb](transaction_ptr trx, user_ptr u){
			trx->commit([cb, u](){ cb(u); });
		}, ecb);
	}
}

void
user::create_user(std::string const& name, std::string const& locale,
		std::string const& tz, transaction_user_callback tcb, error_callback ecb)
{
	using namespace tip::db::pg;
	if (tcb) {
		db_service::begin(db,
		[name, locale, tz, tcb, ecb](transaction_ptr trx)
		{
			create_user(name, locale, tz, trx, tcb, ecb);
		},
		[ecb](error::db_error const& err){
			local_log(logger::ERROR) << "Database error when creating a user: "
					<< err.what();
			if (ecb) ecb(err);
		});
	}
}

void
user::create_user(std::string const& name, std::string const& locale,
		std::string const& tz, transaction_ptr trx,
		transaction_user_callback tcb, error_callback ecb)
{
	using namespace tip::db::pg;
	if (tcb) {
		local_log() << "Create user " << name;
		query(trx, CREATE_USER_SQL, name, locale, tz)
		([tcb](transaction_ptr tran, resultset res, bool complete) {
			if (res.size() == 1) {
				user_ptr u = std::make_shared<user>(res.front());
				local_log(logger::INFO) << "User " << u->name_
						<< " created with id " << u->uid_;
				tcb(tran, u);
			} else {
				// FIXME This is an error
				local_log(logger::WARNING) << "User insertion resultset row count: "
						<< res.size();
			}
		}, [ecb](error::db_error const& err) {
			local_log(logger::ERROR) << "Database error when creating a user: "
					<< err.what();
			if (ecb) {
				ecb(err);
			}
		});
	}
}

void
user::get(identity_type const& uid, user_callback cb)
{
	using namespace tip::db::pg;
	if (cb) {
		local_log() << "Search user with uid " << uid;

		user_lru_service& user_lru = LRU();
		if (user_lru.exists(uid)) {
			cb(user_lru.get(uid));
		} else {
			query(db, GET_USER_SQL, uid)
			([cb](transaction_ptr tran, resultset res, bool complete) {
				tran->commit();
				if (res.size() == 1) {
					user_ptr u = std::make_shared<user>(res.front());
					cb(u);
				} else {
					// No such user
					cb(user_ptr());
				}
			}, [](error::db_error const& err) {
				local_log(logger::ERROR) <<
						"Database error when retrieving a user: "
						<< err.what();
			});
		}
	}
}

void
user::get(identity_type const& uid, transaction_ptr tran,
		transaction_user_callback tcb)
{
	using namespace tip::db::pg;
	if (tcb) {
		local_log() << "Search user with uid " << uid;

		user_lru_service& user_lru = LRU();
		if (user_lru.exists(uid)) {
			tcb(tran, user_lru.get(uid));
		} else {
			query(tran, GET_USER_SQL, uid)
			([tcb](transaction_ptr trx, resultset res, bool complete) {
				if (res.size() == 1) {
					tcb(trx, std::make_shared<user>(res.front()));
				} else {
					tcb(trx, user_ptr());
				}
			}, [](error::db_error const& err) {
				local_log(logger::ERROR) <<
						"Database error when retrieving a user: "
						<< err.what();
			});
		}
	}
}


std::string
user::random_name()
{
	std::string root 	= NAME_ROOT.at(name_root(rndzr));
	std::string suffix 	= NAME_SUFFIX.at(name_suffix(rndzr));
	return root+suffix;
}

std::string const&
user::fields()
{
	return USER_FIELDS;
}

user_lru_service&
user::LRU()
{
	boost::asio::io_service& svc = *tip::db::pg::db_service::io_service();
	if (!boost::asio::has_service< user_lru_service >(svc)) {
		register_lru(60, 60);
	}
	return boost::asio::use_service< user_lru_service >(svc);
}

void
user::register_lru(size_t cleanup, size_t timeout)
{
	LRU_CACHE_TIMEKEYINTRUSIVE_SERVICE(
			*tip::db::pg::db_service::io_service(),
			awm::game::authn, user,
			cleanup, timeout,
			obj->uid(), obj->mtime(), obj->mtime(tm));
}

void
user::clear_lru()
{
	LRU().clear();;
}

std::ostream&
operator << (std::ostream& out, user const& u)
{
	std::ostream::sentry s(out);
	if (s) {
		out << u.name() << " (" << u.uid() << ")";
	}
	return out;
}

//----------------------------------------------------------------------------
//	User facet base
//----------------------------------------------------------------------------

user_ptr
user_facet::user()
{
	user_lru_service& user_lru = user::LRU();
	if (user_lru.exists(uid_)) {
		return user_lru.get(uid_);
	}
	throw std::runtime_error("User is not in cache");
}

} /* namespace authn */
} /* namespace game */
} /* namespace awm */
