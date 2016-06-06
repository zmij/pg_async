/*
 * ip_address.hpp
 *
 *  Created on: Sep 2, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_IP_ADDRESS_HPP_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_IP_ADDRESS_HPP_

#include <tip/db/pg/protocol_io_traits.hpp>
#include <tip/db/pg/asio_config.hpp>

#ifdef WITH_BOOST_ASIO
#include <boost/asio/ip/address.hpp>
#else
#include <asio/ip/address.hpp>
#endif

namespace tip {
namespace db {
namespace pg {
namespace io {

template < >
struct protocol_parser< ASIO_NAMESPACE::ip::address, TEXT_DATA_FORMAT > :
		detail::parser_base< ASIO_NAMESPACE::ip::address > {
	typedef detail::parser_base< ASIO_NAMESPACE::ip::address > base_type;
	typedef base_type::value_type value_type;

	protocol_parser(value_type& v) : base_type(v) {}

	size_t
	size() const
	{
		return base_type::value.to_string().size();
	}

	template < typename InputIterator >
	InputIterator
	operator()(InputIterator begin, InputIterator end)
	{
		typedef InputIterator iterator_type;
		typedef std::iterator_traits< iterator_type > iter_traits;
		typedef typename iter_traits::value_type iter_value_type;
		static_assert(std::is_same< iter_value_type, byte >::type::value,
				"Input iterator must be over a char container");
		std::string s(begin, end);
		asio_config::error_code ec;
		value_type tmp = value_type::from_string(s, ec);
		if (!ec) {
			base_type::value = tmp;
			return end;
		}
		return begin;
	}
};

namespace traits {

template < >
struct has_parser< ASIO_NAMESPACE::ip::address, TEXT_DATA_FORMAT > : std::true_type {};
//@{
template < >
struct pgcpp_data_mapping< oids::type::inet > :
		detail::data_mapping_base< oids::type::inet, boost::asio::ip::address > {};
template < >
struct cpppg_data_mapping< boost::asio::ip::address > :
		detail::data_mapping_base< oids::type::inet, boost::asio::ip::address > {};
//@}

}  // namespace traits

}  // namespace io
}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_IP_ADDRESS_HPP_ */
