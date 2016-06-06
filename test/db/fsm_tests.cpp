/*
 * fsm_tests.cpp
 *
 *  Created on: Jul 29, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <tip/db/pg/detail/connection_fsm.hpp>
#include <tip/db/pg/detail/transport.hpp>
#include <tip/db/pg/query.hpp>

#include <tip/db/pg/log.hpp>
#include "db/config.hpp"
#include "test-environment.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"


namespace asio_config = tip::db::pg::asio_config;

LOCAL_LOGGING_FACILITY_FUNC(PGFSM, TRACE, test_log);

namespace tip {
namespace db {
namespace pg {
namespace detail {

struct dummy_transport {
    using io_service_ptr = asio_config::io_service_ptr;
    using connect_callback = std::function< void (asio_config::error_code const&) >;

    dummy_transport(io_service_ptr) {}

    void
    connect_async(connection_options const&, connect_callback cb)
    {
        asio_config::error_code ec;
        cb(ec);
    }

    bool
    connected() const
    {
        return true;
    }

    void
    close()
    {

    }

    template < typename BufferType, typename HandlerType >
    void
    async_read(BufferType&, HandlerType)
    {
        test_log() << "Dummy async read";
    }

    template < typename BufferType, typename HandlerType >
    void
    async_write(BufferType const&, HandlerType)
    {
        test_log() << "Dummy async write";
    }

};

namespace static_tests {

using fsm_type = concrete_connection<dummy_transport>;
using transaction_fsm_type = ::afsm::state<
        fsm_type::state_definition_type::transaction, fsm_type >;
using simple_query_fsm_type = ::afsm::state<
        transaction_fsm_type::state_definition_type::simple_query, transaction_fsm_type >;
using fetch_data_fsm_type = ::afsm::state<
        simple_query_fsm_type::state_definition_type::fetch_data, simple_query_fsm_type >;

static_assert(!::std::is_same<fetch_data_fsm_type::handled_events, void>::value,
        "Handled events for fetch data is not void");
static_assert(!::std::is_same<fetch_data_fsm_type::internal_events, void>::value,
        "Internal events for fetch data is not void");
static_assert(fetch_data_fsm_type::handled_events::size == 1,
        "Fetch data handles one event internally");
static_assert(fetch_data_fsm_type::internal_events::size == 1,
        "Fetch data handles one event internally");
static_assert(!::std::is_same< fetch_data_fsm_type::state_definition_type::internal_transitions, void >::value,
        "Fetch data has internal transitions table");
static_assert(::psst::meta::contains<row_event, fetch_data_fsm_type::internal_events>::value,
        "Row event is handled internally");

static_assert(::afsm::detail::event_process_selector<
        row_event, fetch_data_fsm_type::internal_events,
        fetch_data_fsm_type::deferred_events>::value
            == ::afsm::actions::event_process_result::process, "");
static_assert(::afsm::detail::event_process_selector<
        row_event, fetch_data_fsm_type::handled_events,
        fetch_data_fsm_type::deferred_events>::value
            == ::afsm::actions::event_process_result::process, "");
}  /* namespace static_tests */

}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip

using namespace tip::db::pg::detail;
using namespace tip::db::pg::events;

using fsm = concrete_connection<dummy_transport>;
using fsm_ptr = std::shared_ptr<fsm>;



tip::db::pg::client_options_type client_options {
    {"client_encoding",         "UTF8"},
    {"application_name",         "pg_async"},
    //{"autocommit",                 "off"},
    {"client_min_messages",     "debug5"}
};

TEST(DummyFSM, NormalFlow)
{
    ::asio_config::io_service_ptr svc(std::make_shared<::asio_config::io_service>());
    fsm_ptr c( new fsm(svc, client_options, {}) );
    //    Connection
    c->process_event("main=tcp://user:password@localhost:5432[db]"_pg);        // unplugged     -> t_conn
    c->process_event(ready_for_query());    // authn        -> idle

    // Begin transaction
    c->process_event(begin());        // idle            -> transaction::starting
    c->process_event(ready_for_query());    // transaction::starting -> transaction::idle

    // Commit transaction
    c->process_event(commit());        // transaction::idle    -> transaction::exiting
    c->process_event(ready_for_query());    // transaction::exiting    -> idle

    // Begin transaction
    c->process_event(begin());        // idle            -> transaction::starting
    c->process_event(ready_for_query());    // transaction::starting -> transaction::idle

    c->process_event(complete());    // transaction::exiting    -> idle

    // Rollback transaction
    c->process_event(rollback());    // transaction::idle    -> transaction::exiting
    c->process_event(ready_for_query());    // transaction::exiting    -> idle

    // Terminate connection
    c->process_event(terminate());    // idle -> X
}

TEST(DummyFSM, TerminateTran)
{
    ::asio_config::io_service_ptr svc(std::make_shared<::asio_config::io_service>());
    fsm_ptr c( new fsm(svc, client_options, {}) );
    //    Connection
    c->process_event("main=tcp://user:password@localhost:5432[db]"_pg);        // unplugged     -> t_conn
    c->process_event(ready_for_query());    // authn        -> idle

    // Begin transaction
    c->process_event(begin());        // idle            -> transaction::starting
    c->process_event(ready_for_query());    // transaction::starting -> transaction::idle

    c->process_event(terminate());    // deferred event

    c->process_event(rollback());    // transaction::idle -> transaction::exiting
    c->process_event(ready_for_query());    // transaction::exiting -> idle
}

TEST(DummyFSM, SimpleQueryMode)
{
    ::asio_config::io_service_ptr svc(std::make_shared<::asio_config::io_service>());
    fsm_ptr c( new fsm(svc, client_options, {}) );
    //    Connection
    test_log() << "Connect";
    c->process_event("main=tcp://user:password@localhost:5432[db]"_pg);        // unplugged     -> t_conn
    test_log() << "Ready for query";
    c->process_event(ready_for_query());    // authn        -> idle

    // Begin transaction
    test_log() << "Begin transaction";
    c->process_event(begin());        // idle            -> transaction::starting
    test_log() << "Ready for query";
    c->process_event(ready_for_query());    // transaction::starting -> transaction::idle

    test_log() << "Execute simple query";
    c->process_event(execute{ "bla" });        // transaction -> simple query
    test_log() << "Row description";
    c->process_event(row_description()); // waiting -> fetch_data
    for (int i = 0; i < 10; ++i) {
        test_log() << "Data row";
        c->process_event(row_event());
    }
    test_log() << "Command complete";
    c->process_event(complete());    // fetch_data -> waiting

    test_log() << "Row description";
    c->process_event(row_description()); // waiting -> fetch_data
    for (int i = 0; i < 10; ++i) {
        test_log() << "Data row";
        c->process_event(row_event());
    }
    test_log() << "Command complete";
    c->process_event(complete());    // fetch_data -> waiting

    test_log() << "Terminate (should be deferred)";
    c->process_event(terminate{});  // defer event

    test_log() << "Ready for query";
    c->process_event(ready_for_query()); // simple query -> transaction::idle
    test_log() << "Commit";
    c->process_event(complete());    // fetch_data -> waiting
    test_log() << "Command complete";
    c->process_event(complete());    // fetch_data -> waiting
    test_log() << "Ready for query";
    c->process_event(ready_for_query());    // transaction -> idle
}

TEST(DummyFSM, ExtendedQueryMode)
{
    ::asio_config::io_service_ptr svc(std::make_shared<::asio_config::io_service>());
    fsm_ptr c( new fsm(svc, client_options, {}) );
    //    Connection
    c->process_event("main=tcp://user:password@localhost:5432[db]"_pg);        // unplugged     -> t_conn
    c->process_event(ready_for_query());    // authn        -> idle

    // Begin transaction
    c->process_event(begin());        // idle            -> transaction::starting
    c->process_event(ready_for_query());    // transaction::starting -> transaction::idle

    // Start extended query mode
    c->process_event(execute_prepared()); // transaction::idle -> eqm::prepare -> parse
    c->process_event(complete());    // parse -> bind
    c->process_event(complete());    // bind -> exec
}

template < typename TransportType >
void
test_normal_flow(tip::db::pg::connection_options const& opts)
{
    using namespace tip::db::pg;
    using fsm_type = concrete_connection< TransportType >;
    using transport_fsm_ptr = std::shared_ptr< fsm_type >;

    ::asio_config::io_service_ptr svc(std::make_shared<::asio_config::io_service>());
    transport_fsm_ptr c(new fsm_type(svc, client_options, {}));

    c->process_event(opts);
    c->process_event(begin{});
    c->process_event(execute{ "select * from pg_catalog.pg_type; select * from pg_catalog.pg_class" });
    c->process_event(execute{ "create temporary table dummy (id bigint)" });
    c->process_event(commit{});
    c->process_event(terminate{});

    svc->run();
}

TEST(FSM, NormalFlow)
{
    using namespace tip::db::pg;
    if (! test::environment::test_database.empty() ) {
        connection_options opts = connection_options::parse(test::environment::test_database);

        if (opts.schema == "tcp") {
            test_normal_flow< tcp_transport >(opts);
        } else if (opts.schema == "socket") {
            test_normal_flow< socket_transport >(opts);
        }
    }
}

template < typename TransportType >
void
test_preliminary_terminate(tip::db::pg::connection_options const& opts)
{
    using namespace tip::db::pg;
    using fsm_type = concrete_connection< TransportType >;
    using transport_fsm_ptr = std::shared_ptr< fsm_type >;

    ::asio_config::io_service_ptr svc(std::make_shared<::asio_config::io_service>());
    transport_fsm_ptr c(new fsm_type(svc, client_options, {}));

    c->process_event(opts);
    c->process_event(begin());
    c->process_event(terminate());
    c->process_event(rollback());

    svc->run();
}

TEST(FSM, PreliminaryTerminate)
{
    using namespace tip::db::pg;
    if (! test::environment::test_database.empty() ) {
        connection_options opts = connection_options::parse(test::environment::test_database);

        if (opts.schema == "tcp") {
            test_preliminary_terminate< tcp_transport >(opts);
        } else if (opts.schema == "socket") {
            test_preliminary_terminate< socket_transport >(opts);
        }
    }
}

template < typename TransportType >
void
test_error_in_query(tip::db::pg::connection_options const& opts)
{
    using namespace tip::db::pg;
    using fsm_type = concrete_connection< TransportType >;
    using transport_fsm_ptr = std::shared_ptr< fsm_type >;

    ::asio_config::io_service_ptr svc(std::make_shared<::asio_config::io_service>());
    transport_fsm_ptr c(new fsm_type(svc, client_options, {}));

    c->process_event(opts);
    c->process_event(begin());
    c->process_event(begin());
    c->process_event(execute{ "select * from _shouldnt_be_there_" });
    c->process_event(terminate());

    svc->run();
}

TEST(FSM, ErrorInSimpleQuery)
{
    using namespace tip::db::pg;
    if (! test::environment::test_database.empty() ) {
        connection_options opts = connection_options::parse(test::environment::test_database);

        if (opts.schema == "tcp") {
            test_error_in_query< tcp_transport >(opts);
        } else if (opts.schema == "socket") {
            test_error_in_query< socket_transport >(opts);
        }
    }
}

template < typename TransportType >
void
test_exec_prepared(tip::db::pg::connection_options const& opts)
{
    using namespace tip::db::pg;
    using fsm_type = concrete_connection< TransportType >;
    using transport_fsm_ptr = std::shared_ptr< fsm_type >;
    using buffer_type = std::vector< char >;

    ::asio_config::io_service_ptr svc(std::make_shared<::asio_config::io_service>());
    transport_fsm_ptr c(new fsm_type(svc, client_options, {}));

    c->process_event(opts);
    c->process_event(begin());

    c->process_event(execute_prepared{
        "select * from pg_catalog.pg_type"
    });

    c->process_event(events::execute{
        "create temporary table test_exec_prepared (id bigint, name text)"
    });

    {
        type_oid_sequence param_types;
        buffer_type params;
        tip::db::pg::detail::write_params(param_types, params, 100500, std::string("foo"));
        c->process_event(execute_prepared{
            "insert into test_exec_prepared(id, name) values ($1, $2)",
            param_types,
            params
        });
    }
    {
        type_oid_sequence param_types;
        buffer_type params;
        tip::db::pg::detail::write_params(param_types, params, 100501, std::string("bar"));
        c->process_event(execute_prepared{
            "insert into test_exec_prepared(id, name) values ($1, $2)",
            param_types,
            params
        });
    }

    c->process_event(commit());
    c->process_event(terminate());

    svc->run();
}

TEST(FSM, ExecPrepared)
{
    using namespace tip::db::pg;
    if (! test::environment::test_database.empty() ) {
        connection_options opts = connection_options::parse(test::environment::test_database);

        if (opts.schema == "tcp") {
            test_exec_prepared< tcp_transport >(opts);
        } else if (opts.schema == "socket") {
            test_exec_prepared< socket_transport >(opts);
        }
    }
}

#pragma GCC diagnostic pop
