/*
 * connection_pool.cpp
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/detail/connection_pool.hpp>
#include <tip/db/pg/detail/basic_connection.hpp>
#include <tip/db/pg/transaction.hpp>
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

connection_pool::connection_pool(io_service& service,
		size_t pool_size,
		connection_options const& co,
		client_options_type const& params)
	: service_(service), pool_size_(pool_size),
	  co_(co), params_(params), closed_(false)
{
	if (pool_size_ == 0)
		throw error::connection_error("Database connection pool size cannot be zero");

	if (co_.uri.empty())
		throw error::connection_error("No URI in database connection string");

	if (co_.database.empty())
		throw error::connection_error("No database name in database connection string");

	if (co_.user.empty())
		throw error::connection_error("No user name in database connection string");
	local_log() << "Connection pool max size " << pool_size;
}

connection_pool::~connection_pool()
{
	local_log(logger::DEBUG) << "*** connection_pool::~connection_pool()";
}

connection_pool::connection_pool_ptr
connection_pool::create(io_service& service,
		size_t pool_size,
		connection_options const& co,
		client_options_type const& params)
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
	{
		auto local = local_log(logger::INFO);
		local << "Create new "
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< alias().value
				<< logger::severity_color()
				<< " connection";
	}
	connection_ptr conn = basic_connection::create(service_, co_, params_, {
			boost::bind(&connection_pool::connection_ready, shared_from_this(), _1),
			boost::bind(&connection_pool::connection_terminated, shared_from_this(), _1),
			boost::bind(&connection_pool::connection_error, shared_from_this(), _1, _2),
	});

	connections_.push_back(conn);
	{
		auto local = local_log();
		local
			<< (util::CLEAR) << (util::RED | util::BRIGHT)
			<< alias().value
			<< logger::severity_color()
			<< " pool size " << connections_.size();
	}
}

void
connection_pool::connection_ready(connection_ptr c)
{
	{
		auto local = local_log();
		local << "Connection "
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< alias().value
				<< logger::severity_color()
				<< " ready";
	}
	lock_type lock(mutex_);

	if (!waiting_.empty()) {
		request_callbacks req = waiting_.front();
		waiting_.pop();
		local_log()
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< alias().value
				<< logger::severity_color()
				<< " queue size " << waiting_.size() << " (dequeue)";
		c->begin({req.first, req.second});
	} else {
		local_log()
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< alias().value
				<< logger::severity_color()
				<< " queue size " << waiting_.size();
		ready_connections_.push(c);
	}
}

void
connection_pool::connection_terminated(connection_ptr c)
{
	{
		auto local = local_log(logger::INFO);
		local << "Connection "
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< alias().value
				<< logger::severity_color()
				<< " gracefully terminated";
	}
	lock_type lock(mutex_);
	auto f = std::find(connections_.begin(), connections_.end(), c);
	if (f != connections_.end()) {
		connections_.erase(f);
	}

	{
		auto local = local_log();
		local
			<< (util::CLEAR) << (util::RED | util::BRIGHT)
			<< alias().value
			<< logger::severity_color()
			<< " pool size " << connections_.size();
	}
}

void
connection_pool::connection_error(connection_ptr c, error::connection_error const& ec)
{
	local_log(logger::ERROR) << "Connection " << alias().value << " error: "
			<< ec.what();
	lock_type lock(mutex_);
	local_log() << "Erase connection from the connection pool";
	auto f = std::find(connections_.begin(), connections_.end(), c);
	if (f != connections_.end()) {
		connections_.erase(f);
	}
	while (!waiting_.empty()) {
		request_callbacks req = waiting_.front();
		waiting_.pop();
		req.second(ec);
	}
}

void
connection_pool::get_connection(transaction_callback const& conn_cb,
		error_callback const& err)
{
	// TODO Call the error callback if the pool is closed
	lock_type lock(mutex_);
	if (!ready_connections_.empty()) {
		local_log() << "Connection to "
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< alias().value
				<< logger::severity_color()
				<< " is ready";
		connection_ptr conn = ready_connections_.front();
		ready_connections_.pop();
		conn->begin({conn_cb, err});
	} else {
		local_log()
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< alias().value
				<< logger::severity_color()
				<< " queue size " << waiting_.size() + 1  << " (enqueue)";;
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

	local_log() << "Close connection pool "
			<< (util::CLEAR) << (util::RED | util::BRIGHT)
			<< alias().value
			<< logger::severity_color();
	connections_container copy = connections_;
	for (auto c : copy) {
		c->terminate();
	}
}

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */
