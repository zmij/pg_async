/**
 * @file /tip-server/src/tip/db/pg/detail/connection_impl.cpp
 * @brief
 * @date Jul 10, 2015
 * @author: zmij
 */

#include <tip/db/pg/detail/connection_impl.hpp>
#include <tip/db/pg/detail/startup.hpp>

#include <tip/db/pg/log.hpp>

#include <boost/bind.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

#ifdef WITH_TIP_LOG
namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "POSTGRE";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
using tip::log::logger;
#endif

//****************************************************************************
// tcp_layer
tcp_transport::tcp_transport(io_service& service) :
		resolver_(service), socket(service), connect_()
{
}

void
tcp_transport::connect_async(connection_options const& conn, connect_callback cb)
{
	if (conn.uri.empty()) {
		throw std::runtime_error("No connection uri!");
	}
	if (conn.schema != "tcp") {
		throw std::runtime_error("Wrong connection schema for TCP transport");
	}
	connect_ = cb;
	std::string host = conn.uri;
	std::string svc = "5432";
	std::string::size_type pos = conn.uri.find(":");
	if (pos != std::string::npos) {
		host = conn.uri.substr(0, pos);
		svc = conn.uri.substr(pos + 1);
	}

	tcp::resolver::query query (host, svc);
	resolver_.async_resolve(query, boost::bind(&tcp_transport::handle_resolve,
				this, asio::placeholders::error, asio::placeholders::iterator
			));

}

void
tcp_transport::handle_resolve(error_code const& ec,
                 	  			tcp::resolver::iterator endpoint_iterator)
{
	if (!ec) {
		asio::async_connect(socket, endpoint_iterator,
				boost::bind( &tcp_transport::handle_connect, this,
						asio::placeholders::error));
	} else {
		connect_(ec);
	}
}

void
tcp_transport::handle_connect(error_code const& ec)
{
	connect_(ec);
}

bool
tcp_transport::connected() const
{
	return socket.is_open();
}

void
tcp_transport::close()
{
	if (socket.is_open())
		socket.close();
}

//----------------------------------------------------------------------------
// socket_transport implementation
//----------------------------------------------------------------------------
socket_transport::socket_transport(io_service& service)
	: socket(service)
{
}

void
socket_transport::connect_async(connection_options const& conn,
		connect_callback cb)
{
	using boost::asio::local::stream_protocol;
	if (conn.schema != "socket") {
		throw std::runtime_error("Wrong connection schema for TCP transport");
	}
	connect_ = cb;
	std::string uri = conn.uri;

	if (uri.empty()) {
		#ifdef WITH_TIP_LOG
		local_log(logger::WARNING) << "Socket name is empty. Trying default";
		#endif
		uri = "/tmp/.s.PGSQL.5432";
	}
	socket.async_connect(stream_protocol::endpoint(uri),
			[cb](error_code const& ec) { cb(ec); });
}

bool
socket_transport::connected() const
{
	return socket.is_open();
}

void
socket_transport::close()
{
	if (socket.is_open())
		socket.close();
}


}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip


