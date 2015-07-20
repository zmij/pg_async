/**
 * common.hpp
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

/**
 * @page Datatype conversions
 *	## PostrgreSQL to C++ datatype conversions
 *
 *	### Numeric types
 *	[PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-numeric.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	smallint			| tip::db::pg::smallint
 *	integer				| tip::db::pg::integer
 *	bigint				| tip::db::pg::bigint
 *	decimal				| @todo decide MPFR or GMP - ?
 *	numeric				| @todo decide MPFR or GMP - ?
 *  real				| float
 *  double precision	| double
 *  smallserial			| tip::db::pg::smallint
 *  serial				| tip::db::pg::integer
 *  bigserial			| tip::db::pg::bigint
 *
 *  ### Character types
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-character.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *  varchar(n)			| std::string
 *  character(n)		| std::string
 *  text				| std::string
 *
 *  ### Monetary type
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-money.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	money				| @todo decide MPFR or GMP - ?
 *
 *  ### Binary type
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-binary.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	bytea				| tip::db::pg::bytea
 *
 *  ### Datetime type
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-datetime.html)
 *  [PostgreSQL date/time support](http://www.postgresql.org/docs/9.4/static/datetime-appendix.html)
 *  [Boost.DateTime library](http://www.boost.org/doc/libs/1_58_0/doc/html/date_time.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *  timestamp			|
 *  timestamptz			|
 *  date				|
 *  time				|
 *  time with tz		|
 *  interval			|
 *
 *
 *	### Boolean type
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-boolean.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	boolean				| bool
 *
 *  ### Enumerated types
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-enum.html)
 *
 *  ### Geometric types
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-geometric.html)
 *
 *  ### Network address types
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-net-types.html)
 *
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	cidr				|
 *	inet				| boost::asio::ip::address
 *	macaddr				|
 *
 *  ### Bit String types
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-bit.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	bit(n)				| std::bitset<n>
 *	bit varying(n)		| std::bitset<n> @todo create a signature structure
 *
 *	### Text search types
 *	[PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-textsearch.html)
 *
 *	### UUID type
 *	[PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-uuid.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	uuid				| boost::uuid
 *
 *	### XML type
 *	[PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-xml.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	xml					| std::string
 *
 *	### JSON types
 *	[PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-json.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	json				| std::string
 *
 *	### Arrays
 *	[PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/arrays.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	type array(n)		| std::vector< type mapping >
 *
 *	### Composite types
 *	[PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/rowtypes.html)
 *
 *	### Range types
 *	[PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/rangetypes.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	int4range			|
 *	int8range			|
 *	numrange			|
 *	tsrange				|
 *	tstzrange			|
 *	daterange			|
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

typedef boost::int_t<16>::exact 	smallint;
typedef boost::uint_t<16>::exact	usmallint;
typedef boost::int_t<32>::exact		integer;
typedef boost::uint_t<32>::exact	uinteger;
typedef boost::int_t<64>::exact		bigint;
typedef boost::uint_t<64>::exact	ubigint;

const int32_t PROTOCOL_VERSION = (3 << 16); // 3.0

typedef char byte;

struct bytea {
	std::vector<byte> data;
};

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
	std::string	name;				/**< The field name. */
	integer		table_oid;			/**< If the field can be identified as a column of a specific table, the object ID of the table; otherwise zero. */
	smallint	attribute_number;	/**< If the field can be identified as a column of a specific table, the attribute number of the column; otherwise zero. */
	integer		type_oid;			/**< The object ID of the field's data type. */
	smallint 	type_size;			/**< The data type size (see pg_type.typlen). Note that negative values denote variable-width types. */
	integer		type_mod;			/**< The type modifier (see pg_attribute.atttypmod). The meaning of the modifier is type-specific. */
	/**
	 * The format code being used for the field. Currently will be zero (text) or one (binary). In a RowDescription returned from the statement
	 * variant of Describe, the format code is not yet known and will always be zero.
	 */
	smallint	format_code;
	integer		max_size;			/**< Maximum size of the field in the result set */
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
