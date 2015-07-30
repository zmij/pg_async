/*
 * basic_connection.new.cpp
 *
 *  Created on: Jul 30, 2015
 *      Author: zmij
 */

#include <tip/db/pg/detail/connection_fsm.hpp>
#include <tip/db/pg/detail/transport.hpp>
#include <tip/db/pg/error.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

template < typename TransportType >
std::shared_ptr< concrete_connection< TransportType > >
create_connection(boost::asio::io_service& svc,
		connection_options const& opts,
		basic_connection::client_options_type const& co)
{
	typedef concrete_connection< TransportType > connection_type;
	typedef std::shared_ptr< connection_type > connection_ptr;

	connection_ptr conn(new connection_type(svc, co));
	conn->start();
	conn->process_event(opts);
	return conn;
}

basic_connection_ptr
basic_connection::create(io_service& svc, connection_options const& opts,
		client_options_type const& co)
{
	if (opts.schema == "tcp") {
		return create_connection< tcp_transport >(svc, opts, co);
	} else if (opts.schema == "socket") {
		return create_connection< socket_transport >(svc, opts, co);
	}
	std::stringstream os;
	os << "Schema " << opts.schema << " is unsupported";
	throw db_error(os.str());
}

basic_connection::basic_connection()
{
}

}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip

