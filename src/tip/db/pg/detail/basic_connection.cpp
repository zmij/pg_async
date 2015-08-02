/*
 * connection_base.cpp
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/detail/basic_connection.hpp>

#include <tip/db/pg/detail/connection_fsm.hpp>
#include <tip/db/pg/detail/transport.hpp>

#include <tip/db/pg/error.hpp>

#include <tip/db/pg/log.hpp>

#include <boost/bind.hpp>
#include <assert.h>

namespace tip {
namespace db {
namespace pg {

namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGCONN";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
using tip::log::logger;


template < typename TransportType >
std::shared_ptr< detail::concrete_connection< TransportType > >
create_connection(boost::asio::io_service& svc,
		connection_options const& opts,
		client_options_type const& co,
		connection_callbacks const& callbacks)
{
	typedef detail::concrete_connection< TransportType > connection_type;
	typedef std::shared_ptr< connection_type > connection_ptr;

	connection_ptr conn(new connection_type(svc, co, callbacks));
	conn->connect(opts);
	return conn;
}

basic_connection_ptr
basic_connection::create(io_service& svc, connection_options const& opts,
		client_options_type const& co, connection_callbacks const& callbacks)
{
	if (opts.schema == "tcp") {
		return create_connection< detail::tcp_transport >(svc, opts, co, callbacks);
	} else if (opts.schema == "socket") {
		return create_connection< detail::socket_transport >(svc, opts, co, callbacks);
	}
	std::stringstream os;
	os << "Schema " << opts.schema << " is unsupported";
	throw db_error(os.str());
}

basic_connection::basic_connection()
{
}

basic_connection::~basic_connection()
{
	local_log() << "*** basic_connection::~basic_connection()";
}

void
basic_connection::connect(connection_options const& opts)
{
	do_connect(opts);
}

void
basic_connection::begin(events::begin const& evt)
{
	do_begin(evt);
}

void
basic_connection::commit()
{
	do_commit();
}

void
basic_connection::rollback()
{
	do_rollback();
}

bool
basic_connection::in_transaction() const
{
	return is_in_transaction();
}

void
basic_connection::execute(events::execute const& query)
{
	do_execute(query);
}
void
basic_connection::execute(events::execute_prepared const& query)
{
	do_execute(query);
}


void
basic_connection::terminate()
{
	local_log() << "Terminate connection";
	do_terminate();
}

}  // namespace pg
}  // namespace db
}  // namespace tip



