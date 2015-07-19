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

enum PROTOCOL_DATA_FORMAT {
	TEXT_DATA_FORMAT = 0,
	BINARY_DATA_FORMAT = 1
};

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

}  // namespace detail

template < typename T, PROTOCOL_DATA_FORMAT >
struct query_parser;

template < typename T, PROTOCOL_DATA_FORMAT >
struct query_formatter;

template < typename T >
struct query_formatter< T, TEXT_DATA_FORMAT > :
		detail::text_data_formatter< T, std::is_enum< typename std::decay< T >::type >::value > {

	typedef detail::text_data_formatter< T, std::is_enum< typename std::decay< T >::type >::value > formatter_base;
	typedef typename formatter_base::value_type value_type;

	query_formatter(value_type const& v) : formatter_base(v) {}
};

template < typename T >
struct query_parser< T, TEXT_DATA_FORMAT > {
	typedef typename std::decay< T >::type value_type;
	typedef tip::util::input_iterator_buffer buffer_type;

	value_type& value;

	query_parser(value_type& v) : value(v) {}

	bool
	operator() (std::istream& in)
	{
		bool result = (in >> value);
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

template < typename T, PROTOCOL_DATA_FORMAT F >
struct protocol_io_traits {
	typedef tip::util::input_iterator_buffer input_buffer_type;
	typedef query_parser< T, F > parser_type;
	typedef query_formatter< T, F > formatter_type;
};

template < PROTOCOL_DATA_FORMAT F, typename T >
typename protocol_io_traits< T, F >::parser_type
query_parse(T& value)
{
	return typename protocol_io_traits< T, F >::parser_type(value);
}

template < PROTOCOL_DATA_FORMAT F, typename T >
typename protocol_io_traits< T, F >::formatter_type
query_format(T const& value)
{
	return typename protocol_io_traits< T, F >::formatter_type(value);
}

template < typename T, PROTOCOL_DATA_FORMAT F >
std::ostream&
operator << (std::ostream& out, query_formatter< T, F > fmt)
{
	std::ostream::sentry s(out);
	if (s) {
		if (!fmt(out))
			out.setstate(std::ios_base::failbit);
	}
	return out;
}

template < typename T, PROTOCOL_DATA_FORMAT F >
std::istream&
operator >> (std::istream& in, query_parser< T, F > parse)
{
	std::istream::sentry s(in);
	if (s) {
		if (!parse(in))
			in.setstate(std::ios_base::failbit);
	}
	return in;
}

template < typename T, PROTOCOL_DATA_FORMAT F >
bool
operator >> (typename protocol_io_traits< T, F >::input_buffer_type& buff, query_parser< T, F > parse)
{
	if (buff.begin() != buff.end())
		return parse(buff);
	return false;
}

/**
 * Query parser specialization for std::string
 */
template < >
struct query_parser< std::string, TEXT_DATA_FORMAT > {
	typedef std::string value_type;
	typedef tip::util::input_iterator_buffer buffer_type;

	value_type& value;

	query_parser(value_type& v) : value(v) {}

	bool
	operator() (std::istream& in);

	bool
	operator() (buffer_type& buffer);
};

/**
 * Query parser specialization for bool
 */
template < >
struct query_parser< bool, TEXT_DATA_FORMAT > {
	typedef bool value_type;
	typedef tip::util::input_iterator_buffer buffer_type;

	value_type& value;

	query_parser(value_type& v) : value(v) {}

	bool
	operator() (std::istream& in);

	bool
	operator() (buffer_type& buffer);
};

/**
 * Query parser specialization for boost::optional (used for nullable types)
 */

template < typename T >
struct query_parser< boost::optional< T >, TEXT_DATA_FORMAT > {
	typedef boost::optional< T > value_type;
	typedef T element_type;
	typedef tip::util::input_iterator_buffer buffer_type;

	value_type& value;

	query_parser(value_type& v) : value(v) {}

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
struct query_parser< bytea, TEXT_DATA_FORMAT > {
	typedef bytea value_type;
	typedef tip::util::input_iterator_buffer buffer_type;

	value_type& value;

	query_parser(value_type& v) : value(v) {}

	bool
	operator() (std::istream& in);

	bool
	operator() (buffer_type& buffer);

};

}  // namespace pg
}  // namespace db
}  // namespace tip


#endif /* TIP_DB_PG_PROTOCOL_IO_TRAITS_HPP_ */
