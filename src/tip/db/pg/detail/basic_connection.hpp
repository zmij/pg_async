/*
 * connection_base.hpp
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_CONNECTION_BASE_HPP_
#define TIP_DB_PG_DETAIL_CONNECTION_BASE_HPP_

#include <boost/noncopyable.hpp>

#include <tip/db/pg/asio_config.hpp>
#include <tip/db/pg/detail/protocol.hpp>

namespace boost {
namespace asio {
// forward declaration
class io_service;
}  // namespace asio
}  // namespace boost

namespace tip {
namespace db {
namespace pg {

class resultset;
class transaction;
typedef std::shared_ptr< transaction > transaction_ptr;

class basic_connection;
typedef std::shared_ptr< basic_connection > basic_connection_ptr;

typedef std::function < void (basic_connection_ptr) > connection_event_callback;
typedef std::function < void (basic_connection_ptr, error::connection_error) > connection_error_callback;
typedef std::function< void (resultset, bool) > query_internal_callback;
typedef std::function< void() > notification_callback;

struct connection_callbacks {
    connection_event_callback    idle;
    connection_event_callback    terminated;
    connection_error_callback    error;
};

namespace events {
struct commit {
    notification_callback       callback;
    error_callback              error;
};
struct rollback {
    notification_callback       callback;
    error_callback              error;
};

struct execute {
    std::string                 expression;
    query_internal_callback     result;
    query_error_callback        error;
};
struct execute_prepared {
    std::string                 expression;
    type_oid_sequence           param_types;
    std::vector< byte >         params;
    query_internal_callback     result;
    query_error_callback        error;
};

}

class basic_connection : public boost::noncopyable {
public:
    typedef asio_config::io_service_ptr io_service_ptr;
public:
    static basic_connection_ptr
    create(io_service_ptr svc, connection_options const&,
            client_options_type const&, connection_callbacks const&);
public:
    virtual ~basic_connection();

    void
    connect(connection_options const&);

    dbalias const&
    alias() const;

    void
    begin(events::begin&&);
    void
    commit(notification_callback = notification_callback(), error_callback = error_callback());
    void
    rollback(notification_callback = notification_callback(), error_callback = error_callback());

    bool
    in_transaction() const;

    void
    execute(events::execute&&);
    void
    execute(events::execute_prepared&&);

    void
    terminate();
protected:
    basic_connection();

private:
    virtual void
    do_connect(connection_options const& ) = 0;

    virtual dbalias const&
    get_alias() const = 0;

    virtual bool
    is_in_transaction() const = 0;

    virtual void
    do_begin(events::begin&&) = 0;
    virtual void
    do_commit(notification_callback, error_callback) = 0;
    virtual void
    do_rollback(notification_callback, error_callback) = 0;

    virtual void
    do_execute(events::execute&&) = 0;
    virtual void
    do_execute(events::execute_prepared&&) = 0;

    virtual void
    do_terminate() = 0;
};

}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* TIP_DB_PG_DETAIL_CONNECTION_BASE_HPP_ */
