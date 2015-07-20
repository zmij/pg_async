/**
 * protocol_io_traits.inl
 *
 *  Created on: 20 июля 2015 г.
 *     @author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_PROTOCOL_IO_TRAITS_INL_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_PROTOCOL_IO_TRAITS_INL_

#include <tip/db/pg/protocol_io_traits.hpp>
#include <boost/endian/conversion.hpp>
#include <algorithm>
#include <cassert>
#include <iterator>

namespace tip {
namespace db {
namespace pg {
namespace detail {

template <typename T>
template <typename InputIterator >
InputIterator
binary_data_parser<T, INTEGRAL>::operator()(InputIterator begin, InputIterator end)
{
	typedef std::iterator_traits< InputIterator > iter_traits;
	typedef typename iter_traits::value_type iter_value_type;
	static_assert(std::is_same< iter_value_type, byte >::type::value,
			"Input iterator must be over a char container");
	assert( (end - begin) >= size && "Buffer size is insufficient" );
	value_type tmp(0);
	char* p = reinterpret_cast<char*>(&tmp);
	char* e = p + size;
	while (p != e && begin != end) {
		*p++ = *begin++;
	}
	tmp = boost::endian::big_to_native(tmp);
	std::swap( base_type::value, tmp );
	return begin;
}


}  // namespace detail

template < typename InputIterator >
InputIterator
protocol_parser< std::string, BINARY_DATA_FORMAT >::operator ()
	(InputIterator begin, InputIterator end)
{
	typedef InputIterator iterator_type;
	typedef std::iterator_traits< iterator_type > iter_traits;
	typedef typename iter_traits::value_type iter_value_type;
	static_assert(std::is_same< iter_value_type, byte >::type::value,
			"Input iterator must be over a char container");

	integer sz = end - begin;

	std::string tmp;
	tmp.reserve(sz);
	for (; begin != end && *begin; ++begin) {
		tmp.push_back(*begin);
	}
	if (!*begin)
		++begin;
	base_type::value.swap(tmp);
	return begin;
}

template < typename InputIterator >
InputIterator
protocol_parser< bool, BINARY_DATA_FORMAT >::operator()
	(InputIterator begin, InputIterator end)
{
	typedef InputIterator iterator_type;
	typedef std::iterator_traits< iterator_type > iter_traits;
	typedef typename iter_traits::value_type iter_value_type;
	static_assert(std::is_same< iter_value_type, byte >::type::value,
			"Input iterator must be over a char container");

	assert( (end - begin) >= size && "Buffer size is insufficient" );
	base_type::value = *begin++;
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

	bytea::container_type tmp(begin, end);
	std::swap(base_type::value.data, tmp);
}

}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_PROTOCOL_IO_TRAITS_INL_ */
