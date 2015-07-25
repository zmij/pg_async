/*
 * prepare_state.cpp
 *
 *  Created on: Jul 20, 2015
 *      Author: zmij
 */

#include <tip/db/pg/detail/extended_query_state.hpp>
#include <tip/db/pg/detail/md5.hpp>
#include <tip/db/pg/detail/basic_connection.hpp>
#include <tip/db/pg/detail/protocol.hpp>
#include <tip/db/pg/protocol_io_traits.hpp>
#include <tip/db/pg/detail/result_impl.hpp>

#include <tip/db/pg/log.hpp>

#include <sstream>

namespace tip {
namespace db {
namespace pg {
namespace detail {

namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGEQUERY";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
// For more convenient changing severity, eg local_log(logger::WARNING)
using tip::log::logger;


extended_query_state::extended_query_state(connection_base& conn,
		std::string const& query,
		param_types const& types,
		params_buffer const& params,
		result_callback const& cb,
		query_error_callback const& err)
	: basic_state(conn), query_(query), param_types_(types), params_(params),
	  result_(cb), error_(err), stage_(PARSE)
{
	std::ostringstream os;
	os << query_ << " {";
	for (auto oid : param_types_) {
		os << oid;
	}
	os << "}";
	query_hash_ = "q_" +
			std::string(boost::md5( os.str().c_str() ).digest().hex_str_value());
}

bool
extended_query_state::do_handle_message(message_ptr m)
{
	message_tag tag = m->tag();
	switch (tag) {
		case ready_for_query_tag : {
			if (stage_ == PARSE) {
				stage_ = BIND;
				enter();
			} else if (stage_ == BIND) {
				stage_ = FETCH;
				enter();
			}
			return true;
		}
		case command_complete_tag: {
			conn.pop_state(this);
			conn.state()->handle_message(m);
			return true;
		}
		default:
			break;
	}
	return false;
}

void
extended_query_state::do_enter()
{
	if (conn.is_prepared(query_hash_)) {
		std::string portal_name = "p_" +
				std::string(boost::md5( query_hash_.c_str() ).digest().hex_str_value());
		if (stage_ == PARSE) {
			stage_ = BIND;
		}
		if (stage_ == BIND) {
			local_log() << "Bind params";
			// bind params
			conn.push_state(connection_state_ptr(
					new bind_state(conn, query_hash_, params_, error_) ));
		} else {
			local_log() << "Execute statement";
			// execute and fetch
			conn.push_state(connection_state_ptr(
					new execute_state(conn, "", result_, error_) ));
		}
	} else {
		// parse
		conn.push_state( connection_state_ptr(
				new parse_state( conn, query_hash_, query_, param_types_, error_ ) ) );
	}
}

parse_state::parse_state(connection_base& conn,
		std::string const& query_name,
		std::string const& query,
		extended_query_state::param_types const& types,
		query_error_callback const& err)
	: basic_state(conn), query_name_(query_name), query_(query), param_types_(types), error_(err)
{
}

void
parse_state::do_enter()
{
	{
		local_log() << "Parse query "
				<< (util::MAGENTA | util::BRIGHT)
				<< query_
				<< logger::severity_color();
	}

	{
		message parse(parse_tag);
		parse.write(query_name_);
		parse.write(query_);
		parse.write( (smallint)param_types_.size() );
		for (oids::type::oid_type oid : param_types_) {
			parse.write( (integer)oid );
		}
		conn.send(parse);
	}
	{
		message describe(describe_tag);
		describe.write('S');
		describe.write(query_name_);
		conn.send(describe);
	}
	{
		message sync(sync_tag);
		conn.send(sync);
	}
}

bool
parse_state::do_handle_message(message_ptr m)
{
	message_tag tag = m->tag();
	switch (tag) {
		case parse_complete_tag : {
			{
				local_log() << "Parse complete";
			}
			return true;
		}
		case parameter_desription_tag : {
			local_log() << "Parameter description message";
			return true;
		}
		case row_description_tag : {
			int16_t col_cnt;
			m->read(col_cnt);

			connection_base::row_description desc;
			for (int i = 0; i < col_cnt; ++i) {
				field_description fd;
				if (m->read(fd)) {
					fd.format_code = traits::has_binary_parser(fd.type_oid) ?
						BINARY_DATA_FORMAT : TEXT_DATA_FORMAT;
					desc.push_back(fd);
				} else {
					local_log(logger::ERROR)
							<< "Failed to read field description " << i;
				}
			}
			conn.set_prepared(query_name_, desc);
			conn.pop_state(this);
			return true;
		}
		case ready_for_query_tag : {
			{
				local_log() << "Ready for query in parse state";
			}
			message m(sync_tag);
			conn.send(m);
			return true;
		}
		default:
			break;
	}
	return false;
}

bind_state::bind_state(connection_base& conn,
		std::string const& query_name,
		params_buffer const& params,
		query_error_callback const& err)
	: basic_state(conn), query_name_(query_name),
	  params_(params)
{
}

void
bind_state::do_enter()
{
	{
		message m(bind_tag);
		m.write(std::string(""));
		m.write(query_name_);
		if (!params_.empty()) {
			local_log() << "Params buffer size " << params_.size();
			auto out = m.output();
			std::copy( params_.begin(), params_.end(), out );
		} else {
			m.write((smallint)0); // parameter format codes
			m.write((smallint)0); // number of parameters
		}
		connection_base::row_description const& row = conn.get_prepared_description(query_name_);
		m.write((smallint)row.size()); // result format codes
		for (auto fd : row) {
			m.write((smallint)fd.format_code);
		}
		conn.send(m);
	}
	{
		message m(sync_tag);
		conn.send(m);
	}
}

bool
bind_state::do_handle_message(message_ptr m)
{
	message_tag tag = m->tag();
	switch (tag) {
		case bind_complete_tag: {
			{
				local_log() << "Bind complete";
			}
			conn.pop_state(this);
			return true;
		}
		default:
			break;
	}
	return false;
}

execute_state::execute_state(connection_base& conn,
		std::string const& portal_name,
		result_callback const& cb,
		query_error_callback const& err)
	: fetch_state(conn, cb, err), portal_name_(portal_name),
	  sync_sent_(false), prev_rows_(0)
{
}

void
execute_state::do_enter()
{
	{
		message m(describe_tag);
		m.write('P');
		m.write(portal_name_);
		conn.send(m);
	}
	{
		message m(sync_tag);
		conn.send(m);
	}
}

bool
execute_state::do_handle_message(message_ptr m)
{
	message_tag tag = m->tag();
	if (fetch_state::do_handle_message(m) && tag != command_complete_tag) {
		return true;
	} else {
		switch (tag) {
			case ready_for_query_tag: {
				if (!complete_) {
					{
						local_log() << "Ready for query in execute state";
					}
					{
						sync_sent_ = false;
						message m(execute_tag);
						m.write(portal_name_);
						m.write((integer)0); // row limit
						conn.send(m);
					}

				}
				return true;
			}
			case command_complete_tag: {
				{
					local_log() << "Command complete in execute state";
				}
				conn.pop_state(this);
				conn.state()->handle_message(m);
				return true;
			}
			default:
				break;
		}
	}
	return false;
}

void
execute_state::on_package_complete(size_t bytes)
{
	fetch_state::on_package_complete(bytes);
	{
		local_log() << "Package complete in execute state";
	}
	if (!result_) {
		message m(sync_tag);
		conn.send(m);
		prev_rows_ = 0;
		local_log() << "Send sync";
	} else if (result_) {
		if (result_->size() == prev_rows_) {
			message m(sync_tag);
			conn.send(m);
			local_log() << "Send sync";
		}
		prev_rows_ = result_->size();
	}
//	if (!complete_ && !sync_sent_) {
//		message m(sync_tag);
//		conn.send(m);
//		sync_sent_ = true;
//	}
}

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */
