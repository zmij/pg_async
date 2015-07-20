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

#include <type_traits>
#include <boost/optional.hpp>

#include <tip/db/pg/common.hpp>
#include <tip/util/streambuf.hpp>

namespace tip {
namespace db {
namespace pg {

/**
 * Protocol format type
 */
enum protocol_data_format {
	TEXT_DATA_FORMAT = 0, //!< TEXT_DATA_FORMAT
	BINARY_DATA_FORMAT = 1//!< BINARY_DATA_FORMAT
};

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
protocol_parse(T& value)
{
	return typename protocol_io_traits< T, F >::parser_type(value);
}

template < protocol_data_format F, typename T >
typename protocol_io_traits< T, F >::formatter_type
protocol_format(T const& value)
{
	return typename protocol_io_traits< T, F >::formatter_type(value);
}

namespace detail {

template <typename T, bool need_quotes>
struct text_data_formatter;

const char QUOTE = '\'';

template < typename T >
struct text_data_formatter < T, true > {
	typedef typename std::decay< T >::type value_type;
	value_type const& value;

	explicit text_data_formatter(value_type const& v)
		: value(v)
	{}

	bool
	operator () ( std::ostream& out ) const
	{
		bool result = (out << QUOTE << value << QUOTE);
		return result;
	}

	template < typename BufferOutputIterator >
	bool
	operator() (BufferOutputIterator i) const;
};

template < typename T >
struct text_data_formatter< T, false > {
	typedef T value_type;
	value_type const& value;

	explicit text_data_formatter(value_type const& v)
		: value(v)
	{}

	bool
	operator () ( std::ostream& out ) const
	{
		bool result = (out << value);
		return result;
	}

	template < typename BufferOutputIterator >
	bool
	operator() (BufferOutputIterator i) const;
};

template < typename T, protocol_binary_type >
struct binary_data_parser;

template < typename T >
struct binary_data_parser < T, INTEGRAL > {
	typedef T value_type;
	typedef tip::util::input_iterator_buffer buffer_type;

	enum {
		size	= sizeof(T)
	};
	value_type& value;

	binary_data_parser(value_type& val) : value(val) {}

	bool
	operator()( std::istream&, bool read_size );
	buffer_type::const_iterator
	operator()( buffer_type& buffer, bool read_size )
	{
		return (*this)( std::make_pair( buffer.begin(), buffer.end() ), read_size );
	}
	template < typename InputIterator >
	InputIterator
	operator()( std::pair< InputIterator, InputIterator > buffer, bool read_size );
};

}  // namespace detail



template < typename T >
struct protocol_formatter< T, TEXT_DATA_FORMAT > :
		detail::text_data_formatter< T, std::is_enum< typename std::decay< T >::type >::value > {

	typedef detail::text_data_formatter< T, std::is_enum< typename std::decay< T >::type >::value > formatter_base;
	typedef typename formatter_base::value_type value_type;

	protocol_formatter(value_type const& v) : formatter_base(v) {}
};

template < typename T >
struct protocol_parser< T, TEXT_DATA_FORMAT > {
	typedef typename std::decay< T >::type value_type;
	typedef tip::util::input_iterator_buffer buffer_type;

	value_type& value;

	protocol_parser(value_type& v) : value(v) {}

	bool
	operator() (std::istream& in)
	{
		in >> value;
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

template < typename T, protocol_data_format F >
std::ostream&
operator << (std::ostream& out, protocol_formatter< T, F > fmt)
{
	std::ostream::sentry s(out);
	if (s) {
		if (!fmt(out))
			out.setstate(std::ios_base::failbit);
	}
	return out;
}

template < typename T, protocol_data_format F >
std::istream&
operator >> (std::istream& in, protocol_parser< T, F > parse)
{
	std::istream::sentry s(in);
	if (s) {
		if (!parse(in))
			in.setstate(std::ios_base::failbit);
	}
	return in;
}

template < typename T, protocol_data_format F >
bool
operator >> (typename protocol_io_traits< T, F >::input_buffer_type& buff, protocol_parser< T, F > parse)
{
	if (buff.begin() != buff.end())
		return parse(buff);
	return false;
}

/**
 * Protocol parser specialization for std::string
 */
template < >
struct protocol_parser< std::string, TEXT_DATA_FORMAT > {
	typedef std::string value_type;
	typedef tip::util::input_iterator_buffer buffer_type;

	value_type& value;

	protocol_parser(value_type& v) : value(v) {}

	bool
	operator() (std::istream& in);

	bool
	operator() (buffer_type& buffer);
};

template < >
struct protocol_parser< std::string, BINARY_DATA_FORMAT > {
	typedef std::string value_type;
	typedef tip::util::input_iterator_buffer buffer_type;

	value_type& value;

	protocol_parser(value_type& v) : value(v) {}

	bool
	operator()(std::istream& in, bool read_size);

	buffer_type::const_iterator
	operator()(buffer_type& buffer, bool read_size);

	template < typename InputIterator >
	InputIterator
	operator()( std::pair< InputIterator, InputIterator > buffer, bool read_size );
};

/**
 * Query parser specialization for bool
 */
template < >
struct protocol_parser< bool, TEXT_DATA_FORMAT > {
	typedef bool value_type;
	typedef tip::util::input_iterator_buffer buffer_type;

	value_type& value;

	protocol_parser(value_type& v) : value(v) {}

	bool
	operator() (std::istream& in);

	bool
	operator() (buffer_type& buffer);
};

/**
 * Query parser specialization for boost::optional (used for nullable types)
 */

template < typename T >
struct protocol_parser< boost::optional< T >, TEXT_DATA_FORMAT > {
	typedef boost::optional< T > value_type;
	typedef T element_type;
	typedef tip::util::input_iterator_buffer buffer_type;

	value_type& value;

	protocol_parser(value_type& v) : value(v) {}

	bool
	operator() (std::istream& in)
	{
		element_type tmp;
		if (query_parse(tmp)(in)) {
			value = value_type(tmp);
		} else {
			value = value_type();
		}
		return true;
	}

	bool
	operator() (buffer_type& buffer)
	{
		element_type tmp;
		if (query_parse(tmp)(buffer)) {
			value = value_type(tmp);
		} else {
			value = value_type();
		}
		return true;
	}
};

template <>
struct protocol_parser< bytea, TEXT_DATA_FORMAT > {
	typedef bytea value_type;
	typedef tip::util::input_iterator_buffer buffer_type;

	value_type& value;

	protocol_parser(value_type& v) : value(v) {}

	bool
	operator() (std::istream& in);

	bool
	operator() (buffer_type& buffer);

};

}  // namespace pg
}  // namespace db
}  // namespace tip

#include <tip/db/pg/protocol_io_traits.inl>

#endif /* TIP_DB_PG_PROTOCOL_IO_TRAITS_HPP_ */
