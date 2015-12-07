/**
 * @file /tip-server/include/tip/http/server/connection.hpp
 * @brief
 * @date Jul 8, 2015
 * @author: zmij
 */

#ifndef TIP_HTTP_SERVER_CONNECTION_HPP
#define TIP_HTTP_SERVER_CONNECTION_HPP

#include <boost/asio.hpp>
#include <array>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <tip/util/read_result.hpp>

#include <tip/http/common/header.hpp>
#include <tip/http/common/response_status.hpp>
#include <tip/http/common/request_fwd.hpp>
#include <tip/http/common/response_fwd.hpp>

#include <tip/http/server/reply.hpp>

#include <tip/http/server/request_handler.hpp>

namespace tip {
namespace http {
namespace server {

/**
 * Represents a single connection from a client.
 */
class connection
		: public std::enable_shared_from_this< connection >,
		  private boost::noncopyable {
public:
	typedef std::shared_ptr< boost::asio::io_service > io_service_ptr;
	typedef std::weak_ptr< boost::asio::io_service > io_service_weak_ptr;
	typedef std::shared_ptr< boost::asio::ip::tcp::endpoint> endpoint_ptr;
public:
	/**
	 * Construct a connection with the given io_service.
	 * @param io_service I/O service
	 * @param handler request handler pointer
	 */
	connection(io_service_ptr io_service,
						request_handler_ptr handler);
	~connection();

	/**
	 * Get the socket associated with the connection.
	 */
	boost::asio::ip::tcp::socket&
	socket();

	/**
	 *  Start the first asynchronous operation for the connection.
	 */
	void start(endpoint_ptr peer);

private:
	typedef util::read_result< std::istream& > read_result_type;
	typedef read_result_type::read_callback_type read_callback;

	void
	read_request_headers();
	void
	handle_read_headers(boost::system::error_code const& e,
			std::size_t bytes_transferred);

	void
	read_request_body(request_ptr req, read_result_type res);

	void
	handle_read_body(boost::system::error_code const& e,
			std::size_t bytes_transferred, request_ptr req,
			read_callback cb);
	/**
	 *  Handle completion of a write operation.
	 */
	void
	handle_write(boost::system::error_code const& e);
	void
	handle_write_response(boost::system::error_code const& e,
			size_t bytes_transferred, response_const_ptr resp);

	void
	send_response( response_const_ptr resp );
	void
	send_error(response_status::status_type status);
private:
	typedef boost::asio::streambuf buffer_type;
	io_service_weak_ptr io_service_;
	endpoint_ptr 		peer_;
	/** Strand to ensure the connection's handlers are not called concurrently. */
	boost::asio::io_service::strand strand_;

	/** Socket for the connection. */
	boost::asio::ip::tcp::socket socket_;

	/** The handler used to process the incoming request. */
	request_handler_ptr request_handler_;

	/** Buffer for incoming data. */
	buffer_type incoming_;
	buffer_type outgoing_;

	headers default_headers_;
};

typedef std::shared_ptr< connection > connection_ptr;

}  // namespace server
}  // namespace http
}  // namespace tip

#endif /* TIP_HTTP_SERVER_CONNECTION_HPP */
