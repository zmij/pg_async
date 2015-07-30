/*
 * fetch_state.cpp
 *
 *  Created on: Jul 20, 2015
 *      Author: zmij
 */

#include <tip/db/pg/detail/fetch_state.hpp>
#include <tip/db/pg/detail/result_impl.hpp>
#include <tip/db/pg/resultset.hpp>
#include <tip/db/pg/error.hpp>

#include <tip/db/pg/log.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGFETCH";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
// For more convenient changing severity, eg local_log(logger::WARNING)
using tip::log::logger;


fetch_state::fetch_state(connection_base& conn,
		result_callback const& cb,
		query_error_callback const& err)
	: basic_state(conn), callback_(cb), error_(err),
	  complete_(false), result_no_(0), bytes_read_(0)
{
}

void
fetch_state::do_exit()
{
	local_log() << "Fetched " << bytes_read_ << " bytes";
}

bool
fetch_state::do_handle_message(message_ptr m)
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
			if (m->read(row)) { // @todo pass row_description to the result
				// push it to the result set
				result_ptr res = result();
				res->rows().push_back(row);
			}
			return true;
		}
		default:
			break;
	}
	return false;
}

bool
fetch_state::do_handle_complete( command_complete const& m)
{
	complete_ = true;
	std::string stat;
	resultset res(result());
	local_log(logger::DEBUG) << "Command is complete " << m.command_tag
			<< " resultset columns " << res.columns_size()
			<< " rows " << res.size();
	// TODO Add the stat to result
	if (callback_) {
		callback_(resultset(result()), true);
	} else {
		local_log(logger::WARNING) << "No results callback";
	}
	result_.reset();
	return true;
}


void
fetch_state::on_package_complete(size_t bytes)
{
	if (result_ && !result_->rows().empty()) {
		if (callback_) {
			callback_(resultset(result_), false);
		}
	}
	bytes_read_ += bytes;
}

result_ptr
fetch_state::result()
{
	if (!result_) {
		local_log() << "Create a new resultset " << result_no_;
		result_.reset(new result_impl);
		++result_no_;
	}
	return result_;
}

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */
