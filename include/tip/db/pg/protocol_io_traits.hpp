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

namespace detail {
/**
 * Enumeration for binary parser/formatter template selection
 */
enum protocol_binary_type {
    OTHER,			//!< OTHER Other types, require specialization
	INTEGRAL,		//!< INTEGRAL Integral types, requiring endianness conversion
	FLOATING_POINT,	//!< FLOATING_POINT Floating point types, requiring endianness conversion
};

typedef std::integral_constant< protocol_binary_type, OTHER > other_binary_type;
typedef std::integral_constant< protocol_binary_type, INTEGRAL > integral_binary_type;
typedef std::integral_constant< protocol_binary_type, FLOATING_POINT > floating_point_binary_type;

template < typename T >
struct protocol_binary_selector : other_binary_type {};
template <> struct protocol_binary_selector<smallint> : integral_binary_type {};
template <> struct protocol_binary_selector<usmallint> : integral_binary_type {};
template <> struct protocol_binary_selector<integer> : integral_binary_type {};
template <> struct protocol_binary_selector<uinteger> : integral_binary_type {};
template <> struct protocol_binary_selector<bigint> : integral_binary_type {};
template <> struct protocol_binary_selector<ubigint> : integral_binary_type {};

template <> struct protocol_binary_selector<float> : floating_point_binary_type{};
template <> struct protocol_binary_selector<double> : floating_point_binary_type{};

}  // namespace detail

template < typename T, protocol_data_format >
struct protocol_parser;

template < typename T, protocol_data_format >
struct protocol_formatter;

template < typename T, protocol_data_format F >
struct protocol_io_traits {
	typedef tip::util::input_iterator_buffer input_buffer_type;
	typedef protocol_parser< T, F > parser_type;
	typedef protocol_formatter< T, F > formatter_type;
};

template < protocol_data_format F, typename T >
typename protocol_io_traits< T, F >::parser_type
protocol_read(T& value)
{
	return typename protocol_io_traits< T, F >::parser_type(value);
}

template < protocol_data_format F, typename T, typename InputIterator >
InputIterator
protocol_read(InputIterator begin, InputIterator end, T& value)
{
	return typename protocol_io_traits< T, F >::parser_type(value)(begin, end);
}

template < protocol_data_format F, typename T >
typename protocol_io_traits< T, F >::formatter_type
protocol_writer(T const& value)
{
	return typename protocol_io_traits< T, F >::formatter_type(value);
}

template < protocol_data_format F, typename T >
bool
protocol_write(std::vector<byte>& buffer, T const& value)
{
	return protocol_writer<F>(value)(buffer);
}

template < protocol_data_format F, typename T, typename OutputIterator >
bool
protocol_write(OutputIterator out, T const& value)
{
	return protocol_writer<F>(value)(out);
}

namespace detail {

template < typename T >
struct parser_base {
	typedef typename std::decay< T >::type value_type;
	value_type& value;

	parser_base(value_type& val) : value(val) {}
};
    
template < typename T >
struct formatter_base {
    typedef typename std::decay< T >::type value_type;
    value_type const& value;
    
    formatter_base(value_type const& val) : value(val) {}
};

template < typename T, protocol_binary_type >
struct binary_data_parser;

template < typename T >
struct binary_data_parser < T, INTEGRAL > : parser_base< T > {
	typedef parser_base<T> base_type;
	typedef typename base_type::value_type value_type;

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

template < typename T, protocol_binary_type >
struct binary_data_formatter;

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

template < oids::type::oid_type TypeOid, typename T >
struct data_mapping_base {
	static constexpr oids::type::oid_type type_oid	= TypeOid;
	typedef typename std::decay<T>::type type;
};

}  // namespace detail

namespace traits {

void
register_binary_parser( oids::type::oid_type );
bool
has_binary_parser( oids::type::oid_type );

/**
 * Struct for using for generating wanted data formats from oids
 * Default type mapping falls back to string type and text format
 */
template < oids::type::oid_type TypeOid >
struct pgcpp_data_mapping : detail::data_mapping_base< TypeOid, std::string > {};

/**
 * Template for specifying data types
 */
template < typename T >
struct cpppg_data_mapping : detail::data_mapping_base < oids::type::unknown, T > {};

//@{
/** @name parser and formatter traits */
template <typename T>
struct has_input_operator {
private:
	static void test(...);
	template < typename U > static bool test(U&);
	static std::istream& is;
	static T& val;
public:
	static constexpr bool value = std::is_same<
			decltype( test( std::declval< std::istream& >() >> std::declval< T& >()) ),
			bool
		>::type::value;
};

template <typename T>
struct has_output_operator {
private:
	static void test(...);
	template < typename U > static bool test(U&);
public:
	static constexpr bool value = std::is_same<
			decltype( test( std::declval< std::ostream& >() << std::declval< T >() ) ),
			bool
		>::type::value;
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
struct ___no_inout_test {};
//@}

/**
 * Template parser selector
 */
template < typename T >
struct best_parser {
private:
	static constexpr bool has_binary_parser = has_parser<T, BINARY_DATA_FORMAT>::value;
public:
	static constexpr protocol_data_format value = has_binary_parser ? BINARY_DATA_FORMAT : TEXT_DATA_FORMAT;
	typedef protocol_parser< T, value > type;
};

/**
 * Template formatter selector
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
    operator()(std::istream& out)
    {
        out << base_type::value;
        return out.good();
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
	bool
	operator() (std::istream& in)
	{
		in >> base_type::value;
		bool result = !in.fail();
		if (!result)
			in.setstate(std::ios_base::failbit);
		return result;
	}

	bool
	operator() (buffer_type& buffer)
	{
		std::istream in(&buffer);
		return (*this)(in);
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

	typedef tip::util::input_iterator_buffer buffer_type;

	protocol_parser(value_type& v) : base_type(v) {}

	size_t
	size() const
	{
		return base_type::value.size();
	}
	bool
	operator() (std::istream& in);

	bool
	operator() (buffer_type& buffer);

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
    operator()(std::vector<char>& buffer)
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
	operator() (std::istream& in);

	bool
	operator() (buffer_type& buffer);
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

/**
 * @brief Protocol parser specialization for bytea (binary string), text data format
 */
template <>
struct protocol_parser< bytea, TEXT_DATA_FORMAT > :
		detail::parser_base< bytea > {
	typedef detail::parser_base< bytea > base_type;
	typedef base_type::value_type value_type;
	typedef tip::util::input_iterator_buffer buffer_type;

	protocol_parser(value_type& v) : base_type(v) {}

	size_t
	size() const
	{
		return base_type::value.data.size();
	}
	bool
	operator() (std::istream& in);

	bool
	operator() (buffer_type& buffer);
};

/**
 * @brief Protocol parser specialization for bytea (binary string), binary data format
 */
template <>
struct protocol_parser< bytea, BINARY_DATA_FORMAT > :
		detail::parser_base< bytea > {
	typedef detail::parser_base< bytea > base_type;
	typedef base_type::value_type value_type;

	protocol_parser(value_type& val) : base_type(val) {}
	size_t
	size() const
	{
		return base_type::value.data.size();
	}
	template < typename InputIterator >
	InputIterator
	operator()( InputIterator begin, InputIterator end );
};

}  // namespace pg
}  // namespace db
}  // namespace tip

#include <tip/db/pg/protocol_io_traits.inl>
#include <tip/db/pg/datatype_mapping.hpp>

#endif /* TIP_DB_PG_PROTOCOL_IO_TRAITS_HPP_ */
