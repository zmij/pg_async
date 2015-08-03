/*
 * transport.hpp
 *
 *  Created on: Jul 30, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_TRANSPORT_HPP_
#define LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_TRANSPORT_HPP_

#include <boost/asio.hpp>
#include <tip/db/pg/common.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

struct tcp_transport {
	typedef boost::asio::io_service io_service;
	typedef boost::asio::ip::tcp tcp;
	typedef boost::system::error_code error_code;
	typedef std::function< void (boost::system::error_code const&) > connect_callback;
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
		boost::asio::async_read(socket, buffer,
				boost::asio::transfer_at_least(1), handler);
	}

	template < typename BufferType, typename HandlerType >
	void
	async_write(BufferType const& buffer, HandlerType handler)
	{
		boost::asio::async_write(socket, buffer, handler);
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
	typedef boost::asio::io_service io_service;
	typedef boost::asio::local::stream_protocol::socket socket_type;
	typedef boost::system::error_code error_code;
	typedef std::function< void (boost::system::error_code const&) > connect_callback;

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
		boost::asio::async_read(socket, buffer,
				boost::asio::transfer_at_least(1), handler);
	}

	template < typename BufferType, typename HandlerType >
	void
	async_write(BufferType const& buffer, HandlerType handler)
	{
		boost::asio::async_write(socket, buffer, handler);
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
