/*
 * protocol_io_traits.inl
 *
 *  Created on: 20 июля 2015 г.
 *      Author: brysin
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_PROTOCOL_IO_TRAITS_INL_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_PROTOCOL_IO_TRAITS_INL_

#include <tip/db/pg/protocol_io_traits.hpp>
#include <boost/endian/conversion.hpp>
#include <cassert>

namespace tip {
namespace db {
namespace pg {
namespace detail {

template <typename T>
bool
binary_data_parser<T, INTEGRAL>::operator()(std::istream& is, bool read_size)
{
	std::istream_iterator<char> beg(is);
	std::istream_iterator<char> e;
	std::istream_iterator<char> r = (*this)(std::make_pair(beg, e), read_size);
	return r != beg;
}

template <typename T>
template <typename InputIterator >
InputIterator
binary_data_parser<T, INTEGRAL>::operator()(std::pair<InputIterator, InputIterator> buffer, bool read_size)
{
	typedef std::iterator_traits< InputIterator > iter_traits;
	typedef typename iter_traits::value_type iter_value_type;
	static_assert(std::is_same< iter_value_type, byte >::type::value,
			"Input iterator must be over a char container");
	integer sz = size;
	if (read_size) {
		buffer.first = protocol_parse< BINARY_DATA_FORMAT >(sz)(buffer, false);
	}
	assert( (buffer.second - buffer.first) >= sz && "Buffer size is insufficient" );
	value_type tmp(0);
	char* p = reinterpret_cast<char*>(&tmp);
	char* e = p + sz;
	while (p != e && buffer.first != buffer.second) {
		*p++ = *buffer.first++;
	}
	tmp = boost::endian::big_to_native(tmp);
	std::swap( value, tmp );
	return buffer.first;
}


}  // namespace detail

template < typename InputIterator >
InputIterator
protocol_parser< std::string, BINARY_DATA_FORMAT >::operator ()(std::pair< InputIterator, InputIterator > buffer, bool read_size)
{
	typedef InputIterator iterator_type;
	typedef std::iterator_traits< iterator_type > iter_traits;
	typedef typename iter_traits::value_type iter_value_type;
	static_assert(std::is_same< iter_value_type, byte >::type::value,
			"Input iterator must be over a char container");

	std::string tmp;

}

}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_PROTOCOL_IO_TRAITS_INL_ */
