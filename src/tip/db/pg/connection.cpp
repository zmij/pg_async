/*
 * pgconnection.cpp
 *
 *  Created on: 07 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/connection.hpp>
#include <tip/db/pg/resultset.hpp>
#include <tip/db/pg/detail/protocol.hpp>
#include <tip/db/pg/detail/connection_impl.hpp>
#include <tip/db/pg/detail/connection_lock.hpp>

#ifdef WITH_TIP_LOG
#include <tip/log/log.hpp>
#endif

#include <sstream>
#include <exception>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

namespace tip {
namespace db {
namespace pg {

namespace asio = boost::asio;
using asio::ip::tcp;
using boost::system::error_code;

#ifdef WITH_TIP_LOG
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
#endif

connection_ptr
connection::create(io_service& service,
		   connection_event_callback ready,
		   connection_event_callback terminated,
		   connection_error_callback err,
		   connection_options const& opts,
           connection_params const& params)
{
	connection_ptr conn(new connection());
	conn->init(service, ready, terminated, err, opts, params);
	return conn;
}

connection::connection() :
	pimpl_()
{
}

void
connection::init(io_service& service,
		   connection_event_callback ready,
		   connection_event_callback terminated,
		   connection_error_callback err,
		   connection_options const& opts,
		   connection_params const& params)
{
	typedef detail::connection_impl< detail::tcp_transport > tcp_impl;
	typedef detail::connection_impl< detail::socket_transport > socket_impl;

	detail::connection_base::event_callback impl_ready =
			std::bind( &connection::implementation_event,
					shared_from_this(), ready,
					std::placeholders::_1 );
	detail::connection_base::event_callback impl_terminated =
			std::bind( &connection::implementation_event,
					shared_from_this(), terminated,
					std::placeholders::_1 );
	detail::connection_base::connection_error_callback impl_error =
			std::bind( &connection::implementation_error,
					shared_from_this(), err,
					std::placeholders::_1 );

	// Switch the connection schema
	if (opts.schema == "tcp") {
		pimpl_.reset(new tcp_impl(service, opts, impl_ready, impl_terminated, impl_error, params));
	} else if (opts.schema == "socket") {
		pimpl_.reset(new socket_impl(service, opts, impl_ready, impl_terminated, impl_error, params));
	} else {
		std::stringstream os;
		os << "Schema " << opts.schema << " is unsupported";
		throw std::runtime_error(os.str());
	}

}

connection::~connection()
{
#ifdef WITH_TIP_LOG
	local_log(logger::DEBUG) << "**** connection::~connection()";
#endif
}

connection::state_type
connection::state() const
{
	return pimpl_->connection_state();
}

void
connection::execute_query(std::string const& query,
		result_callback cb,
		error_callback err,
		connection_lock_ptr l)
{
	pimpl_->execute_query(query,
			std::bind(&connection::query_executed,
					shared_from_this(), cb,
					std::placeholders::_1, std::placeholders::_2, l),
			err);
}

void
connection::terminate()
{
	pimpl_->terminate();
}

connection_lock_ptr
connection::lock()
{
	pimpl_->lock();
	return connection_lock_ptr(new detail::connection_lock( shared_from_this(),
			boost::bind(&connection::unlock, shared_from_this() )));
}

void
connection::unlock()
{
	pimpl_->unlock();
}

void
connection::begin_transaction(connection_lock_callback cb, error_callback err, bool autocommit)
{
	pimpl_->begin_transaction(
			std::bind(&connection::transaction_started, shared_from_this(), cb),
			err, autocommit
	);
}

void
connection::commit_transaction(connection_lock_ptr l, connection_lock_callback cb, error_callback err)
{
	pimpl_->commit_transaction(
			std::bind(&connection::transaction_finished, shared_from_this(), l, cb),
			err
	);
}

void
connection::rollback_transaction(connection_lock_ptr l, connection_lock_callback cb, error_callback err)
{
	pimpl_->rollback_transaction(
			std::bind(&connection::transaction_finished, shared_from_this(), l, cb),
			err
	);
}

bool
connection::in_transaction() const
{
	return pimpl_->in_transaction();
}

void
connection::implementation_event(connection_event_callback event, pimpl i)
{
	if (event) {
		#ifdef WITH_TIP_LOG
		local_log() << "Dispatch connection event";
		#endif
		event(shared_from_this());
	}
}

void
connection::implementation_error(connection_error_callback handler, connection_error const& ec)
{
	if (handler) {
		handler(shared_from_this(), ec);
	}
}

void
connection::transaction_started(connection_lock_callback cb)
{
	cb(lock());
}

void
connection::transaction_finished(connection_lock_ptr l, connection_lock_callback cb)
{
	cb(l);
}

void
connection::query_executed(result_callback cb, resultset r, bool complete, connection_lock_ptr l)
{
	cb(l ? l : lock(), r, complete);
}

}  // namespace pg
} /* namespace db */
} /* namespace tip */
