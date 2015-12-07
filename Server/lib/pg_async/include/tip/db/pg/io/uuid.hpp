/*
 * uuid.hpp
 *
 *  Created on: Aug 10, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_UUID_HPP_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_UUID_HPP_

#include <tip/db/pg/protocol_io_traits.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace tip {
namespace db {
namespace pg {
namespace io {

namespace traits {

//@{
template < >
struct pgcpp_data_mapping < oids::type::uuid > :
		detail::data_mapping_base< oids::type::uuid, boost::uuids::uuid > {};
template < >
struct cpppg_data_mapping < boost::uuids::uuid > :
		detail::data_mapping_base< oids::type::uuid, boost::uuids::uuid > {};
//@}


}  // namespace traits

}  // namespace io
}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_UUID_HPP_ */
