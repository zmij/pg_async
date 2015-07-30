/*
 * transaction_state.cpp
 *
 *  Created on: Jul 16, 2015
 *      Author: zmij
 */

#include <tip/db/pg/detail/transaction_state.hpp>
#include <tip/db/pg/detail/simple_query_state.hpp>
#include <tip/db/pg/detail/basic_connection.hpp>
#include <tip/db/pg/detail/protocol.hpp>

#include <tip/db/pg/error.hpp>

#include <tip/db/pg/log.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGTRAN";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
// For more convenient changing severity, eg local_log(logger::WARNING)
using tip::log::logger;

transaction_state::transaction_state(connection_base& conn,
		simple_callback const& cb,
		error_callback const& err,
		bool autocommit)
	: idle_state(conn), autocommit_(autocommit),
	message_pending_(false), complete_(false),
	  	  command_complete_(cb), error_(err)
{
}

void
transaction_state::do_enter()
{
	#ifdef WITH_TIP_LOG
	{
		local_log() << "Begin transaction";
	}
	#endif
	message m(query_tag);
	m.write("begin");
	conn.send(m);
	message_pending_ = true;
}

void
transaction_state::do_exit()
{
}

void
transaction_state::do_handle_unlocked()
{
	if (!message_pending_ && !complete_) {
		#ifdef WITH_TIP_LOG
		{
			local_log(logger::WARNING) << "Rollback transaction in connection unlock";
		}
		#endif
		dirty_exit(simple_callback());
		handle_unlocked();
	} else if (!message_pending_) {
		#ifdef WITH_TIP_LOG
		local_log() << "Pop state from handle unlocked";
		#endif
		conn.pop_state(this);
	}
}

bool
transaction_state::do_handle_message(message_ptr m)
{
	message_tag tag = m->tag();
	switch (tag) {
		case ready_for_query_tag: {
			if (command_complete_) {
				simple_callback cb = command_complete_;
				command_complete_ = simple_callback();
				cb();
			}
			return true;
		}
		default:
			break;
	}
	return false;
}

bool
transaction_state::do_handle_complete( command_complete const& m )
{
	local_log(logger::DEBUG) << "Command is complete " << m.command_tag;

	message_pending_ = false;

	if (complete_ && !exited) {
		local_log() << "Pop state from handle message";
		conn.pop_state(this);
	}
	return true;
}

bool
transaction_state::do_handle_error(notice_message const& msg)
{
	#ifdef WITH_TIP_LOG
	local_log(logger::ERROR) << "Error when starting transaction: "
			<< msg;
	#endif
	if (error_) {
		error_(query_error(msg.message, msg.severity, msg.sqlstate, msg.detail));
	}
	return true;
}

void
transaction_state::do_commit_transaction(simple_callback const& cb,
		error_callback const& err)
{
	if (!complete_) {
		#ifdef WITH_TIP_LOG
		{
			local_log() << "Commit transaction";
		}
		#endif
		std::shared_ptr<transaction_state> _this = shared_this<transaction_state>();
		command_complete_ = [_this, cb]() {
			_this->complete_ = true;
			if (cb) {
				cb();
			}
		};
		error_ = err;

		message_pending_ = true;
		message m(query_tag);
		m.write("commit");
		conn.send(m);
	}
}
void
transaction_state::do_rollback_transaction(simple_callback const& cb,
		error_callback const& err)
{
	if (!complete_) {
		#ifdef WITH_TIP_LOG
		{
			local_log() << "Rollback transaction";
		}
		#endif
		std::shared_ptr<transaction_state> _this = shared_this<transaction_state>();
		command_complete_ = [_this, cb]() {
			_this->complete_ = true;
			if (cb) {
				cb();
			}
		};
		error_ = err;

		message_pending_ = true;
		message m(query_tag);
		m.write("rollback");
		conn.send(m);
	}
}

void
transaction_state::do_terminate(simple_callback const& cb)
{
	#ifdef WITH_TIP_LOG
	{
		local_log() << "Terminate state "
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< name()
				<< logger::severity_color();
	}
	#endif
	if (!complete_) {
		dirty_exit(cb);
	} else {
		conn.pop_state(this);
		if (cb)
			cb();
	}
}

void
transaction_state::dirty_exit(simple_callback const& cb)
{
	#ifdef WITH_TIP_LOG
	{
		local_log(logger::WARNING) << "Dirty exit";
	}
	#endif
	//std::shared_ptr<transaction_state> _this = shared_this<transaction_state>();
	if (autocommit_) {
		simple_callback commit = [this, cb] {
			conn.pop_state(this);
			if (cb) cb();
		};
		commit_transaction(commit, error_);
	} else {
		simple_callback rollback = [this, cb]() {
			this->conn.pop_state(this);
			if (error_) {
				#ifdef WITH_TIP_LOG
				{
					local_log(logger::WARNING) << "Transaction rolled back on dirty exit";
				}
				#endif
				error_( query_error("Transaction rolled back") );
			}
			if (cb) cb();
		};
		rollback_transaction(rollback, error_);
	}
}

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */
