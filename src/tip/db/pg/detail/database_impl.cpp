/*
 * database_impl.cpp
 *
 *  Created on: Jul 13, 2015
 *      Author: zmij
 */

#include <tip/db/pg/detail/database_impl.hpp>
#include <tip/db/pg/detail/connection_pool.hpp>
#include <stdexcept>

#ifdef WITH_TIP_LOG
#include <tip/version.hpp>
#endif
#include <tip/db/pg/log.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

#ifdef WITH_TIP_LOG
namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGDB";
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

database_impl::database_impl(size_t pool_size, connection_params const& defaults)
	: pool_size_(pool_size), defaults_(defaults)
{
	#ifdef WITH_TIP_LOG
	local_log() << "Initializing postgre db service version " << tip::VERSION;
	#endif
}

database_impl::~database_impl()
{
	stop();
	#ifdef WITH_TIP_LOG
	local_log() << "* database_impl::~database_impl";
	#endif
}

void
database_impl::set_defaults(size_t pool_size, connection_params const& defaults)
{
	pool_size_ = pool_size;
	defaults_ = defaults;
}

void
database_impl::add_connection(std::string const& connection_string,
		db_service::optional_size pool_size,
		connection_params const& params)
{
	connection_options co = connection_options::parse(connection_string);
	if (co.uri.empty())
		throw std::runtime_error("No URI in database connection string");

	if (co.database.empty())
		throw std::runtime_error("No database name in database connection string");

	if (co.user.empty())
		throw std::runtime_error("No user name in database connection string");

	if (co.alias.value.empty()) {
		co.generate_alias();
	}

	add_pool(co, pool_size, params);
}

database_impl::connection_pool_ptr
database_impl::add_pool(connection_options const& co, db_service::optional_size pool_size,
		connection_params const& params)
{
	if (!connections_.count(co.alias)) {
		if (!pool_size.is_initialized()) {
			pool_size = pool_size_;
		}
		#ifdef WITH_TIP_LOG
		local_log(logger::INFO) << "Create a new connection pool " << co.alias.value;
		#endif
		connection_params parms(params);
		for (auto p : defaults_) {
			if (!parms.count(p.first)) {
				parms.insert(p);
			}
		}
		#ifdef WITH_TIP_LOG
		local_log(logger::INFO) << "Register new connection " << co.uri
				<< "[" << co.database << "]" << " with alias " << co.alias;
		#endif
		connections_.insert(std::make_pair(co.alias,
				connection_pool_ptr(connection_pool::create(service_,
						*pool_size, co, parms) )));
	}
	return connections_[co.alias];
}

void
database_impl::get_connection(std::string const& connection_string,
		connection_lock_callback cb,
		error_callback err)
{
	connection_options co = connection_options::parse(connection_string);
	if (co.uri.empty())
		throw std::runtime_error("No URI in database connection string");

	if (co.database.empty())
		throw std::runtime_error("No database name in database connection string");

	if (co.user.empty())
		throw std::runtime_error("No user name in database connection string");

	if (co.alias.value.empty()) {
		co.generate_alias();
	}
	connection_pool_ptr pool = add_pool(co);
	pool->get_connection(cb, err);
}

void
database_impl::get_connection(dbalias const& alias,
		connection_lock_callback cb,
		error_callback err)
{
	if (!connections_.count(alias)) {
		// FIXME Create a specific exception
		throw std::runtime_error("Database alias is not registered");
	}
	connection_pool_ptr pool = connections_[alias];
	pool->get_connection(cb, err);
}

void
database_impl::run()
{
	service_.run();
}

void
database_impl::stop()
{
	for (auto c: connections_) {
		c.second->close();
	}
	connections_.clear();

	if (!service_.stopped()) {
		service_.stop();
		service_.reset();
	}
}

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */
