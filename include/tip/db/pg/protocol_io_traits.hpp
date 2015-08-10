/**
 * protocol_traits.hpp
 *
 *  Created on: Jul 19, 2015
 *      Author: zmij
 */

#ifndef TIP_DB_PG_PROTOCOL_IO_TRAITS_HPP_
#define TIP_DB_PG_PROTOCOL_IO_TRAITS_HPP_

#include <string>

#include <istream>
#include <ostream>
#include <sstream>

#include <type_traits>
#include <boost/optional.hpp>

#include <tip/db/pg/common.hpp>
#include <tip/db/pg/pg_types.hpp>
#include <tip/util/streambuf.hpp>

namespace tip {
namespace db {
namespace pg {
namespace io {

/**
 * @brief Enumeration for binary parser/formatter template selection
 */
enum protocol_binary_type {
    OTHER,			//!< OTHER Other types, require specialization
	INTEGRAL,		//!< INTEGRAL Integral types, requiring endianness conversion
	FLOATING_POINT,	//!< FLOATING_POINT Floating point types, requiring endianness conversion
};

namespace detail {
/** @brief Types other then integral or floating point.
 * Require explicit specialization of a binary data parser
 */
typedef std::integral_constant< protocol_binary_type, OTHER > other_binary_type;
/** @brief Integral datatypes.
 * Selects binary parser specialization with network byte order conversion
 */
typedef std::integral_constant< protocol_binary_type, INTEGRAL > integral_binary_type;
/** @brief Floating point datatypes.
 * Binary parser is not implemented yet.
 * @todo Implement binary parser for floating point values
 */
typedef std::integral_constant< protocol_binary_type, FLOATING_POINT > floating_point_binary_type;

/**
 * @brief Metafunction for specifying binary procotocol type
 */
template < typename T >
struct protocol_binary_selector : other_binary_type {};
//@{
/** @name Protocol selectors for integral types */
template <> struct protocol_binary_selector<smallint> : integral_binary_type {};
template <> struct protocol_binary_selector<usmallint> : integral_binary_type {};
template <> struct protocol_binary_selector<integer> : integral_binary_type {};
template <> struct protocol_binary_selector<uinteger> : integral_binary_type {};
template <> struct protocol_binary_selector<bigint> : integral_binary_type {};
template <> struct protocol_binary_selector<ubigint> : integral_binary_type {};
//@}

//@{
/** @name Protocol selectors for floating point types */
template <> struct protocol_binary_selector<float> : floating_point_binary_type{};
template <> struct protocol_binary_selector<double> : floating_point_binary_type{};
//@}

}  // namespace detail

template < typename T, protocol_data_format >
struct protocol_parser;

template < typename T, protocol_data_format >
struct protocol_formatter;

/**
 * @brief I/O Traits structure
 * @tparam T data type for input/output
 * @tparam F data format
 */
template < typename T, protocol_data_format F >
struct protocol_io_traits {
	typedef tip::util::input_iterator_buffer input_buffer_type;
	typedef protocol_parser< T, F > parser_type;
	typedef protocol_formatter< T, F > formatter_type;
};

/**
 * @brief Helper function to create a protocol parser
 *
 * Deduces datatype by the function argument and returns a data parser
 * using protocol_io_traits.
 *
 * @param value
 * @return
 */
template < protocol_data_format F, typename T >
typename protocol_io_traits< T, F >::parser_type
protocol_reader(T& value)
{
	return typename protocol_io_traits< T, F >::parser_type(value);
}

/**
 * @brief Read value from input buffer
 *
 * Deduces a parser using protocol_io_traits for the type and protocol and uses
 * it to read a value from the buffer specified by the pair of iterators
 *
 * @param begin Iterator to start of buffer
 * @param end Iterator beyond the end of buffer
 * @param value variable to read into
 * @return	iterator after the value. If the iterator returned is equal to the
 * 			begin iterator, nothing has been read and it means an error.
 * @tparam F Protocol data format
 * @tparam T Data type to read
 * @tparam InputIterator Buffer iterator type
 */
template < protocol_data_format F, typename T, typename InputIterator >
InputIterator
protocol_read(InputIterator begin, InputIterator end, T& value)
{
	return typename protocol_io_traits< T, F >::parser_type(value)(begin, end);
}

/**
 * @brief Helper function to create a protocol formatter
 *
 * Deduces datatype by the function argument and returns a data formatter
 * using protocol_io_traits.
 *
 * @param value
 * @return
 */
template < protocol_data_format F, typename T >
typename protocol_io_traits< T, F >::formatter_type
protocol_writer(T const& value)
{
	return typename protocol_io_traits< T, F >::formatter_type(value);
}

/**
 * @brief Write value to a buffer
 *
 * Deduces a formatter using protocol_io_traits for the type and protocol and
 * uses it to write the value to the buffer
 *
 * @param buffer target data buffer
 * @param value
 * @return true in case of success, false in case of failure
 * @tparam F data format
 * @tparam T data type
 */
template < protocol_data_format F, typename T >
bool
protocol_write(std::vector<byte>& buffer, T const& value)
{
	return protocol_writer<F>(value)(buffer);
}

/**
 * @brief Write a value to a buffer using an output iterator
 *
 * Deduces a formatter using protocol_io_traits for the type and protocol and
 * uses it to write the value to the output iterator.
 *
 * @param out output iterator
 * @param value value to output
 * @return true in case of success, false in case of failure
 */
template < protocol_data_format F, typename T, typename OutputIterator >
bool
protocol_write(OutputIterator out, T const& value)
{
	return protocol_writer<F>(value)(out);
}

namespace detail {

/**
 * @brief Base template struct for a data parser
 * @tparam T type of value to parse
 */
template < typename T >
struct parser_base {
	typedef typename std::decay< T >::type value_type;
	value_type& value;

	parser_base(value_type& val) : value(val) {}
};
    
/**
 * @brief Base template struct for a data formatter
 * @tparam T type of value to format
 */
template < typename T >
struct formatter_base {
    typedef typename std::decay< T >::type value_type;
    value_type const& value;
    
    formatter_base(value_type const& val) : value(val) {}
};

/**
 * @brief Base structure for a binary data parser.
 * Has no definition.
  * @tparam T type of value to parse
  * @tparam TYPE selector for the type
 */
template < typename T, protocol_binary_type TYPE >
struct binary_data_parser;

/**
 * @brief Specification of a binary parser for integral values
 *
 * Supports @ref tip::db::pg::smallint, @ref tip::db::pg::integer,
 * @ref tip::db::pg::bigint and their unsigned variants
 * @tparam T integral data type
 */
template < typename T >
struct binary_data_parser < T, INTEGRAL > : parser_base< T > {
	typedef parser_base<T> base_type;
	typedef typename base_type::value_type value_type;

	/**
	 * @brief data size
	 */
	size_t
	size() const
	{
		return sizeof(T);
	}

	binary_data_parser(value_type& val) : base_type(val) {}

	template < typename InputIterator >
	InputIterator
	operator()( InputIterator begin, InputIterator end );
};

template < typename T >
struct binary_data_parser < T, OTHER >;

/**
 * @brief Base structure for a binary data formatter.
 * Has no definition.
  * @tparam T type of value to parse
  * @tparam TYPE selector for the type
 */
template < typename T, protocol_binary_type TYPE >
struct binary_data_formatter;

/**
 * @brief Specification of a binary formatter for integral values
 *
 * Supports @ref tip::db::pg::smallint, @ref tip::db::pg::integer,
 * @ref tip::db::pg::bigint and their unsigned variants
 * @tparam T integral data type
 */
template < typename T >
struct binary_data_formatter < T, INTEGRAL > : formatter_base< T > {
	typedef formatter_base< T > base_type;
	typedef typename base_type::value_type value_type;

	size_t
	size() const
	{
		return sizeof(T);
	}

	binary_data_formatter(value_type const& val) : base_type(val) {}

	bool
	operator()(std::vector<byte>& buffer);

	template < typename OutputIterator >
	bool
	operator()(OutputIterator);
};

/**
 * @brief Base structure for specifying mapping between C++ data type and
 * 		  PostgreSQL type oid.
 */
template < oids::type::oid_type TypeOid, typename T >
struct data_mapping_base {
	static constexpr oids::type::oid_type type_oid	= TypeOid;
	typedef typename std::decay<T>::type type;
};

}  // namespace detail

namespace traits {

/**
 * @brief Mark a type oid that it has a binary parser, so that pg_async
 * 		  can request data in binary format.
 * @param id PostgreSQL type oid.
 */
void
register_binary_parser( oids::type::oid_type id );

/**
 * @brief Check if there is a binary parser for the specified oid
 * @param id PostgreSQL type oid.
 * @return
 */
bool
has_binary_parser( oids::type::oid_type id );

/**
 * Struct for using for generating wanted data formats from oids
 * Default type mapping falls back to string type and text format
 */
template < oids::type::oid_type TypeOid >
struct pgcpp_data_mapping : detail::data_mapping_base< TypeOid, std::string > {};

/**
 * @brief Template for specifying mapping between a C++ type and PostgreSQL
 * 		  type oid
 *
 * Default mapping is unknown and will lead to a compilation error when a type
 * is used as a parameter for a prepared statement
 */
template < typename T >
struct cpppg_data_mapping : detail::data_mapping_base < oids::type::unknown, T > {};

//@{
/** @name parser and formatter traits */
struct __io_meta_function_helper {
	template <typename T> __io_meta_function_helper(T const&);
};

std::false_type
operator << (std::ostream const&, __io_meta_function_helper const&);
std::false_type
operator >> (std::istream const&, __io_meta_function_helper const&);

template <typename T>
struct has_input_operator {
private:
	static std::false_type test(std::false_type);
	static std::true_type test(std::istream&);

	static std::istream& is;
	static T& val;
public:
	static constexpr bool value = std::is_same<
			decltype( test( is >> val ) ), std::true_type >::type::value;
};

template <typename T>
struct has_output_operator {
private:
	static std::false_type test(std::false_type);
	static std::true_type test(std::ostream&);

	static std::ostream& os;
	static T const& val;
public:
	static constexpr bool value = std::is_same<
			decltype( test( os << val) ), std::true_type >::type::value;
};

template < typename T, protocol_data_format format >
struct has_parser : std::false_type {};
template < typename T >
struct has_parser< T, TEXT_DATA_FORMAT >
	: std::integral_constant< bool, has_input_operator< T >::value > {};
template < > struct has_parser< smallint, BINARY_DATA_FORMAT > : std::true_type {};
template < > struct has_parser< integer, BINARY_DATA_FORMAT > : std::true_type {};
template < > struct has_parser< bigint, BINARY_DATA_FORMAT > : std::true_type {};

template < typename T, protocol_data_format format >
struct has_formatter : std::false_type {};
template < typename T >
struct has_formatter< T, TEXT_DATA_FORMAT >
	: std::integral_constant< bool, has_output_operator< T >::value > {};
template < > struct has_formatter< smallint, BINARY_DATA_FORMAT > : std::true_type {};
template < > struct has_formatter< integer, BINARY_DATA_FORMAT > : std::true_type {};
template < > struct has_formatter< bigint, BINARY_DATA_FORMAT > : std::true_type {};
//@}

//@{
/** @name IO metafunctions tests */
struct ___no_inout_test {};

static_assert(has_input_operator<___no_inout_test>::value == false,
		"Input operator test OK");
static_assert(has_output_operator<___no_inout_test>::value == false,
		"Output operator test OK");

static_assert(has_parser<___no_inout_test, TEXT_DATA_FORMAT>::value == false,
		"Text parser test is OK");
static_assert(has_formatter<___no_inout_test, TEXT_DATA_FORMAT>::value == false,
		"Text formatter test is OK");
//@}

/**
 * @brief Template parser selector
 */
template < typename T >
struct best_parser {
private:
	static constexpr bool has_binary_parser = has_parser<T, BINARY_DATA_FORMAT>::value;
public:
	static constexpr protocol_data_format value = has_binary_parser ?
			BINARY_DATA_FORMAT : TEXT_DATA_FORMAT;
	typedef protocol_parser< T, value > type;
};

/**
 * @brief Template formatter selector
 */
template < typename T >
struct best_formatter {
private:
	static constexpr bool has_binary_formatter = has_formatter< T, BINARY_DATA_FORMAT >::value;
public:
	static constexpr protocol_data_format value = has_binary_formatter ? BINARY_DATA_FORMAT : TEXT_DATA_FORMAT;
	typedef protocol_formatter< T, value > type;
};

//@{
/** @name checks for integral types */
static_assert(has_parser<smallint, TEXT_DATA_FORMAT>::value,
		"Text format parser for smallint");
static_assert(has_parser<smallint, BINARY_DATA_FORMAT>::value,
		"Binary format parser for smallint");
static_assert(best_parser< smallint >::value == BINARY_DATA_FORMAT,
		"Best parser for smallint is binary");

static_assert(has_formatter<smallint, TEXT_DATA_FORMAT>::value,
		"Text format writer for smallint");
static_assert(has_formatter<smallint, BINARY_DATA_FORMAT>::value,
		"Binary format writer for smallint");
static_assert(best_formatter< smallint >::value == BINARY_DATA_FORMAT,
		"Best writer for smallint is binary");

static_assert(has_parser<integer, TEXT_DATA_FORMAT>::value,
		"Text format parser for integer");
static_assert(has_parser<integer, BINARY_DATA_FORMAT>::value,
		"Binary format parser for integer");
static_assert(best_parser< integer >::value == BINARY_DATA_FORMAT,
		"Best parser for integer is binary");

static_assert(has_formatter<integer, TEXT_DATA_FORMAT>::value,
		"Text format writer for integer");
static_assert(has_formatter<integer, BINARY_DATA_FORMAT>::value,
		"Binary format writer for integer");
static_assert(best_formatter< integer >::value == BINARY_DATA_FORMAT,
		"Best writer for integer is binary");

static_assert(has_parser<bigint, TEXT_DATA_FORMAT>::value,
		"Text format parser for bigint");
static_assert(has_parser<bigint, BINARY_DATA_FORMAT>::value,
		"Binary format parser for bigint");
static_assert(best_parser< bigint >::value == BINARY_DATA_FORMAT,
		"Best parser for bigint is binary");

static_assert(has_formatter<bigint, TEXT_DATA_FORMAT>::value,
		"Text format writer for bigint");
static_assert(has_formatter<bigint, BINARY_DATA_FORMAT>::value,
		"Binary format writer for bigint");
static_assert(best_formatter< bigint >::value == BINARY_DATA_FORMAT,
		"Best writer for bigint is binary");
//@}
//@{
/** @name checks for floating-point types */
static_assert(has_parser<float, TEXT_DATA_FORMAT>::value,
		"Text format parser for float");
// @todo implement binary parser for floats
//static_assert(has_parser<float, BINARY_DATA_FORMAT>::value,
//		"Binary format parser for float");
static_assert(best_parser< float >::value == TEXT_DATA_FORMAT,
		"Best parser for float is text");

static_assert(has_formatter<float, TEXT_DATA_FORMAT>::value,
		"Text format writer for float");
// @todo implement binary formatter for floats
//static_assert(has_formatter<float, BINARY_DATA_FORMAT>::value,
//		"Binary format writer for float");
static_assert(best_formatter< float >::value == TEXT_DATA_FORMAT,
		"Best writer for float is text");

static_assert(has_parser<double, TEXT_DATA_FORMAT>::value,
		"Text format parser for double");
// @todo implement binary parser for doubles
//static_assert(has_parser<double, BINARY_DATA_FORMAT>::value,
//		"Binary format parser for double");
static_assert(best_parser< double >::value == TEXT_DATA_FORMAT,
		"Best parser for double is text");

static_assert(has_formatter<double, TEXT_DATA_FORMAT>::value,
		"Text format writer for double");
// @todo implement binary formatter for doubles
//static_assert(has_formatter<double, BINARY_DATA_FORMAT>::value,
//		"Binary format writer for double");
static_assert(best_formatter< double >::value == TEXT_DATA_FORMAT,
		"Best writer for double is text");
//@}

}  // namespace traits

/**
 * @brief Generic implementation of a formatter for text data format.
 */
template < typename T >
struct protocol_formatter< T, TEXT_DATA_FORMAT > : detail::formatter_base< T > {
    typedef detail::formatter_base< T > base_type;
    typedef typename base_type::value_type value_type;
    
    protocol_formatter(value_type const& val) : base_type(val) {}
    
    size_t
	size() const
    {
    	std::ostringstream os;
    	os << base_type::value;
    	return os.str().size();
    }
    bool
    operator()(std::vector<char>& buffer)
    {
    	std::ostringstream os;
    	os << base_type::value;
        auto str = os.str();
    	std::copy(str.begin(), str.end(), std::back_inserter(buffer));
    	return true;
    }
};

/**
 * Default parser for text data format implementation
 */
template < typename T >
struct protocol_parser< T, TEXT_DATA_FORMAT > : detail::parser_base< T > {
	typedef detail::parser_base< T > base_type;
	typedef typename base_type::value_type value_type;

	typedef tip::util::input_iterator_buffer buffer_type;

	protocol_parser(value_type& v) : base_type(v) {}

	size_t
	size() const
	{
		return sizeof(T);
	}

	template < typename InputIterator >
    InputIterator
    operator()(InputIterator begin, InputIterator end);
};

template < typename T >
struct protocol_formatter < T, BINARY_DATA_FORMAT > :
	detail::binary_data_formatter< T,
		detail::protocol_binary_selector< typename std::decay<T>::type >::value > {

	typedef detail::binary_data_formatter< T,
			detail::protocol_binary_selector< typename std::decay<T>::type >::value > formatter_base;
	typedef typename formatter_base::value_type value_type;

	protocol_formatter(value_type const& val) : formatter_base(val) {}
};

template < typename T >
struct protocol_parser< T, BINARY_DATA_FORMAT > :
	detail::binary_data_parser< T,
		detail::protocol_binary_selector< typename std::decay<T>::type >::value > {

	typedef detail::binary_data_parser< T,
			detail::protocol_binary_selector< typename std::decay<T>::type >::value > parser_base;
	typedef typename parser_base::value_type value_type;

	protocol_parser(value_type& val) : parser_base(val) {}
};

/**
 * @brief Protocol parser specialization for std::string, text data format
 */
template < >
struct protocol_parser< std::string, TEXT_DATA_FORMAT > :
		detail::parser_base< std::string > {
	typedef detail::parser_base< std::string > base_type;
	typedef base_type::value_type value_type;

	protocol_parser(value_type& v) : base_type(v) {}

	size_t
	size() const
	{
		return base_type::value.size();
	}

    template < typename InputIterator >
    InputIterator
    operator()(InputIterator begin, InputIterator end);
};

namespace traits {
static_assert(has_parser<std::string, TEXT_DATA_FORMAT>::value,
              "Text data parser for std::string");
static_assert(!has_parser<std::string, BINARY_DATA_FORMAT>::value,
               "No binary data parser for std::string");
static_assert(best_parser<std::string>::value == TEXT_DATA_FORMAT,
		"Best parser for std::string is binary");
}  // namespace traits

template < >
struct protocol_formatter< std::string, TEXT_DATA_FORMAT > :
		detail::formatter_base< std::string > {
	typedef detail::formatter_base< std::string > base_type;
	typedef base_type::value_type value_type;

	protocol_formatter(value_type const& v) : base_type(v) {}

   size_t
	size() const
    {
    	return base_type::value.size();
    }
    bool
    operator()(std::ostream& out)
    {
        out << base_type::value;
        return out.good();
    }
    bool
    operator()(std::vector<byte>& buffer)
    {
    	auto iter = std::copy(base_type::value.begin(), base_type::value.end(),
    			std::back_inserter(buffer));
    	return true;
    }

};

namespace traits {
static_assert(has_formatter<std::string, TEXT_DATA_FORMAT>::value,
              "Text data parser for std::string");
static_assert(!has_formatter<std::string, BINARY_DATA_FORMAT>::value,
              "No binary data parser for std::string");
static_assert(best_formatter<std::string>::value == TEXT_DATA_FORMAT,
		"Best parser for std::string is binary");
}  // namespace traits

/**
 * @brief Protocol parser specialization for bool, text data format
 */
template < >
struct protocol_parser< bool, TEXT_DATA_FORMAT > :
		detail::parser_base< bool > {
	typedef detail::parser_base< bool > base_type;
	typedef base_type::value_type value_type;

	typedef tip::util::input_iterator_buffer buffer_type;

	protocol_parser(value_type& v) : base_type(v) {}

	size_t
	size() const
	{
		return sizeof(bool);
	}
	bool
	use_literal(std::string const& l);

    template < typename InputIterator >
    InputIterator
    operator()(InputIterator begin, InputIterator end);

};

/**
 * @brief Protocol parser specialization for bool, binary data format
 */
template < >
struct protocol_parser< bool, BINARY_DATA_FORMAT > :
			detail::parser_base< bool > {
	typedef detail::parser_base< bool > base_type;
	typedef base_type::value_type value_type;
	typedef tip::util::input_iterator_buffer buffer_type;

	protocol_parser(value_type& v) : base_type(v) {}
	size_t
	size() const
	{
		return sizeof(bool);
	}
	template < typename InputIterator >
	InputIterator
	operator()( InputIterator begin, InputIterator end );
};

namespace traits {
template < > struct has_parser< bool, BINARY_DATA_FORMAT > : std::true_type {};
static_assert(has_parser<bool, TEXT_DATA_FORMAT>::value,
                  "Text data parser for bool");
static_assert(has_parser<bool, BINARY_DATA_FORMAT>::value,
                  "Binary data parser for bool");
static_assert(best_parser<bool>::value == BINARY_DATA_FORMAT,
		"Best parser for bool is binary");
}  // namespace traits

/**
 * @brief Protocol parser specialization for boost::optional (used for nullable types), text data format
 */
template < typename T >
struct protocol_parser< boost::optional< T >, TEXT_DATA_FORMAT > :
		detail::parser_base< boost::optional< T > > {
	typedef detail::parser_base< boost::optional< T > > base_type;
	typedef typename base_type::value_type value_type;

	typedef T element_type;
	typedef tip::util::input_iterator_buffer buffer_type;

	protocol_parser(value_type& v) : base_type(v) {}

	bool
	operator() (std::istream& in)
	{
		element_type tmp;
		if (query_parse(tmp)(in)) {
			base_type::value = value_type(tmp);
		} else {
			base_type::value = value_type();
		}
		return true;
	}

	bool
	operator() (buffer_type& buffer)
	{
		element_type tmp;
		if (query_parse(tmp)(buffer)) {
			base_type::value = value_type(tmp);
		} else {
			base_type::value = value_type();
		}
		return true;
	}

	template < typename InputIterator >
	InputIterator
	operator()(InputIterator begin, InputIterator end);
};

/**
 * @brief Protocol parser specialization for boost::optional (used for nullable types), binary data format
 */
template < typename T >
struct protocol_parser< boost::optional< T >, BINARY_DATA_FORMAT > :
		detail::parser_base< boost::optional< T > > {
	typedef detail::parser_base< boost::optional< T > > base_type;
	typedef typename base_type::value_type value_type;

	typedef T element_type;
	typedef protocol_parser< element_type, BINARY_DATA_FORMAT > element_parser;
	typedef tip::util::input_iterator_buffer buffer_type;

	protocol_parser(value_type& v) : base_type(v) {}

	size_t
	size() const
	{
		if (base_type::value)
			return element_parser(*base_type::value).size();
		return 0;
	}

	template < typename InputIterator >
	InputIterator
	operator()( InputIterator begin, InputIterator end )
	{
		T tmp;
		InputIterator c = protocol_read< BINARY_DATA_FORMAT >(begin, end, tmp);
		if (c != begin) {
			base_type::value = value_type(tmp);
		} else {
			base_type::value = value_type();
		}
		return c;
	}
};

}  // namespace io
}  // namespace pg
}  // namespace db
}  // namespace tip

#include <tip/db/pg/protocol_io_traits.inl>
// Common datatypes implementation includes
#include <tip/db/pg/io/bytea.hpp>
#include <tip/db/pg/datatype_mapping.hpp>

#endif /* TIP_DB_PG_PROTOCOL_IO_TRAITS_HPP_ */
