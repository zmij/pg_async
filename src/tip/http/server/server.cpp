/**
 * @file /tip-server/src/tip/http/server/server.cpp
 * @brief
 * @date Jul 8, 2015
 * @author: zmij
 */

#include <tip/http/server/server.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <functional>
#include <thread>

#include <string.h>

#include <tip/log.hpp>

namespace tip {
namespace http {
namespace server {

using tip::log::logger;

namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "HTTPSRV";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace

server::server(io_service_ptr io_svc,
		std::string const& address, std::string const& port,
		std::size_t thread_pool_size,
		request_handler_ptr handler,
		stop_function stop)
		: io_service_(io_svc),
			thread_pool_size_(thread_pool_size),
			signals_(*io_service_),
			hard_signals_(*io_service_),
			acceptor_(*io_service_),
			new_connection_(),
			request_handler_(handler),
			stop_(stop)
{
	// Register to handle the signals that indicate when the server should exit.
	// It is safe to register for the same signal multiple times in a program,
	// provided all registration for the specified signal is made through Asio.
	signals_.add(SIGINT);
	signals_.add(SIGTERM);
#if defined(SIGQUIT)
	signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)
	signals_.async_wait(boost::bind(&server::handle_stop, this));

	hard_signals_.add(SIGSEGV);
	hard_signals_.add(SIGABRT);
	hard_signals_.async_wait(
		boost::bind(&server::handle_error, this,
			boost::asio::placeholders::error, boost::asio::placeholders::signal_number));

	// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
	boost::asio::ip::tcp::resolver resolver(*io_service_);
	boost::asio::ip::tcp::resolver::query query(address, port);
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();

	start_accept();
}

void server::run()
{
	// Create a pool of threads to run all of the io_services.
	std::vector< std::shared_ptr< std::thread > > threads;
	for (std::size_t i = 0; i < thread_pool_size_; ++i) {
		std::shared_ptr< std::thread > thread(
		new std::thread([&](){
			try {
				io_service_->run();
			} catch (std::exception const& e) {
				local_log(logger::ERROR) << "Uncaught exception " << e.what();
				handle_stop();
			} catch (...) {
				local_log(logger::ERROR) << "Uncaught exception";
				handle_stop();
			}
		}));
		threads.push_back(thread);
	}

	local_log(logger::INFO) << "Server started";
	// Wait for all threads in the pool to exit.
	for (std::size_t i = 0; i < threads.size(); ++i)
		threads[i]->join();
}

void server::start_accept()
{
	new_connection_.reset(new connection(io_service_, request_handler_));
	auto endpoint = std::make_shared<boost::asio::ip::tcp::endpoint>();
	acceptor_.async_accept(new_connection_->socket(), *endpoint,
			std::bind(&server::handle_accept, this,
					std::placeholders::_1, endpoint));
}

void server::handle_accept(const boost::system::error_code& e,
		endpoint_ptr endpoint)
{
	if (!e)
	{
		new_connection_->start(endpoint);
	}

	start_accept();
}

void
server::handle_stop()
{
	local_log(logger::INFO) << "Server stopping";
	acceptor_.cancel();
	if (stop_) {
		stop_();
	} else {
		io_service_->stop();
	}
}

void
server::handle_error(boost::system::error_code const& e, int signo)
{
	local_log(logger::ERROR) << "Received signal " << strsignal(signo) << ", terminating";
	handle_stop();
}


}  // namespace server
}  // namespace http
}  // namespace tip
