/*
 * vector.hpp
 *
 *  Created on: Sep 28, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_VECTOR_HPP_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_VECTOR_HPP_

#include <tip/db/pg/protocol_io_traits.hpp>
#include <tip/db/pg/io/container_to_array.hpp>

#include <vector>

namespace tip {
namespace db {
namespace pg {
namespace io {

/**
 * @brief Protocol format specialization for std::vector, mapping to postgre array
 */
template < typename T >
struct protocol_formatter< std::vector< T >, TEXT_DATA_FORMAT > :
		detail::text_container_formatter< std::vector< T > > {

	typedef detail::text_container_formatter< std::vector< T > > base_type;
	typedef typename base_type::value_type value_type;

	protocol_formatter(value_type const& v) : base_type(v) {}
};

template < typename T >
struct protocol_parser< std::vector< T >, TEXT_DATA_FORMAT > :
		detail::text_container_parser<
			protocol_parser< std::vector< T >, TEXT_DATA_FORMAT >,
				std::vector< T > > {

	typedef detail::text_container_parser<
			protocol_parser< std::vector< T >, TEXT_DATA_FORMAT >,
				std::vector< T > > base_type;
	typedef typename base_type::value_type value_type;

	protocol_parser(value_type& v) : base_type(v) {}

	typename base_type::token_iterator
	tokens_end(typename base_type::tokens_list const& tokens)
	{
		return tokens.end();
	}

	template < typename InputIterator >
	InputIterator
	operator()(InputIterator begin, InputIterator end)
	{
		base_type::value.clear();
		return base_type::parse(begin, end, std::back_inserter(base_type::value));
	}
};

namespace traits {

template < typename T >
struct has_formatter< std::vector< T >, TEXT_DATA_FORMAT > : std::true_type {};
template < typename T >
struct has_parser< std::vector< T >, TEXT_DATA_FORMAT > : std::true_type {};

template < typename T >
struct cpppg_data_mapping< std::vector< T > > :
	detail::data_mapping_base < oids::type::text, std::vector< T > > {};

}  // namespace traits

}  // namespace io
}  // namespace pg
}  // namespace db
}  // namespace tip


#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_VECTOR_HPP_ */
