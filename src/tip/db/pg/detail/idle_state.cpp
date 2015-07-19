/*
 * idle_state.cpp
 *
 *  Created on: Jul 10, 2015
 *      Author: zmij
 */

#include <tip/db/pg/detail/idle_state.hpp>
#include <tip/db/pg/detail/simple_query_state.hpp>
#include <tip/db/pg/detail/transaction_state.hpp>

#include <tip/db/pg/detail/basic_connection.hpp>

#include <tip/db/pg/log.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

#ifdef WITH_TIP_LOG
namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGIDLE";
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

namespace {
const std::string STATE_NAME = "idle";
}  // namespace

idle_state::idle_state(connection_base& conn)
	: basic_state(conn)
{
}

bool
idle_state::do_handle_message(message_ptr m)
{
	message_tag tag = m->tag();
	switch (tag) {
		case ready_for_query_tag: {
			char stat(0);
			m->read(stat);
			#ifdef WITH_TIP_LOG
			local_log() << "Database "
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< conn.uri()
				<< "[" << conn.database() << "]"
				<< logger::severity_color()
				<< " is ready for query (" << stat << ")";
			#endif
			conn.ready();
			return true;
		}
		default:
			break;
	}
	return false;
}

void
idle_state::do_handle_unlocked()
{
	#ifdef WITH_TIP_LOG
	local_log() << "Database "
		<< (util::CLEAR) << (util::RED | util::BRIGHT)
		<< conn.uri()
		<< "[" << conn.database() << "] "
		<< logger::severity_color()
		<< "is unlocked and ready for query";
	#endif
	conn.ready();
}

std::string const
idle_state::get_name() const
{
	return STATE_NAME;
}

void
idle_state::do_begin_transaction(simple_callback cb, error_callback err, bool autocommit)
{
	conn.push_state( connection_state_ptr(
			new transaction_state(conn, cb, err, autocommit)));
}

void
idle_state::do_execute_query(std::string const& q, result_callback cb, query_error_callback err)
{
	conn.push_state( connection_state_ptr(
			new simple_query_state(conn, q, cb, err)) );
}

void
idle_state::do_terminate(simple_callback cb)
{
	#ifdef WITH_TIP_LOG
	local_log() << "Terminate state "
			<< (util::CLEAR) << (util::RED | util::BRIGHT)
			<< name()
			<< logger::severity_color();
	#endif
	message m(terminate_tag);
	std::shared_ptr<idle_state> _this = shared_this<idle_state>();
	conn.send(m,
	[_this, cb](boost::system::error_code const& ec, size_t bytes) {
		#ifdef WITH_TIP_LOG
		local_log() << "Termination message sent";
		#endif
		_this->conn.pop_state(_this.get());
		if (cb)
			cb();
	});
}

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */
