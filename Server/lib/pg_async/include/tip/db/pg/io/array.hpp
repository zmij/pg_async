/*
 * array.hpp
 *
 *  Created on: Sep 29, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_ARRAY_HPP_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_ARRAY_HPP_

#include <tip/db/pg/protocol_io_traits.hpp>
#include <tip/db/pg/detail/array_tokenizer.hpp>
#include <tip/db/pg/io/container_to_array.hpp>

#include <array>

namespace tip {
namespace db {
namespace pg {
namespace io {

template < typename T, std::size_t Sz >
struct protocol_formatter< std::array< T, Sz >, TEXT_DATA_FORMAT > :
		detail::text_container_formatter< std::array< T, Sz > > {
	typedef detail::text_container_formatter< std::array< T, Sz > > base_type;
	typedef typename base_type::value_type value_type;

	protocol_formatter(value_type const& v) : base_type(v) {}
};

template < typename T, std::size_t Sz >
struct protocol_parser< std::array< T, Sz >, TEXT_DATA_FORMAT > :
		detail::text_container_parser<
			protocol_parser< std::array< T, Sz >, TEXT_DATA_FORMAT >,
				std::array< T, Sz > > {

	enum {
		array_size = Sz
	};

	typedef detail::text_container_parser<
			protocol_parser< std::array< T, Sz >, TEXT_DATA_FORMAT >,
				std::array< T, Sz > > base_type;
	typedef typename base_type::value_type value_type;
	protocol_parser(value_type& v) : base_type(v) {}

	typename base_type::token_iterator
	tokens_end(typename base_type::tokens_list const& tokens)
	{
		if (tokens.size() <= array_size)
			return tokens.end();
		return tokens.begin() + array_size;
	}

	template < typename InputIterator >
	InputIterator
	operator()(InputIterator begin, InputIterator end)
	{
		return base_type::parse(begin, end, base_type::value.begin());
	}
};

namespace traits {

template < typename T, std::size_t Sz >
struct has_formatter< std::array< T, Sz >, TEXT_DATA_FORMAT > : std::true_type {};
template < typename T, std::size_t Sz >
struct has_parser< std::array< T, Sz >, TEXT_DATA_FORMAT > : std::true_type {};

template < typename T, std::size_t Sz  >
struct cpppg_data_mapping< std::array< T, Sz > > :
	detail::data_mapping_base < oids::type::text, std::array< T, Sz > > {};

}  // namespace traits

}  // namespace io
}  // namespace pg
}  // namespace db
}  // namespace tip


#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_ARRAY_HPP_ */
