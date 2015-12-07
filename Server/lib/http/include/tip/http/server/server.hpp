/**
 * @file /tip-server/include/tip/http/server/server.hpp
 * @brief
 * @date Jul 8, 2015
 * @author: zmij
 */

#ifndef TIP_HTTP_SERVER_SERVER_HPP
#define TIP_HTTP_SERVER_SERVER_HPP

#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <memory>
#include <tip/http/server/connection.hpp>
#include <tip/http/server/request_handler.hpp>

namespace tip {
namespace http {
namespace server {

/**
 * The top-level class of the HTTP server.
 */
class server: private boost::noncopyable {
public:
	typedef std::function< void () > stop_function;
	typedef std::shared_ptr< boost::asio::io_service > io_service_ptr;
	typedef std::shared_ptr< boost::asio::ip::tcp::endpoint> endpoint_ptr;
public:
	/**
	 * Construct the server to listen on the specified TCP address and port, and
	 * serve up files from the given directory.
	 * @param address TCP address to listen to
	 * @param port TCP port to bind
	 * @param thread_pool_size Size of the thread pool
	 * @param handler
	 */
	explicit
	server(io_service_ptr io_svc,
			std::string const& address,
			std::string const& port,
			std::size_t thread_pool_size,
			request_handler_ptr handler,
			stop_function = stop_function());

	/**
	 * Run the server's io_service loop.
	 */
	void run();

private:
	/**
	 * Initiate an asynchronous accept operation.
	 */
	void
	start_accept();

	/**
	 * Handle completion of an asynchronous accept operation.
	 * @param e
	 */
	void
	handle_accept(boost::system::error_code const& e, endpoint_ptr);

	/**
	 * Handle a request to stop the server.
	 */
	void
	handle_stop();

	/**
	 * Handle a hard exception
	 * @param e
	 * @param signo
	 */
	void
	handle_error(boost::system::error_code const& e, int signo);

private:
	/** The io_service used to perform asynchronous operations. */
	io_service_ptr io_service_;

	/** The number of threads that will call io_service::run(). */
	std::size_t thread_pool_size_;

	/** The signal_set is used to register for process termination notifications. */
	boost::asio::signal_set signals_;
	/** The signal_set is used to register for hard errors, to log then and gracefully exit. */
	boost::asio::signal_set hard_signals_;

	/** Acceptor used to listen for incoming connections. */
	boost::asio::ip::tcp::acceptor acceptor_;

	/** The next connection to be accepted. */
	connection_ptr new_connection_;

	/** The handler for all incoming requests. */
	request_handler_ptr request_handler_;

	stop_function stop_;
};

} // namespace server
} // namespace http
} // namespace tip

#endif /* TIP_HTTP_SERVER_SERVER_HPP */
