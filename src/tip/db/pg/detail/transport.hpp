/*
 * transport.hpp
 *
 *  Created on: Jul 30, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_TRANSPORT_HPP_
#define LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_TRANSPORT_HPP_

#include <tip/db/pg/asio_config.hpp>
#include <tip/db/pg/common.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

struct tcp_transport {
	typedef asio_config::io_service io_service;
	typedef asio_config::tcp tcp;
	typedef asio_config::error_code error_code;
	typedef std::function< void (error_code const&) > connect_callback;
	typedef tcp::socket socket_type;

	tcp_transport(io_service&);

	void
	connect_async(connection_options const&, connect_callback);

	bool
	connected() const;

	void
	close();

	template < typename BufferType, typename HandlerType >
	void
	async_read(BufferType& buffer, HandlerType handler)
	{
		ASIO_NAMESPACE::async_read(socket, buffer,
				ASIO_NAMESPACE::transfer_at_least(1), handler);
	}

	template < typename BufferType, typename HandlerType >
	void
	async_write(BufferType const& buffer, HandlerType handler)
	{
		ASIO_NAMESPACE::async_write(socket, buffer, handler);
	}
private:
	tcp::resolver resolver_;
	connect_callback connect_;
	socket_type socket;


	void
	handle_resolve(error_code const& ec,
					tcp::resolver::iterator endpoint_iterator);
	void
	handle_connect(error_code const& ec);
};

struct socket_transport {
	typedef asio_config::io_service io_service;
	typedef asio_config::stream_protocol::socket socket_type;
	typedef asio_config::error_code error_code;
	typedef std::function< void (error_code const&) > connect_callback;

	socket_transport(io_service&);

	void
	connect_async(connection_options const&, connect_callback);

	bool
	connected() const;

	void
	close();
	template < typename BufferType, typename HandlerType >
	void
	async_read(BufferType& buffer, HandlerType handler)
	{
		ASIO_NAMESPACE::async_read(socket, buffer,
				ASIO_NAMESPACE::transfer_at_least(1), handler);
	}

	template < typename BufferType, typename HandlerType >
	void
	async_write(BufferType const& buffer, HandlerType handler)
	{
		ASIO_NAMESPACE::async_write(socket, buffer, handler);
	}
private:
	connect_callback connect_;
	socket_type socket;
};


}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip


#endif /* LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_TRANSPORT_HPP_ */
