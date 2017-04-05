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

LOCAL_LOGGING_FACILITY_CFG(PGCONN, config::CONNECTION_LOG);

template < typename Handler >
void
wrap(asio_config::io_service::strand& strand, Handler& h)
{
    if (h)
        h = strand.wrap(h);
}

template < typename Handler >
Handler
wrap_it(asio_config::io_service::strand& strand, Handler h)
{
    if (h)
        return strand.wrap(h);
    return h;
}

template < typename TransportType >
std::shared_ptr< detail::concrete_connection< TransportType > >
create_connection(asio_config::io_service_ptr svc,
        connection_options const& opts,
        client_options_type const& co,
        connection_callbacks const& callbacks)
{
    typedef detail::concrete_connection< TransportType > connection_type;
    typedef std::shared_ptr< connection_type > concrete_connection_ptr;

    concrete_connection_ptr conn(new connection_type(svc, co, callbacks));
    conn->connect(opts);
    return conn;
}

basic_connection_ptr
basic_connection::create(io_service_ptr svc, connection_options const& opts,
        client_options_type const& co, connection_callbacks const& callbacks)
{
    if (opts.schema == "tcp") {
        return create_connection< detail::tcp_transport >(svc, opts, co, callbacks);
    } else if (opts.schema == "socket") {
        return create_connection< detail::socket_transport >(svc, opts, co, callbacks);
    }
    std::stringstream os;
    os << "Schema " << opts.schema << " is unsupported";
    throw error::connection_error(os.str());
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

dbalias const&
basic_connection::alias() const
{
    return get_alias();
}

void
basic_connection::begin(events::begin&& evt)
{
    wrap(strand(), evt.started);
    wrap(strand(), evt.error);
    do_begin(::std::move(evt));
}

void
basic_connection::commit(notification_callback cb, error_callback ecb)
{
    do_commit(wrap_it(strand(), cb), wrap_it(strand(), ecb));
}

void
basic_connection::rollback(notification_callback cb, error_callback ecb)
{
    do_rollback(wrap_it(strand(), cb), wrap_it(strand(), ecb));
}

bool
basic_connection::in_transaction() const
{
    return is_in_transaction();
}

void
basic_connection::execute(events::execute&& query)
{
    wrap(strand(), query.result);
    wrap(strand(), query.error);
    do_execute(::std::move(query));
}
void
basic_connection::execute(events::execute_prepared&& query)
{
    wrap(strand(), query.result);
    wrap(strand(), query.error);
    do_execute(::std::move(query));
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



