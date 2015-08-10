/*
 * bytea.hpp
 *
 *  Created on: Aug 7, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_BYTEA_HPP_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_BYTEA_HPP_

#include <tip/db/pg/protocol_io_traits.hpp>

namespace tip {
namespace db {
namespace pg {
namespace io {

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
		return base_type::value.size();
	}
	bool
	operator() (std::istream& in);

	bool
	operator() (buffer_type& buffer);

	template < typename InputIterator >
	InputIterator
	operator()( InputIterator begin, InputIterator end );
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
		return base_type::value.size();
	}
	template < typename InputIterator >
	InputIterator
	operator()( InputIterator begin, InputIterator end );
};

template <>
struct protocol_formatter< bytea, BINARY_DATA_FORMAT > :
		detail::formatter_base< bytea > {

	typedef detail::formatter_base< bytea > base_type;
	typedef base_type::value_type value_type;

	protocol_formatter(bytea const& val) : base_type(val) {}
	size_t
	size() const
	{
		return base_type::value.size();
	}

	bool
	operator() (std::vector<byte>& buffer)
	{
		if (buffer.capacity() - buffer.size() < size()) {
			buffer.reserve(buffer.size() + size());
		}
		std::copy(base_type::value.begin(), base_type::value.end(),
				std::back_inserter(buffer));
		return true;
	}
};

namespace traits {

template <> struct has_parser< bytea, TEXT_DATA_FORMAT > : std::true_type {};
template <> struct has_parser< bytea, BINARY_DATA_FORMAT > : std::true_type {};
template <> struct has_formatter < bytea, BINARY_DATA_FORMAT > : std::true_type {};

static_assert(has_parser<bytea, BINARY_DATA_FORMAT>::value,
                  "Binary data parser for bool");
static_assert(best_parser<bytea>::value == BINARY_DATA_FORMAT,
		"Best parser for bool is binary");

}  // namespace traits

template < typename InputIterator >
InputIterator
protocol_parser< bytea, TEXT_DATA_FORMAT >::operator ()
	(InputIterator begin, InputIterator end)
{
	typedef InputIterator iterator_type;
	typedef std::iterator_traits< iterator_type > iter_traits;
	typedef typename iter_traits::value_type iter_value_type;
	static_assert(std::is_same< iter_value_type, byte >::type::value,
			"Input iterator must be over a char container");
	std::vector<byte> data;

	auto result = detail::bytea_parser().parse(begin, end, std::back_inserter(data));
	if (result.first) {
		base_type::value.swap(data);
		return result.second;
	}
	return begin;
}

template < typename InputIterator >
InputIterator
protocol_parser< bytea, BINARY_DATA_FORMAT >::operator ()
	(InputIterator begin, InputIterator end)
{
	typedef InputIterator iterator_type;
	typedef std::iterator_traits< iterator_type > iter_traits;
	typedef typename iter_traits::value_type iter_value_type;
	static_assert(std::is_same< iter_value_type, byte >::type::value,
			"Input iterator must be over a char container");

	bytea tmp(begin, end);
	std::swap(base_type::value, tmp);

	return end;
}

}  // namespace io
}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_BYTEA_HPP_ */
