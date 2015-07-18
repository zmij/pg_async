/*
 * simple_query_state.cpp
 *
 *  Created on: 11 июля 2015 г.
 *      Author: brysin
 */

#include <tip/db/pg/detail/simple_query_state.hpp>
#include <tip/db/pg/detail/idle_state.hpp>
#include <tip/db/pg/detail/basic_connection.hpp>
#include <tip/db/pg/detail/result_impl.hpp>
#include <tip/db/pg/resultset.hpp>
#include <tip/db/pg/error.hpp>
#include <tip/log/log.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

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

simple_query_state::simple_query_state(connection_base& conn,
		std::string const& q, result_callback cb, query_error_callback err)
	: basic_state(conn), exp_(q), callback_(cb), error_(err), complete_(false)
{
}

result_ptr
simple_query_state::result()
{
	if (!result_) {
		local_log() << "Create a new resultset";
		result_.reset(new result_impl);
	}
	return result_;
}

bool
simple_query_state::do_handle_message(message_ptr m)
{
	message_tag tag = m->tag();
	switch (tag) {
		case row_description_tag: {
			complete_ = false;
			int16_t col_cnt;
			m->read(col_cnt);

			result_ptr res = result();
			result_impl::row_description_type& desc = res->row_description();
			for (int i = 0; i < col_cnt; ++i) {
				field_description fd;
				if (m->read(fd)) {
					desc.push_back(fd);
				} else {
					local_log(logger::ERROR)
							<< "Failed to read field description " << i;
				}
			}
			return true;
		}
		case data_row_tag : {
			complete_ = false;
			row_data row;
			if (m->read(row)) {
				// push it to the result set
				result_ptr res = result();
				res->rows().push_back(row);
			}
			return true;
		}
		case command_complete_tag: {
			complete_ = true;
			std::string stat;
			m->read(stat);
			resultset res(result());
			local_log(logger::DEBUG) << "Command is complete " << stat
					<< " resultset columns " << res.columns_size()
					<< " rows " << res.size();
			// TODO Add the stat to result
			if (callback_) {
				callback_(resultset(result()), true);
			}
			result_.reset();
			return true;
		}
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
	return false;
}

bool
simple_query_state::do_handle_error(notice_message const& msg)
{
	local_log(logger::ERROR) << "Error when executing command \""
			<< exp_ << "\" " << msg;
	if (error_) {
		error_(query_error(msg.message, msg.severity, msg.sqlstate, msg.detail));
	}
	return true;
}

void
simple_query_state::on_package_complete(size_t bytes)
{
	//local_log() << "Package " << bytes << "b complete in " << name() << " state";
	if (result_ && !result_->rows().empty()) {
		if (callback_) {
			callback_(resultset(result_), false);
		}
	}
}

void
simple_query_state::do_enter()
{
	{
		local_log() << "Send query "
				<< (util::MAGENTA | util::BRIGHT)
				<< exp_
				<< logger::severity_color()
				<< " to server";
	}
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
