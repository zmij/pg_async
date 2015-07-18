/*
 * database.cpp
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/database.hpp>
#include <tip/db/pg/detail/database_impl.hpp>

#include <tip/log/log.hpp>
#include <tip/log/ansi_colors.hpp>

#include <stdexcept>
#include <mutex>

namespace tip {
namespace db {
namespace pg {

namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGSVC";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
// For more convenient changing severity, eg local_log(logger::WARNING)
using tip::log::logger;

typedef std::recursive_mutex mutex_type;
typedef std::lock_guard<mutex_type> lock_type;

namespace {
mutex_type&
db_service_lock()
{
	static mutex_type _mtx;
	return _mtx;
}

}  // namespace

db_service::pimpl db_service::pimpl_;

db_service::pimpl
db_service::impl(size_t pool_size, connection_params const& defaults)
{
	lock_type lock(db_service_lock());
	if (!pimpl_) {
		pimpl_.reset(new detail::database_impl(pool_size, defaults));
	}
	return pimpl_;
}

void
db_service::initialize(size_t pool_size, connection_params const& defaults)
{
	lock_type lock(db_service_lock());
	if (!pimpl_) {
		local_log() << "Create new impl";
		pimpl_.reset(new detail::database_impl(pool_size, defaults));
	} else {
		pimpl_->set_defaults(pool_size, defaults);
	}
}

void
db_service::add_connection(std::string const& connection_string, optional_size)
{
	impl()->add_connection(connection_string);
}

void
db_service::get_connection_async(std::string const& connection_string,
		connection_lock_callback result, error_callback error)
{
	impl()->get_connection(connection_string, result, error);
}

void
db_service::get_connection_async(dbalias const& alias,
		connection_lock_callback result,
		error_callback error)
{
	impl()->get_connection(alias, result, error);
}

void
db_service::run()
{
	impl()->run();
}

void
db_service::stop()
{
	lock_type lock(db_service_lock());
	if (pimpl_) {
		pimpl_->stop();
	}
	pimpl_.reset();
}

boost::asio::io_service&
db_service::io_service()
{
	local_log() << "db_service::io_service";
	return impl()->io_service();
}

}  // namespace pg
}  // namespace db
}  // namespace tip
