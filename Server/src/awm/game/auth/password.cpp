/*
 * Password.cpp
 *
 *  Created on: Aug 10, 2015
 *      Author: zmij
 */

#include <tip/db/pg.hpp>
#include <tip/db/pg/io/uuid.hpp>
#include <tip/db/pg/io/boost_date_time.hpp>

#include <awm/game/auth/password.hpp>

#include <tip/log.hpp>

namespace awm {
namespace game {
namespace authn {

LOCAL_LOGGING_FACILITY(PASSWORD, DEBUG);

namespace {

const tip::db::pg::dbalias db = "main"_db;

const std::string CREATE_PASSWORD = R"~(
insert into auth.passwords (uid, login, pwd_hash) 
values ($1, $2, $3)
)~";

// TODO Share user columns
const std::string PASSWORD_SELECT = R"~(
		select u.uid,
			u.ctime,
			u.mtime,
			u.name,
			u.online
		from auth.users u
		join auth.passwords p on (p.uid = u.uid)
		where p.login = $1 and p.pwd_hash = $2)~";
}  // namespace

password::password()
{
	// TODO Auto-generated constructor stub

}

void
password::createPassword(user_ptr user, std::string const& login,
		tip::db::pg::bytea const& pwd_hash, user_callback cb, error_callback ecb)
{
	using namespace tip::db::pg;
	query(db, CREATE_PASSWORD, user->uid(), login, pwd_hash)
	([user, cb](transaction_ptr tran, resultset res, bool complete) {
		tran->commit();
		if (cb) {
			cb(user);
		}
	}, [ecb](error::db_error const& err) {
		// FIXME Handle error. Most likely error - duplicate login
		if (ecb) {
			ecb(err);
		}
	});
}

void
password::login(std::string const& login, tip::db::pg::bytea const& pwd_hash,
		user_callback cb)
{
	using namespace tip::db::pg;
	query(db, PASSWORD_SELECT, login, pwd_hash)(
	[cb](transaction_ptr tran, resultset res, bool complete) {
		if (res.size() > 0) {
			user_ptr u = std::make_shared<user>(res.front());
			u->online(true);
			u->update(tran, cb);
		} else {
			tran->commit();
			if (cb)
				cb(user_ptr());
		}
	}, [&](error::db_error const& e) {

	});
}

} /* namespace authn */
} /* namespace game */
} /* namespace awm */
