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

#include <tip/log/log.hpp>

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
		simple_callback cb, error_callback err, bool autocommit)
	: basic_state(conn), autocommit_(autocommit),
	message_pending_(false), complete_(false),
	  	  command_complete_(cb), error_(err)
{
}

void
transaction_state::do_enter()
{
	{
		local_log() << "Begin transaction";
	}
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
		{
			local_log(logger::WARNING) << "Rollback transaction in connection unlock";
		}
		dirty_exit(simple_callback());
		handle_unlocked();
	} else if (!message_pending_) {
		local_log() << "Pop state from handle unlocked";
		conn.pop_state(this);
	}
}

bool
transaction_state::do_handle_message(message_ptr m)
{
	message_tag tag = m->tag();
	switch (tag) {
		case command_complete_tag: {
			std::string stat;
			m->read(stat);
			local_log(logger::DEBUG) << "Command is complete " << stat;

			message_pending_ = false;

			if (command_complete_) {
				simple_callback cb = command_complete_;
				command_complete_ = simple_callback();
				cb();
			}
			if (complete_ && !exited) {
				local_log() << "Pop state from handle message";
				conn.pop_state(this);
			}
			return true;
		}
		case ready_for_query_tag:
			return true;
		default:
			break;
	}
	return false;
}

bool
transaction_state::do_handle_error(notice_message const& msg)
{
	local_log(logger::ERROR) << "Error when starting transaction: "
			<< msg;
	if (error_) {
		error_(query_error(msg.message, msg.severity, msg.sqlstate, msg.detail));
	}
	return true;
}

void
transaction_state::do_commit_transaction(simple_callback cb, error_callback err)
{
	if (!complete_) {
		{
			local_log() << "Commit transaction";
		}
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
transaction_state::do_rollback_transaction(simple_callback cb, error_callback err)
{
	if (!complete_) {
		{
			local_log() << "Rollback transaction";
		}
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
transaction_state::do_terminate(simple_callback cb)
{
	{
		local_log() << "Terminate state "
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< name()
				<< logger::severity_color();
	}
	if (!complete_) {
		dirty_exit(cb);
	} else {
		conn.pop_state(this);
		if (cb)
			cb();
	}
}

void
transaction_state::dirty_exit(simple_callback cb)
{
	{
		local_log(logger::WARNING) << "Dirty exit";
	}
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
				{
					local_log(logger::WARNING) << "Transaction rolled back on dirty exit";
				}
				error_( query_error("Transaction rolled back") );
			}
			if (cb) cb();
		};
		rollback_transaction(rollback, error_);
	}
}

void
transaction_state::do_execute_query(std::string const& q, result_callback cb, query_error_callback err)
{
	conn.push_state( connection_state_ptr(
			new simple_query_state(conn, q, cb, err)) );
}


} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */
