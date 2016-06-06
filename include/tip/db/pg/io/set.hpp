//
// Created by zmij on 19.10.15.
//

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_SET_HPP
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_SET_HPP

#include <tip/db/pg/protocol_io_traits.hpp>
#include <tip/db/pg/io/container_to_array.hpp>

#include <set>

namespace tip {
namespace db {
namespace pg {
namespace io {

template < typename T >
struct  protocol_formatter< std::set< T >, TEXT_DATA_FORMAT > :
		detail::text_container_formatter< std::set< T > > {

	typedef detail::text_container_formatter< std::set< T > > base_type;
	typedef typename base_type::value_type value_type;

	protocol_formatter(value_type const& v) : base_type(v) {}
};

template < typename T >
struct protocol_parser< std::set< T >, TEXT_DATA_FORMAT > :
	detail::text_container_parser<
	protocol_parser< std::set< T >, TEXT_DATA_FORMAT >,
	std::set< T > > {

	typedef detail::text_container_parser<
	protocol_parser< std::set< T >, TEXT_DATA_FORMAT >,
	std::set< T > > base_type;
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
		return base_type::parse(begin, end, std::inserter(base_type::value,
				base_type::value.begin()));
	}
};

namespace traits {

template < typename T >
struct has_formatter< std::set< T >, TEXT_DATA_FORMAT > : std::true_type {};
template < typename T >
struct has_parser< std::set< T >, TEXT_DATA_FORMAT > : std::true_type {};

template < typename T >
struct cpppg_data_mapping< std::set< T > > :
	detail::data_mapping_base < oids::type::text, std::set< T > > {};

}  // namespace traits


}  // namespace io
}  // namespace pg
}  // namespace db
}  // namespace tip

#endif //LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_SET_HPP
