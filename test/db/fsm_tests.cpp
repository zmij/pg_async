/*
 * fsm_tests.cpp
 *
 *  Created on: Jul 29, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <tip/db/pg/detail/connection_fsm.hpp>
#include <tip/db/pg/detail/transport.hpp>

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
local_log(logger::event_severity s = DEFAULT_SEVERITY)
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
typedef boost::msm::back::state_machine< connection_fsm_< dummy_transport > > fsm;
typedef std::shared_ptr<fsm> fsm_ptr;
fsm::client_options_type client_options {
	{"client_encoding", "UTF8"},
	{"application_name", "pg_async"},
	{"client_min_messages", "debug5"}
};

TEST(DummyFSM, NormalFlow)
{
	boost::asio::io_service svc;
	fsm_ptr c( new fsm(std::ref(svc), client_options) );
	c->start();
	//	Connection
	c->process_event("main=tcp://user:password@localhost:5432[db]"_pg);		// unplugged 	-> t_conn
	c->process_event(complete());	// authn		-> idle

	// Begin transaction
	c->process_event(begin());		// idle			-> transaction::starting
	c->process_event(complete());	// transaction::starting -> transaction::idle

	// Commit transaction
	c->process_event(commit());		// transaction::idle	-> transaction::exiting
	c->process_event(complete());	// transaction::exiting	-> idle

	// Begin transaction
	c->process_event(begin());		// idle			-> transaction::starting
	c->process_event(complete());	// transaction::starting -> transaction::idle

	c->process_event(complete());	// transaction::exiting	-> idle

	// Rollback transaction
	c->process_event(rollback());	// transaction::idle	-> transaction::exiting
	c->process_event(complete());	// transaction::exiting	-> idle

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
	c->process_event(complete());	// authn		-> idle

	// Begin transaction
	c->process_event(begin());		// idle			-> transaction::starting
	c->process_event(complete());	// transaction::starting -> transaction::idle

	c->process_event(terminate());	// deferred event

	c->process_event(rollback());	// transaction::idle -> transaction::exiting
	c->process_event(complete());	// transaction::exiting -> idle
}

TEST(DummyFSM, SimpleQueryMode)
{
	boost::asio::io_service svc;
	fsm_ptr c( new fsm(std::ref(svc), client_options) );
	c->start();
	//	Connection
	c->process_event("main=tcp://user:password@localhost:5432[db]"_pg);		// unplugged 	-> t_conn
	c->process_event(complete());	// authn		-> idle

	// Begin transaction
	c->process_event(begin());		// idle			-> transaction::starting
	c->process_event(complete());	// transaction::starting -> transaction::idle

	c->process_event(execute());		// transaction -> simple query
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
	c->process_event(complete());	// transaction -> idle
}

TEST(DummyFSM, ExtendedQueryMode)
{
	boost::asio::io_service svc;
	fsm_ptr c( new fsm(std::ref(svc), client_options) );
	c->start();
	//	Connection
	c->process_event("main=tcp://user:password@localhost:5432[db]"_pg);		// unplugged 	-> t_conn
	c->process_event(complete());	// authn		-> idle

	// Begin transaction
	c->process_event(begin());		// idle			-> transaction::starting
	c->process_event(complete());	// transaction::starting -> transaction::idle

	// Start extended query mode
	c->process_event(execute_prepared()); // transaction::idle -> eqm::prepare -> parse
	c->process_event(complete());	// parse -> bind
	c->process_event(complete());	// bind -> exec
}

typedef boost::msm::back::state_machine< connection_fsm_< tcp_transport > > tcp_fsm;
typedef std::shared_ptr< tcp_fsm > tcp_fsm_ptr;

TEST(FSM, NormalFlow)
{
	using namespace tip::db::pg;
	if (! test::environment::test_database.empty() ) {
		boost::asio::io_service svc;
		connection_options opts = connection_options::parse(test::environment::test_database);

		tcp_fsm_ptr c(new tcp_fsm(std::ref(svc), client_options));
		c->start();
		c->process_event(opts);


		svc.run();
	}
}
