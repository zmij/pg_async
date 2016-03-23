/**
 * protocol_io_traits.inl
 *
 *  Created on: 20 июля 2015 г.
 *     @author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_PROTOCOL_IO_TRAITS_INL_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_PROTOCOL_IO_TRAITS_INL_

#include <tip/db/pg/protocol_io_traits.hpp>
#include <tip/db/pg/detail/protocol_parsers.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <tip/util/endian.hpp>
#include <algorithm>
#include <cassert>
#include <iterator>

namespace tip {
namespace db {
namespace pg {
namespace io {
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
	typedef decltype (end - begin) difference_type;
	assert( (end - begin) >= (difference_type)size() && "Buffer size is insufficient" );
	value_type tmp(0);
	char* p = reinterpret_cast<char*>(&tmp);
	char* e = p + size();
	while (p != e && begin != end) {
		*p++ = *begin++;
	}
	tmp = util::endian::big_to_native(tmp);
	std::swap( base_type::value, tmp );
	return begin;
}

template < typename T >
bool
binary_data_formatter<T, INTEGRAL>::operator()(std::vector<byte>& buffer)
{
	return (*this)( std::back_inserter(buffer) );
}

template < typename T >
template < typename OutputIterator >
bool
binary_data_formatter< T, INTEGRAL >::operator ()(OutputIterator out)
{
//	typedef OutputIterator iterator_type;
//	typedef std::iterator_traits< iterator_type > iter_traits;
//	typedef typename iter_traits::value_type iter_value_type;
//	static_assert(std::is_same< iter_value_type, byte >::value,
//			"Output iterator must be over a char container");

	T tmp = util::endian::native_to_big(base_type::value);
	char const* p = reinterpret_cast<char const*>(&tmp);
	char const* e = p + size();
	std::copy(p, e, out);
	return true;
}


}  // namespace detail

template < typename T >
template < typename InputIterator >
InputIterator
protocol_parser< T, TEXT_DATA_FORMAT >::operator()
	(InputIterator begin, InputIterator end)
{
	typedef std::iterator_traits< InputIterator > iter_traits;
	typedef typename iter_traits::value_type iter_value_type;
	static_assert(std::is_same< iter_value_type, byte >::type::value,
			"Input iterator must be over a char container");
	boost::iostreams::filtering_istream is(boost::make_iterator_range(begin, end));
	is >> base_type::value;
	begin += size();
	return begin;
}

template < typename InputIterator >
InputIterator
protocol_parser< std::string, TEXT_DATA_FORMAT >::operator ()
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
	if (begin != end && !*begin)
		++begin;
	base_type::value.swap(tmp);
	return begin;
}

template < typename InputIterator >
InputIterator
protocol_parser< bool, TEXT_DATA_FORMAT >::operator()
	(InputIterator begin, InputIterator end)
{
	typedef InputIterator iterator_type;
	typedef std::iterator_traits< iterator_type > iter_traits;
	typedef typename iter_traits::value_type iter_value_type;
	static_assert(std::is_same< iter_value_type, byte >::type::value,
			"Input iterator must be over a char container");

	std::string literal;
	iterator_type tmp = protocol_read< TEXT_DATA_FORMAT >(begin, end, literal);
	if (use_literal(literal)) {
		return tmp;
	}
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

	typedef decltype (end - begin) difference_type;
	assert( (end - begin) >= (difference_type)size() && "Buffer size is insufficient" );
	base_type::value = *begin++;
	return begin;
}

}  // namespace io
}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_PROTOCOL_IO_TRAITS_INL_ */
