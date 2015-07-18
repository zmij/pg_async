/**
 * @file /tip-server/src/tip/db/pg/detail/startup.cpp
 * @brief
 * @date Jul 10, 2015
 * @author: zmij
 */

#include <tip/db/pg/detail/startup.hpp>
#include <tip/db/pg/detail/protocol.hpp>
#include <tip/db/pg/detail/idle_state.hpp>
#include <tip/db/pg/detail/basic_connection.hpp>

#ifdef WITH_TIP_LOG
#include <tip/log/log.hpp>
#include <tip/log/ansi_colors.hpp>
#endif

#include <tip/db/pg/detail/md5.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

#ifdef WITH_TIP_LOG
namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGAUTH";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace

using tip::log::logger;
#endif

startup_state::startup_state(connection_base& conn)
		: basic_state(conn), conn_state_(connection::DISCONNECTED)
{

}

bool
startup_state::do_handle_message(message_ptr m)
{
	conn_state_ = connection::CONNECTING;
	message_tag tag = m->tag();
	switch (tag) {
		case authentication_tag: {
			int32_t auth_state(-1);
			if (m->read(auth_state)) {
				switch (auth_state) {
					case OK: {
						#ifdef WITH_TIP_LOG
						{
							auto local = local_log(logger::INFO);
							local << "Database "
								<< (util::CLEAR) << (util::RED | util::BRIGHT)
								<< conn.uri()
								<< "[" << conn.database() << "]"
								<< logger::severity_color(local->severity())
								<< " connected";
						}
						#endif
						conn.transit_state(state_ptr(new idle_state(conn)));
						break;
					}
					case Cleartext: {
						#ifdef WITH_TIP_LOG
						local_log() << "Cleartext password requested";
						#endif
						message pm(password_message_tag);
						pm.write(conn.options().password);
						conn.send(pm);
						break;
					}
					case MD5Password: {
						#ifdef WITH_TIP_LOG
						local_log() << "MD5 password requested";
						#endif
						// Read salt
						std::string salt;
						m->read(salt, 4);
						connection_options const& co = conn.options();
						// Calculate hash
						std::string pwdhash = boost::md5((co.password + co.user).c_str()).digest().hex_str_value();
						std::string md5digest = std::string("md5") + boost::md5( (pwdhash + salt).c_str() ).digest().hex_str_value();
						// Construct and send message
						message pm(password_message_tag);
						pm.write(md5digest);
						conn.send(pm);
						break;
					}
					default:
						#ifdef WITH_TIP_LOG
						local_log(logger::ERROR) << "Unsupported authentication scheme "
							<< auth_state << " requested by server.";
						#endif
						// FIXME Bail out
						break;
				}
			}
			return true;
		}
		default:
			break;
	}
	return false;
}

}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip

