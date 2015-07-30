/**
 * @file /tip-server/src/tip/db/pg/detail/connection_impl.hpp
 * @brief
 * @date Jul 10, 2015
 * @author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_CONNECTION_IMPL_HPP_
#define TIP_DB_PG_DETAIL_CONNECTION_IMPL_HPP_

#include <tip/db/pg/detail/basic_connection.hpp>
#include <tip/db/pg/detail/terminated_state.hpp>
#include <tip/db/pg/error.hpp>
#include <tip/db/pg/detail/transport.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

template < typename TransportType >
class connection_impl: public connection_base {
public:
	typedef TransportType transport_type;
private:
	transport_type transport;
public:
	connection_impl(io_service& service, connection_options const& co,
					event_callback const& ready,
					event_callback const& terminated,
					connection_error_callback const& err,
					options_type const& aux)
		: connection_base(service, co, ready, terminated, err, aux),
		  transport(service)
	{
		start_connect();
	}

	virtual ~connection_impl() {}

	virtual void
	send(message const& m, api_handler handler = api_handler())
	{
		if (transport.connected() && !is_terminated()) {
			auto data_range = m.buffer();
			std::shared_ptr< connection_impl > shared_this =
					std::dynamic_pointer_cast< connection_impl > (shared_from_this());
			if (!handler) {
				handler = boost::bind(
						&connection_impl::handle_write, shared_this,
						asio::placeholders::error,
						asio::placeholders::bytes_transferred);
			}
			transport.async_write(
					boost::asio::buffer(&*data_range.first,
							data_range.second - data_range.first),
					strand_.wrap(handler));
		}
	}

private:
	void
	start_connect()
	{
		if (conn.uri.empty()) {
			throw std::runtime_error("No connection uri!");
		}
		if (conn.database.empty()) {
			throw std::runtime_error("No database!");
		}
		if (conn.user.empty()) {
			throw std::runtime_error("User not specified!");
		}
		transport.connect_async(conn,
				boost::bind(&connection_impl::handle_connect, this, _1));
	}

	void
	start_read()
	{
		std::shared_ptr< connection_impl > shared_this =
				std::dynamic_pointer_cast< connection_impl > (shared_from_this());
		transport.async_read(incoming_,
				strand_.wrap(boost::bind(&connection_impl::handle_read, shared_this,
						asio::placeholders::error, asio::placeholders::bytes_transferred)));
	}

	//@{
	/** @name asio callbacks */
	void
	handle_read(error_code const& ec, size_t bytes_transferred)
	{
		if (!ec) {
			std::istreambuf_iterator<char> in(&incoming_);
			read_message(in, bytes_transferred);
			std::shared_ptr< connection_impl > shared_this =
					std::dynamic_pointer_cast< connection_impl > (shared_from_this());
			transport.async_read(incoming_,
					strand_.wrap(boost::bind(&connection_impl::handle_read, shared_this,
							asio::placeholders::error, asio::placeholders::bytes_transferred)));
			read_package_complete(bytes_transferred);
		} else {
			if (!is_terminated()) {
				transit_state(connection_state_ptr(new terminated_state(*this)));

				error(connection_error(ec.message()));
			}
		}
	}
	//@}

	virtual void
	close()
	{
		transport.close();
	}
};

}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* TIP_DB_PG_DETAIL_CONNECTION_IMPL_HPP_ */
