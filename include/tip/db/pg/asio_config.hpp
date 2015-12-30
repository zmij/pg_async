/*
 * config.hpp
 *
 *  Created on: Aug 5, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_ASIO_CONFIG_HPP_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_ASIO_CONFIG_HPP_


#ifdef WITH_BOOST_ASIO
#include <boost/asio.hpp>
#define ASIO_NAMESPACE ::boost::asio
#else
#include <asio.hpp>
#define ASIO_NAMESPACE ::asio
#endif
#include <memory>

namespace tip {
namespace db {
namespace pg {
namespace asio_config {

typedef ASIO_NAMESPACE::io_service io_service;
typedef std::shared_ptr< io_service > io_service_ptr;
typedef ASIO_NAMESPACE::ip::tcp tcp;
typedef ASIO_NAMESPACE::local::stream_protocol stream_protocol;

#ifdef WITH_BOOST_ASIO
typedef boost::system::error_code error_code;
#else
typedef ::asio::error_code error_code;
#endif

}  // namespace asio
}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_ASIO_CONFIG_HPP_ */
