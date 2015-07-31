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

#include <boost/system/error_code.hpp>

#include <tip/db/pg/log.hpp>
#include "db/config.hpp"
#include "test-environment.hpp"

namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGFSM";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
test_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace


// For more convenient changing severity, eg local_log(logger::WARNING)
using tip::log::logger;
namespace tip {
namespace db {
namespace pg {
namespace detail {

struct dummy_transport {
	typedef boost::asio::io_service io_service;
	typedef std::function< void (boost::system::error_code const&) > connect_callback;

	dummy_transport(io_service& svc) {}

	void
	connect_async(connection_options const&, connect_callback cb)
	{
		boost::system::error_code ec;
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
	async_read(BufferType& buffer, HandlerType handler)
	{
		local_log() << "Dummy async read";
	}

	template < typename BufferType, typename HandlerType >
	void
	async_write(BufferType const& buffer, HandlerType handler)
	{
		local_log() << "Dummy async write";
	}

};
}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip

using namespace tip::db::pg::detail;
using namespace tip::db::pg::events;
struct fsm : boost::msm::back::state_machine< connection_fsm_< dummy_transport, fsm > > {
	typedef boost::msm::back::state_machine< connection_fsm_< dummy_transport, fsm > > base_type;
	fsm(boost::asio::io_service& svc, client_options_type const& co)
		: base_type(std::ref(svc), co)
	{
	}
	virtual ~fsm() {}
	virtual void do_notify_idle()
	{
		test_log(logger::INFO) << "Connection notified idle";
	}
	virtual void do_notify_terminated()
	{
		test_log(logger::INFO) << "Connection notified terminated";
	}
	virtual void do_notify_error(tip::db::pg::connection_error const& e)
	{
		test_log(logger::INFO) << "Connection notified error " << e.what();
	}
};
typedef std::shared_ptr<fsm> fsm_ptr;
fsm::client_options_type client_options {
	{"client_encoding", 		"UTF8"},
	{"application_name", 		"pg_async"},
	//{"autocommit", 				"off"},
	{"client_min_messages", 	"debug5"}
};

TEST(DummyFSM, NormalFlow)
{
	boost::asio::io_service svc;
	fsm_ptr c( new fsm(std::ref(svc), client_options) );
	c->start();
	//	Connection
	c->process_event("main=tcp://user:password@localhost:5432[db]"_pg);		// unplugged 	-> t_conn
	c->process_event(ready_for_query());	// authn		-> idle

	// Begin transaction
	c->process_event(begin());		// idle			-> transaction::starting
	c->process_event(ready_for_query());	// transaction::starting -> transaction::idle

	// Commit transaction
	c->process_event(commit());		// transaction::idle	-> transaction::exiting
	c->process_event(ready_for_query());	// transaction::exiting	-> idle

	// Begin transaction
	c->process_event(begin());		// idle			-> transaction::starting
	c->process_event(ready_for_query());	// transaction::starting -> transaction::idle

	c->process_event(complete());	// transaction::exiting	-> idle

	// Rollback transaction
	c->process_event(rollback());	// transaction::idle	-> transaction::exiting
	c->process_event(ready_for_query());	// transaction::exiting	-> idle

	// Terminate connection
	c->process_event(terminate());	// idle -> X
}

TEST(DummyFSM, TerminateTran)
{
	boost::asio::io_service svc;
	fsm_ptr c( new fsm(std::ref(svc), client_options) );
	c->start();
	//	Connection
	c->process_event("main=tcp://user:password@localhost:5432[db]"_pg);		// unplugged 	-> t_conn
	c->process_event(ready_for_query());	// authn		-> idle

	// Begin transaction
	c->process_event(begin());		// idle			-> transaction::starting
	c->process_event(ready_for_query());	// transaction::starting -> transaction::idle

	c->process_event(terminate());	// deferred event

	c->process_event(rollback());	// transaction::idle -> transaction::exiting
	c->process_event(ready_for_query());	// transaction::exiting -> idle
}

TEST(DummyFSM, SimpleQueryMode)
{
	boost::asio::io_service svc;
	fsm_ptr c( new fsm(std::ref(svc), client_options) );
	c->start();
	//	Connection
	c->process_event("main=tcp://user:password@localhost:5432[db]"_pg);		// unplugged 	-> t_conn
	c->process_event(ready_for_query());	// authn		-> idle

	// Begin transaction
	c->process_event(begin());		// idle			-> transaction::starting
	c->process_event(ready_for_query());	// transaction::starting -> transaction::idle

	c->process_event(execute{ "bla" });		// transaction -> simple query
	c->process_event(row_description()); // waiting -> fetch_data
	for (int i = 0; i < 10; ++i) {
		c->process_event(row_data());
	}
	c->process_event(complete());	// fetch_data -> waiting

	c->process_event(row_description()); // waiting -> fetch_data
	for (int i = 0; i < 10; ++i) {
		c->process_event(row_data());
	}
	c->process_event(complete());	// fetch_data -> waiting

	c->process_event(ready_for_query()); // simple query -> transaction::idle
	c->process_event(commit());		// transaction::idle -> transaction::exiting
	c->process_event(ready_for_query());	// transaction -> idle
}

TEST(DummyFSM, ExtendedQueryMode)
{
	boost::asio::io_service svc;
	fsm_ptr c( new fsm(std::ref(svc), client_options) );
	c->start();
	//	Connection
	c->process_event("main=tcp://user:password@localhost:5432[db]"_pg);		// unplugged 	-> t_conn
	c->process_event(ready_for_query());	// authn		-> idle

	// Begin transaction
	c->process_event(begin());		// idle			-> transaction::starting
	c->process_event(ready_for_query());	// transaction::starting -> transaction::idle

	// Start extended query mode
	c->process_event(execute_prepared()); // transaction::idle -> eqm::prepare -> parse
	c->process_event(complete());	// parse -> bind
	c->process_event(complete());	// bind -> exec
}

template < typename TransportType >
struct test_connection :
		boost::msm::back::state_machine< connection_fsm_<
			TransportType,
			test_connection< TransportType > > > {
	typedef boost::msm::back::state_machine<
			connection_fsm_< TransportType, test_connection< TransportType > > > base_type;

	test_connection(boost::asio::io_service& svc,
			typename base_type::client_options_type const& co)
		: base_type(std::ref(svc), co)
	{
	}
	virtual ~test_connection() {}

	virtual void do_notify_idle()
	{
		test_log(logger::INFO) << "Connection notified idle";
	}
	virtual void do_notify_terminated()
	{
		test_log(logger::INFO) << "Connection notified terminated";
	}
	virtual void do_notify_error(tip::db::pg::connection_error const& e)
	{
		test_log(logger::INFO) << "Connection notified error " << e.what();
	}
};

template < typename TransportType >
void
test_normal_flow(tip::db::pg::connection_options const& opts)
{
	using namespace tip::db::pg;
	typedef test_connection< TransportType > fsm_type;
	typedef std::shared_ptr< fsm_type > fsm_ptr;

	boost::asio::io_service svc;
	fsm_ptr c(new fsm_type(std::ref(svc), client_options));

	c->start();
	c->process_event(opts);
	c->process_event(begin());
	c->process_event(execute{ "select * from pg_catalog.pg_type; select * from pg_catalog.pg_class" });
	c->process_event(execute{ "create temporary table dummy (id bigint)" });
	c->process_event(commit());
	c->process_event(terminate());

	svc.run();
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
	typedef test_connection< TransportType > fsm_type;
	typedef std::shared_ptr< fsm_type > fsm_ptr;

	boost::asio::io_service svc;
	fsm_ptr c(new fsm_type(std::ref(svc), client_options));

	c->start();
	c->process_event(opts);
	c->process_event(begin());
	c->process_event(terminate());
	c->process_event(rollback());

	svc.run();
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
	typedef test_connection< TransportType > fsm_type;
	typedef std::shared_ptr< fsm_type > fsm_ptr;

	boost::asio::io_service svc;
	fsm_ptr c(new fsm_type(std::ref(svc), client_options));

	c->start();
	c->process_event(opts);
	c->process_event(begin());
	c->process_event(begin());
	c->process_event(execute{ "select * from _shouldnt_be_there_" });
	//c->process_event(rollback());
	c->process_event(terminate());

	svc.run();
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
	typedef test_connection< TransportType > fsm_type;
	typedef std::shared_ptr< fsm_type > fsm_ptr;
	typedef std::vector< char > buffer_type;
	typedef std::vector< oids::type::oid_type > oid_sequence;

	boost::asio::io_service svc;
	fsm_ptr c(new fsm_type(std::ref(svc), client_options));

	c->start();
	c->process_event(opts);
	c->process_event(begin());

	c->process_event(execute_prepared{
		"select * from pg_catalog.pg_type"
	});

	c->process_event(execute{
		"create temporary table test_exec_prepared (id bigint, name text)"
	});

	{
		oid_sequence param_types;
		buffer_type params;
		tip::db::pg::detail::write_params(param_types, params, 100500, std::string("foo"));
		c->process_event(execute_prepared{
			"insert into test_exec_prepared(id, name) values ($1, $2)",
			param_types,
			params
		});
	}
	{
		oid_sequence param_types;
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

	svc.run();
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

