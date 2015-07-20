/*
 * simple_query_state.cpp
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/detail/simple_query_state.hpp>
#include <tip/db/pg/detail/idle_state.hpp>
#include <tip/db/pg/detail/basic_connection.hpp>
#include <tip/db/pg/detail/result_impl.hpp>
#include <tip/db/pg/resultset.hpp>
#include <tip/db/pg/error.hpp>

#include <tip/db/pg/log.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

#ifdef WITH_TIP_LOG
namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGQUERY";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
// For more convenient changing severity, eg local_log(logger::WARNING)
using tip::log::logger;
#endif

simple_query_state::simple_query_state(connection_base& conn,
		std::string const& q, result_callback cb, query_error_callback err)
	: fetch_state(conn, cb, err), exp_(q)
{
}

bool
simple_query_state::do_handle_message(message_ptr m)
{
	if (fetch_state::do_handle_message(m)) {
		return true;
	} else {
		message_tag tag = m->tag();
		switch (tag) {
			case ready_for_query_tag: {
				if (complete_) {
					conn.pop_state(this);
					conn.state()->handle_message(m);
				} // else - unexpected
				return true;
			}
			default:
				break;
		}
	}
	return false;
}

bool
simple_query_state::do_handle_error(notice_message const& msg)
{
	#ifdef WITH_TIP_LOG
	local_log(logger::ERROR) << "Error when executing command \""
			<< exp_ << "\" " << msg;
	#endif
	if (error_) {
		error_(query_error(msg.message, msg.severity, msg.sqlstate, msg.detail));
	}
	return true;
}

void
simple_query_state::do_enter()
{
	#ifdef WITH_TIP_LOG
	{
		local_log() << "Send query "
				<< (util::MAGENTA | util::BRIGHT)
				<< exp_
				<< logger::severity_color()
				<< " to server";
	}
	#endif
	message m(query_tag);
	m.write(exp_);
	conn.send(m);
}

void
simple_query_state::do_execute_query(std::string const& q, result_callback cb, query_error_callback err)
{
	conn.push_state( connection_state_ptr(
			new simple_query_state(conn, q, cb, err)) );
}

void
simple_query_state::do_handle_unlocked()
{
	conn.pop_state(this);
	// FIXME Handle unlocked with the top state
}

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */
