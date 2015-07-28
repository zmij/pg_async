/*
 * connection_pool.cpp
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/detail/connection_pool.hpp>
#include <tip/db/pg/detail/connection_lock.hpp>
#include <tip/db/pg/common.hpp>
#include <tip/db/pg/error.hpp>

#include <tip/db/pg/log.hpp>

#include <stdexcept>
#include <algorithm>
#include <boost/bind.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

#ifdef WITH_TIP_LOG
namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGPOOL";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
// For more convenient changing severity, eg local_log(logger::WARNING)
using tip::log::logger;
#endif

connection_pool::connection_pool(io_service& service,
		size_t pool_size,
		connection_options const& co,
		connection_params const& params)
	: service_(service), pool_size_(pool_size),
	  co_(co), params_(params), closed_(false)
{
	if (pool_size_ == 0)
		throw std::runtime_error("Database connection pool size cannot be zero");

	if (co_.uri.empty())
		throw std::runtime_error("No URI in database connection string");

	if (co_.database.empty())
		throw std::runtime_error("No database name in database connection string");

	if (co_.user.empty())
		throw std::runtime_error("No user name in database connection string");
}

connection_pool::~connection_pool()
{
	#ifdef WITH_TIP_LOG
	local_log(logger::DEBUG) << "*** connection_pool::~connection_pool()";
	#endif
}

connection_pool::connection_pool_ptr
connection_pool::create(io_service& service,
		size_t pool_size,
		connection_options const& co,
		connection_params const& params)
{
	connection_pool_ptr pool(new connection_pool( service, pool_size, co, params ));
	pool->create_new_connection();
	return pool;
}

void
connection_pool::create_new_connection()
{
	if (closed_)
		return;
	#ifdef WITH_TIP_LOG
	{
		auto local = local_log(logger::INFO);
		local << "Create new "
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< alias().value
				<< logger::severity_color()
				<< " connection";
	}
	#endif
	connection_ptr conn(connection::create(service_,
			boost::bind(&connection_pool::connection_ready, shared_from_this(), _1),
			boost::bind(&connection_pool::connection_terminated, shared_from_this(), _1),
			boost::bind(&connection_pool::connection_error, shared_from_this(), _1, _2),
			co_, params_));

	connections_.push_back(conn);
	#ifdef WITH_TIP_LOG
	{
		auto local = local_log();
		local
			<< (util::CLEAR) << (util::RED | util::BRIGHT)
			<< alias().value
			<< logger::severity_color()
			<< " pool size " << connections_.size();
	}
	#endif
}

void
connection_pool::connection_ready(connection_ptr c)
{
	if (c->state() == connection::IDLE) {
		#ifdef WITH_TIP_LOG
		{
			auto local = local_log();
			local << "Connection "
					<< (util::CLEAR) << (util::RED | util::BRIGHT)
					<< alias().value
					<< logger::severity_color()
					<< " ready";
		}
		#endif
		lock_type lock(mutex_);

		if (!waiting_.empty()) {
			request_callbacks req = waiting_.front();
			waiting_.pop();
			#ifdef WITH_TIP_LOG
			local_log()
					<< (util::CLEAR) << (util::RED | util::BRIGHT)
					<< alias().value
					<< logger::severity_color()
					<< " queue size " << waiting_.size() << " (dequeue)";
			#endif
			req.first(c->lock());
		} else {
			#ifdef WITH_TIP_LOG
			local_log()
					<< (util::CLEAR) << (util::RED | util::BRIGHT)
					<< alias().value
					<< logger::severity_color()
					<< " queue size " << waiting_.size();
			#endif
			ready_connections_.push(c);
		}
	}
}

void
connection_pool::connection_terminated(connection_ptr c)
{
	#ifdef WITH_TIP_LOG
	{
		auto local = local_log(logger::INFO);
		local << "Connection "
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< alias().value
				<< logger::severity_color()
				<< " gracefully terminated";
	}
	#endif
	lock_type lock(mutex_);
	auto f = std::find(connections_.begin(), connections_.end(), c);
	if (f != connections_.end()) {
		connections_.erase(f);
	}

	#ifdef WITH_TIP_LOG
	{
		auto local = local_log();
		local
			<< (util::CLEAR) << (util::RED | util::BRIGHT)
			<< alias().value
			<< logger::severity_color()
			<< " pool size " << connections_.size();
	}
	#endif
}

void
connection_pool::connection_error(connection_ptr c, class connection_error const& ec)
{
	#ifdef WITH_TIP_LOG
	local_log(logger::ERROR) << "Connection " << alias().value << " error: "
			<< ec.what();
	#endif
	if (c->state() == connection::DISCONNECTED) {
		lock_type lock(mutex_);
		#ifdef WITH_TIP_LOG
		local_log() << "Erase connection from the connection pool";
		#endif
		auto f = std::find(connections_.begin(), connections_.end(), c);
		if (f != connections_.end()) {
			connections_.erase(f);
		}
		while (!waiting_.empty()) {
			request_callbacks req = waiting_.front();
			waiting_.pop();
			req.second(ec);
		}
	} else {
		// FIXME Handle the error
	}
}

void
connection_pool::get_connection(transaction_callback const& conn_cb,
		error_callback const& err)
{
	// TODO Call the error callback if the pool is closed
	lock_type lock(mutex_);
	if (!ready_connections_.empty()) {
		#ifdef WITH_TIP_LOG
		local_log() << "Connection to "
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< alias().value
				<< logger::severity_color()
				<< " is ready";
		#endif
		connection_ptr conn = ready_connections_.front();
		ready_connections_.pop();
		conn_cb(conn->lock());
	} else {
		#ifdef WITH_TIP_LOG
		local_log()
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< alias().value
				<< logger::severity_color()
				<< " queue size " << waiting_.size() + 1  << " (enqueue)";;
		#endif
		if (connections_.size() < pool_size_) {
			create_new_connection();
		}
		waiting_.push(std::make_pair(conn_cb, err));
	}
}

void
connection_pool::close()
{
	lock_type lock(mutex_);
	closed_ = true;

	#ifdef WITH_TIP_LOG
	local_log() << "Close connection pool "
			<< (util::CLEAR) << (util::RED | util::BRIGHT)
			<< alias().value
			<< logger::severity_color();
	#endif
	connections_container copy = connections_;
	for (auto c : copy) {
		c->terminate();
	}
}

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */
