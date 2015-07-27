/*
 * pg_commands.hpp
 *
 *  Created on: 07 июля 2015 г.
 *     @author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_COMMANDS_HPP_
#define TIP_DB_PG_DETAIL_COMMANDS_HPP_

#include <functional>
#include <vector>
#include <set>
#include <string>
#include <iosfwd>

#include <tip/db/pg/common.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

/**
 * @e message_tag Postgre message codes
 * @see http://www.postgresql.org/docs/current/static/protocol-message-formats.html
 */
enum message_tag {
	empty_tag				= '\0', /** (F) Startup message, SSL request, cancel request */
	authentication_tag		= 'R', /**< (B) All authentication requests begin with 'R' */
	backend_key_data_tag	= 'K', /**< (B) Backend key data (cancellation key data) */
	bind_tag				= 'B', /**< (F) Bind parameters command */
	bind_complete_tag		= '2', /**< (B) Server's reply to bind complete command */
	close_tag				= 'C', /**< (F) Close prepared statement or a portal */
	close_complete_tag		= '3', /**< (B) */
	command_complete_tag	= 'C', /**< (B) */
	copy_data_tag			= 'd', /**< (B&F) */
	copy_done_tag			= 'c', /**< (B&F) */
	copy_fail_tag			= 'f', /**< (F) */
	copy_in_response_tag	= 'G', /**< (B) */
	copy_out_response_tag	= 'H', /**< (B) */
	copy_both_response_tag	= 'W', /**< (B) */
	data_row_tag			= 'D', /**< (B) */
	describe_tag			= 'D', /**< (F) */
	empty_query_response_tag= 'I', /**< (B) */
	error_response_tag		= 'E', /**< (B) */
	execute_tag				= 'E', /**< (F) */
	flush_tag				= 'H', /**< (F) */
	function_call_tag		= 'F', /**< (F) */
	function_call_resp_tag	= 'V', /**< (B) */
	no_data_tag				= 'n', /**< (B) */
	notice_response_tag		= 'N', /**< (B) */
	notification_resp_tag	= 'A', /**< (B) */
	parameter_desription_tag= 't', /**< (B) */
	parameter_status_tag	= 'S', /**< (B) */
	parse_tag				= 'P', /**< (F) */
	parse_complete_tag		= '1', /**< (B) */
	password_message_tag	= 'p', /**< (F) */
	portal_suspended_tag	= 's', /**< (B) */
	query_tag				= 'Q', /**< (F) */
	ready_for_query_tag		= 'Z', /**< (B) */
	row_description_tag		= 'T', /**< (B) */
	sync_tag				= 'S', /**< (F) */
	terminate_tag			= 'X', /**< (F) */
};
typedef std::set<message_tag> tag_set_type;

struct row_data;
struct notice_message;

/**
 * On-the-wire message of PostgreSQL protocol v3.
 *
 */
class message {
public:
	/** Buffer type for the message */
	typedef std::vector<char> buffer_type;

	/** Input iterator for the message buffer */
	typedef buffer_type::const_iterator const_iterator;
	/** Output iterator for the message buffer */
	typedef std::back_insert_iterator<buffer_type> output_iterator;

	/** A range of iterators for input */
	typedef std::pair<const_iterator, const_iterator> const_range;

	/** Length type for the message */
	typedef integer size_type;
public:
	/**
	 * Construct message for reading from the stream
	 */
	message();
	/**
	 * Construct message for sending to the backend.
	 * The tag must be one of allowed for the frontend.
	 */
	explicit message(message_tag tag);

	message_tag
	tag() const; /**< PostgreSQL message tag */

	size_type
	length() const; /**< Length encoded in payload, bytes 1-4 */

	/**
	 * Size of payload, minus 1 (for the tag)
	 * When reading from stream, it must match the length
	 */
	size_t
	size() const;
	/**
	 * Full size of buffer including the tag
	 * @return
	 */
	size_t
	buffer_size() const;

	/**
	 * A pair of iterators for constructing buffer for writing into the stream
	 */
	const_range
	buffer() const;

	/**
	 * Iterator to current read position
	 * @return
	 */
	const_iterator
	input() const;
	/**
	 * An interator to write into the buffer
	 */
	output_iterator
	output();

	//@{
	/** @name Stream read interface */
	/**
	 * Move the read iterator to the beginning of actual payload
	 */
	void
	reset_read();
	/**
	 * Read a byte from the message buffer
	 * @param c the char
	 * @return true if the operation was successful
	 */
	bool
	read(char&);

	/**
	 * Read a two-byte integer from the message buffer
	 * @param v the integer
	 * @return true if the operation was successful
	 */
	bool
	read(smallint&);

	/**
	 * Read a 4-byte integer from the message buffer
	 * @param v the integer
	 * @return true if the operation was successful
	 */
	bool
	read(integer&);

	/**
	 * Read a null-terminated string from the message buffer
	 * @param s the string
	 * @return true if the operation was successful
	 */
	bool
	read(std::string&);

	/**
	 * Read n bytes from message to the string.
	 * @param s the string
	 * @param n nubmer of bytes
	 * @return true if the operation was successful
	 */
	bool
	read(std::string&, size_t n);

	/**
	 * Read field description from the message buffer
	 * @param fd field description
	 * @return true if the operation was successful
	 */
	bool
	read(field_description& fd);

	/**
	 * Read data row from the message buffer
	 * @param row data row
	 * @return true if the operation was successful
	 */
	bool
	read(row_data& row);

	/**
	 * Read a notice or error message form the message buffer
	 * @param notice
	 * @return true if the operation was successful
	 */
	bool
	read(notice_message& notice);
	//@}

	//@{
	/** @name Stream write interface */
	/**
	 * Write char to the message buffer
	 * @param c the char
	 */
	void
	write(char);
	/**
	 * Write a 2-byte integer to the message buffer
	 * @param v the integer
	 */
	void
	write(smallint);
	/**
	 * Write a 4-byte integer to the message buffer
	 * @param v the integer
	 */
	void
	write(integer);
	/**
	 * Write a NULL-terminated string to the message buffer
	 * @param s the string
	 */
	void
	write(std::string const&);
	//@}

	/**
	 * Append a message to send in a single network package
	 * @param
	 */
	void
	pack(message const&);

	//@{
	/** @name Protocol-related static interface */
	/**
	 * Tags that can be send by the frontend
	 * @return @c tag_set_type a set of tags
	 */
	static tag_set_type const&
	frontend_tags();
	/**
	 * Tags that can be send by the backend
	 * @return @c tag_set_type a set of tags
	 */
	static tag_set_type const&
	backend_tags();
	//@}
private:
	mutable buffer_type	payload;
	const_iterator curr_;
	bool packed_;
};

struct row_data {
	typedef std::vector<byte>	data_buffer;
	typedef data_buffer::const_iterator const_data_iterator;
	typedef std::pair<const_data_iterator, const_data_iterator> data_buffer_bounds;

	typedef uint16_t size_type;
	typedef std::vector< integer > offsets_type;
	typedef std::set< size_type > null_map_type;

	offsets_type offsets;
	data_buffer data;
	null_map_type null_map;

	size_type
	size() const; /**< Number of fields in the row */

	bool
	empty() const; /**< Is the row empty */

	/**
	 * Is field at index is null
	 * @param index
	 * @return true if the value in the field is null
	 * @throw out_of_range if the index is out of range
	 */
	bool
	is_null(size_type index) const;

	/**
	 * Buffer bounds for field with index
	 * @param index
	 * @return a pair of iterators in the data buffer
	 * @throw out_of_range if the index is out of range
	 * @throw value_is_null if the field is null
	 */
	data_buffer_bounds
	field_buffer_bounds(size_type index) const;

	field_buffer
	field_data(size_type index) const;

	void
	swap(row_data& rhs)
	{
		offsets.swap(rhs.offsets);
		data.swap(rhs.data);
		null_map.swap(rhs.null_map);
	}
private:
	void
	check_index(size_type index) const;
};

struct notice_message {
	std::string severity;
	std::string sqlstate;
	std::string message;
	std::string detail;
	std::string hint;
	std::string position;
	std::string internal_position;
	std::string internal_query;
	std::string where;
	std::string schema_name;
	std::string table_name;
	std::string column_name;
	std::string data_type_name;
	std::string constraint_name;
	std::string file_name;
	std::string line;
	std::string routine;

	bool
	has_field(char code) const;
	std::string&
	field(char code);
};

std::ostream&
operator << (std::ostream&, notice_message const&);

struct command_complete_message {
	std::string command_tag;
};

}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* TIP_DB_PG_DETAIL_COMMANDS_HPP_ */
