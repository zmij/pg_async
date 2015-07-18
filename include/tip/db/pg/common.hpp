/**
 * common.hpp
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#ifndef TIP_DB_PG_COMMON_HPP_
#define TIP_DB_PG_COMMON_HPP_

#include <string>
#include <iosfwd>
#include <vector>
#include <functional>
#include <memory>
#include <boost/integer.hpp>

#include <tip/util/streambuf.hpp>

namespace tip {
namespace db {
namespace pg {

typedef boost::int_t<16>::exact 	int16_t;
typedef boost::uint_t<16>::exact	uint16_t;
typedef boost::int_t<32>::fast		int32_t;
typedef boost::uint_t<32>::fast		uint32_t;
typedef boost::int_t<64>::fast		int64_t;

const int32_t PROTOCOL_VERSION = (3 << 16); // 3.0

typedef char byte;

typedef tip::util::input_iterator_buffer field_buffer;

/**
 * Signature structure, to pass instead of connection string
 */
struct dbalias {
	std::string value;

	void
	swap(std::string& rhs)
	{
		value.swap(rhs);
	}

	operator std::string () { return value; }

	bool
	operator == (dbalias const& rhs) const
	{
		return value == rhs.value;
	}
	bool
	operator != (dbalias const& rhs) const
	{
		return !(*this == rhs);
	}

	bool
	operator < (dbalias const& rhs) const
	{
		return value < rhs.value;
	}
};

inline bool
operator == (dbalias const& lhs, std::string const& rhs)
{
	return lhs.value == rhs;
}

std::ostream&
operator << (std::ostream& out, dbalias const&);

/**
 * Postgre connection options
 */
struct connection_options {
	dbalias alias;
	std::string schema;
	std::string uri;
	std::string database;
	std::string user;
	std::string password;

	void
	generate_alias();
	/**
	 * Parse a connection string
	 * @code{.cpp}
	 * // Full options for a TCP connection
	 * connection_options opts = "aliasname=tcp://user:password@localhost:5432[database]"_pg;
	 * // SSL connection over TCP
	 * opts = "ssl://localhost:5432[database]"_pg;
	 * // Connection via UNIX socket
	 * opts = "socket:///tmp/.s.PGSQL.5432[database]"_pg;
	 * @endcode
	 *
	 */
	static connection_options
	parse(std::string const&);
};

/**
 * Description of a field returned by the backend
 */
struct field_description {
	std::string name;			/**< The field name. */
	int32_t table_oid;			/**< If the field can be identified as a column of a specific table, the object ID of the table; otherwise zero. */
	int16_t attribute_number;	/**< If the field can be identified as a column of a specific table, the attribute number of the column; otherwise zero. */
	int32_t type_oid;			/**< The object ID of the field's data type. */
	int16_t type_size;			/**< The data type size (see pg_type.typlen). Note that negative values denote variable-width types. */
	int32_t type_mod;			/**< The type modifier (see pg_attribute.atttypmod). The meaning of the modifier is type-specific. */
	/**
	 * The format code being used for the field. Currently will be zero (text) or one (binary). In a RowDescription returned from the statement
	 * variant of Describe, the format code is not yet known and will always be zero.
	 */
	int16_t format_code;
};

//@{
/** @name Forward declarations */
struct resultset;
struct connection;
struct db_error;
struct connection_error;
struct query_error;
namespace detail {
struct connection_lock;
}  // namespace detail
typedef std::shared_ptr<detail::connection_lock> connection_lock_ptr;
typedef std::shared_ptr<connection> connection_ptr;
//@}

typedef std::function< void () > simple_callback;
/** Callback for error handling */
typedef std::function< void (db_error const&) > error_callback;
/** Callback for connection acquiring */
typedef std::function< void (connection_lock_ptr) > connection_lock_callback;

/** Callback for query results */
typedef std::function< void (connection_lock_ptr, resultset, bool) > query_result_callback;
typedef std::function< void (query_error const&) > query_error_callback;

typedef std::function< void (connection_ptr) > connection_event_callback;
typedef std::function< void (connection_ptr, connection_error const&) > connection_error_callback;
namespace detail {
/** Callback for internal results passing */
typedef std::function< void (resultset, bool) > internal_result_callback;

}  // namespace detail


}  // namespace pg
}  // namespace db
}  // namespace tip

tip::db::pg::dbalias
operator"" _db(const char*, size_t n);

/**
 * User-defined literal for a PostgreSQL connection string
 * @code{.cpp}
 * // Full options for a TCP connection
 * connection_options opts = "aliasname=tcp://user:password@localhost:5432[database]"_pg;
 * // SSL connection over TCP
 * opts = "ssl://localhost:5432[database]"_pg;
 * // Connection via UNIX socket
 * opts = "socket:///tmp/.s.PGSQL.5432[database]"_pg;
 * @endcode
 */
tip::db::pg::connection_options
operator"" _pg(const char*, size_t);

#endif /* TIP_DB_PG_COMMON_HPP_ */
