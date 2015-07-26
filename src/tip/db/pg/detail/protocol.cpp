/*
 * protocol.cpp
 *
 *  Created on: 09 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/detail/protocol.hpp>
#include <tip/db/pg/common.hpp>
#include <tip/db/pg/protocol_io_traits.hpp>

#include <tip/db/pg/log.hpp>

#include <boost/asio/buffer.hpp>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <exception>
#include <sstream>
#include <map>

#include <boost/endian/conversion.hpp>


namespace tip {
namespace db {
namespace pg {
namespace detail {

namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "POSTGRE";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
// For more convenient changing severity, eg local_log(logger::WARNING)
using tip::log::logger;

namespace {
	tag_set_type FRONTEND_COMMANDS {
		empty_tag,
		bind_tag,
		close_tag,
		copy_data_tag,
		copy_done_tag,
		copy_fail_tag,
		describe_tag,
		execute_tag,
		flush_tag,
		function_call_tag,
		parse_tag,
		password_message_tag,
		query_tag,
		sync_tag,
		terminate_tag
	};
	tag_set_type BACKEND_COMMANDS {
		authentication_tag,
		backend_key_data_tag,
		bind_complete_tag,
		close_complete_tag,
		command_complete_tag,
		copy_data_tag,
		copy_done_tag,
		copy_in_response_tag,
		copy_out_response_tag,
		copy_both_response_tag,
		data_row_tag,
		empty_query_response_tag,
		error_response_tag,
		function_call_resp_tag,
		no_data_tag,
		notice_response_tag,
		notification_resp_tag,
		parameter_desription_tag,
		parameter_status_tag,
		parse_complete_tag,
		portal_suspended_tag,
		ready_for_query_tag,
		row_description_tag
	};
}  // namespace

tag_set_type const&
message::frontend_tags()
{
	return FRONTEND_COMMANDS;
}

tag_set_type const&
message::backend_tags()
{
	return BACKEND_COMMANDS;
}

message::message() :
		payload(), packed_(false)
{
	payload.reserve(256);
}

message::message(message_tag tag) :
		payload(5, 0), packed_(false)
{
	// TODO Check the tag
	payload[0] = (char)tag;
}

message_tag
message::tag() const
{
	if (!payload.empty()) {
		message_tag t = static_cast<message_tag>(payload.front());
		return t;
	}
	return empty_tag;
}

message::size_type
message::length() const
{
	const size_t header_size = sizeof(integer) + sizeof(byte);
	size_type len(0);
	if (payload.size() >= header_size) {
		// Decode length of message
		unsigned char* p = reinterpret_cast<unsigned char*>(&len);
		auto q = payload.begin() + 1;
		std::copy(q, q + sizeof(size_type), p);
		len = boost::endian::big_to_native(len);
	}

	return len;
}

message::const_range
message::buffer() const
{
	if (!packed_) {
		// Encode length of message
		integer len = size();
		protocol_write< BINARY_DATA_FORMAT >(payload.begin() + 1, len);
	}

	if (payload.front() == 0)
		return std::make_pair(payload.begin() + 1, payload.end());
	return std::make_pair(payload.begin(), payload.end());
}

size_t
message::size() const
{
	return payload.empty() ? 0 : payload.size() - 1;
}

size_t
message::buffer_size() const
{
	return payload.size();
}

message::const_iterator
message::input() const
{
	return curr_;
}

message::output_iterator
message::output()
{
	return std::back_inserter(payload);
}

void
message::reset_read()
{
	if (payload.size() <= 5) {
		curr_ = payload.end();
	} else {
		curr_ = payload.begin() + 5;
	}
}

bool
message::read(char& c)
{
	if (curr_ != payload.end()) {
		c = *curr_++;
		return true;
	}
	return false;
}

template <typename T>
void
write_int(message::buffer_type& payload, T val)
{
	protocol_write< BINARY_DATA_FORMAT >(payload, val);
}


bool
message::read(smallint& val)
{
	const_iterator c = protocol_read< BINARY_DATA_FORMAT >(curr_, payload.cend(), val);
	if (curr_ == c)
		return false;
	curr_ = c;
	return true;
}

bool
message::read(integer& val)
{
	const_iterator c = protocol_read< BINARY_DATA_FORMAT >(curr_, payload.cend(), val);
	if (curr_ == c)
		return false;
	curr_ = c;
	return true;
}

bool
message::read(std::string& val)
{
	const_iterator c = protocol_read< TEXT_DATA_FORMAT >( curr_, payload.cend(), val );
	if (curr_ == c)
		return false;
	curr_ = c;
	return true;
}

bool
message::read(std::string& val, size_t n)
{
	if (payload.end() - curr_ >= n) {
		for (size_t i = 0; i < n; ++i) {
			val.push_back(*curr_++);
		}
		return true;
	}
	return false;
}

bool
message::read(field_description& fd)
{
	field_description tmp;
	tmp.max_size = 0;
	integer type_oid;
	smallint fmt;
	if (read(tmp.name) &&
			read(tmp.table_oid) &&
			read(tmp.attribute_number) &&
			read(type_oid) &&
			read(tmp.type_size) &&
			read(tmp.type_mod) &&
			read(fmt)) {
		tmp.type_oid = (oids::type::oid_type)type_oid;
		tmp.format_code = (protocol_data_format)fmt;
		fd = tmp;
		return true;
	}
	return false;
}

bool
message::read(row_data& row)
{
	integer len = length();
	assert( len == size() && "Invalid message length");
	local_log(logger::OFF) << "Row data message size " << len;
	if (len < sizeof(integer) + sizeof(smallint)) {
		std::cerr << "Size of invalid data row message is " << len << "\n";
		assert ( len >= sizeof(integer) + sizeof(smallint) && "Invalid data row message");
	}
	smallint col_count(0);
	if (read(col_count)) {
		row_data tmp;
		tmp.offsets.reserve(col_count);
		size_t expected_sz = len - sizeof(integer)*(col_count + 1) - sizeof(int16_t);
		tmp.data.reserve( expected_sz );
		for (int16_t i = 0; i < col_count; ++i) {
			tmp.offsets.push_back(tmp.data.size());
			integer col_size(0);
			if (!read(col_size))
				return false;
			if (col_size == -1) {
				tmp.null_map.insert(i);
			} else if (col_size > 0) {
				const_iterator in = curr_;
				auto out = std::back_inserter(tmp.data);
				for (; in != curr_ + col_size; ++in) {
					*out++ = *in;
				}
				curr_ = in;
			}
		}
		row.swap(tmp);
		return true;
	}
	return false;
}

bool
message::read(notice_message& notice)
{
	char code(0);
	while (read(code) && code) {
		if (notice.has_field(code)) {
			read(notice.field(code));
		} else {
			std::string fval;
			read(fval);
			#ifdef WITH_TIP_LOG
			local_log(logger::WARNING) << "Unknown error/notice field ("
					<< code << ") with value " << fval;
			#endif
		}
	}
	return true;
}

void
message::write(char c)
{
	payload.push_back(c);
}

void
message::write(smallint v)
{
	write_int(payload, v);
}

void
message::write(integer v)
{
	write_int(payload, v);
}

void
message::write(std::string const& s)
{
	protocol_write< TEXT_DATA_FORMAT >(payload, s);
}

void
message::pack(message const& m)
{
	buffer(); // to write the length, if hasn't been packed already
	packed_ = true;
	payload.reserve(payload.size() + m.payload.size());
	const_range r = m.buffer();
	std::copy(r.first, r.second, std::back_inserter(payload));
}

//----------------------------------------------------------------------------
// row_data implementation
//----------------------------------------------------------------------------
row_data::size_type
row_data::size() const
{
	return offsets.size();
}

bool
row_data::empty() const
{
	return offsets.empty();
}

void
row_data::check_index(size_type index) const
{
	if (index >= offsets.size()) {
		std::ostringstream out;
		out << "Field index " << index << " is out of range [0.."
				<< size() << ")";
		throw std::out_of_range(out.str().c_str());
	}
}

bool
row_data::is_null(size_type index) const
{
	check_index(index);
	return null_map.count(index);
}

row_data::data_buffer_bounds
row_data::field_buffer_bounds(size_type index) const
{
	check_index(index);
	const_data_iterator s = data.begin() + offsets[index];
	if (index == offsets.size() - 1) {
		return std::make_pair(s, data.end());
	}
	return std::make_pair(s, data.begin() + offsets[index + 1]);
}

field_buffer
row_data::field_data(size_type index) const
{
	data_buffer_bounds bounds = field_buffer_bounds(index);
	return field_buffer(bounds.first, bounds.second);
}

//----------------------------------------------------------------------------
// notice_message implementation
//----------------------------------------------------------------------------
namespace {

const std::map<char, std::string notice_message::* > notice_fields {
	{ 'S', &notice_message::severity },
	{ 'C', &notice_message::sqlstate },
	{ 'M', &notice_message::message },
	{ 'D', &notice_message::detail },
	{ 'H', &notice_message::hint },
	{ 'P', &notice_message::position },
	{ 'p', &notice_message::internal_position },
	{ 'q', &notice_message::internal_query },
	{ 'W', &notice_message::where },
	{ 's', &notice_message::schema_name },
	{ 't', &notice_message::table_name },
	{ 'c', &notice_message::column_name },
	{ 'd', &notice_message::data_type_name },
	{ 'n', &notice_message::constraint_name },
	{ 'F', &notice_message::file_name },
	{ 'L', &notice_message::line },
	{ 'R', &notice_message::routine }
};

}  // namespace

bool
notice_message::has_field(char code) const
{
	return notice_fields.count(code);
}

std::string&
notice_message::field(char code)
{
	auto f = notice_fields.find(code);
	if (f == notice_fields.end()) {
		throw std::runtime_error("Invalid message field code");
	}
	std::string notice_message::*pf = f->second;
	return this->*pf;
}

std::ostream&
operator << (std::ostream& out, notice_message const& msg)
{
	std::ostream::sentry s(out);
	if (s) {
		out << "severity: " << msg.severity
				<< " SQL code: " << msg.sqlstate
				<< " message: '" << msg.message << "'";
		if (!msg.detail.empty()) {
			out << " detail: '" << msg.detail << "'";
		}
	}
	return out;
}

}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip
