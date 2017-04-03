/*
 * asio_config.hpp
 *
 *  Created on: Mar 26, 2017
 *      Author: zmij
 */

#ifndef PUSHKIN_ASIO_ASIO_CONFIG_HPP_
#define PUSHKIN_ASIO_ASIO_CONFIG_HPP_

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
#include <boost/asio/local/stream_protocol.hpp>
#endif

#include <memory>

namespace asio_ns = ::boost::asio;

namespace psst {
namespace asio {

using io_service        = ::asio_ns::io_service;
using io_service_ptr    = ::std::shared_ptr< io_service >;

using error_code        = ::boost::system::error_code;
using system_error      = ::boost::system::system_error;

using tcp               = ::asio_ns::ip::tcp;
using udp               = ::asio_ns::ip::udp;

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
using local_socket      = ::asio_ns::local::stream_protocol;
#endif

} /* namespace asio */
} /* namespace psst */


#endif /* PUSHKIN_ASIO_ASIO_CONFIG_HPP_ */
