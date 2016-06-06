/*
 * arraytokenizer.hpp
 *
 *  Created on: Sep 28, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_DETAIL_ARRAY_TOKENIZER_HPP_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_DETAIL_ARRAY_TOKENIZER_HPP_

#include <tip/db/pg/detail/tokenizer_base.hpp>

namespace tip {
namespace db {
namespace pg {
namespace io {
namespace detail {

template < typename InputIterator >
class array_tokenizer {
public:
	typedef InputIterator								iterator_type;
	typedef tokenizer_base< InputIterator, '{', '}' >	tokenizer_type;
public:
	template< typename OutputIterator >
	array_tokenizer(iterator_type& begin, iterator_type end, OutputIterator out)
	{
		tokenizer_type(begin, end, out);
	}
};

} /* namespace detail */
}  // namespace io
} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_DETAIL_ARRAY_TOKENIZER_HPP_ */
