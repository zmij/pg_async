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
#include <tip/db/pg/detail/result_impl.hpp>

#include <tip/db/pg/log.hpp>

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
		result_callback cb, query_error_callback err)
	: basic_state(conn), query_(query), result_(cb), error_(err), stage_(PARSE)
{
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
	std::string query_hash = "q_" +
			std::string(boost::md5( query_.c_str() ).digest().hex_str_value());
	if (conn.is_prepared(query_hash)) {
		std::string portal_name = "p_" +
				std::string(boost::md5( query_hash.c_str() ).digest().hex_str_value());
		if (stage_ == PARSE) {
			stage_ = BIND;
		}
		if (stage_ == BIND) {
			local_log() << "Bind params";
			// bind params
			conn.push_state(connection_state_ptr( new bind_state(conn, query_hash, portal_name) ));
		} else {
			local_log() << "Execute statement";
			// execute and fetch
			conn.push_state(connection_state_ptr(
					new execute_state(conn, portal_name, result_, error_) ));
		}
	} else {
		// parse
		conn.push_state( connection_state_ptr(
				new parse_state( conn, query_hash, query_, error_ ) ) );
	}
}

parse_state::parse_state(connection_base& conn, std::string const& query_name,
		std::string const& query,
		query_error_callback err)
	: basic_state(conn), query_name_(query_name), query_(query), error_(err)
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
		message m(parse_tag);
		m.write(query_name_);
		m.write(query_);
		m.write( (smallint)0 ); // Number of params
		conn.send(m);
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
			conn.set_prepared(query_name_);
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

bind_state::bind_state(connection_base& conn, std::string const& query_name,
		std::string const& portal_name)
	: basic_state(conn), query_name_(query_name), portal_name_(portal_name)
{
}

void
bind_state::do_enter()
{
	{
		message m(describe_tag);
		m.write('S');
		m.write(query_name_);
		conn.send(m);
	}
	{
		message m(bind_tag);
		m.write(portal_name_);
		m.write(query_name_);
		m.write((smallint)0); // parameter format codes
		m.write((smallint)0); // number of parameters
		m.write((smallint)0); // result format codes
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
		result_callback cb,
		query_error_callback err)
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
