/*
 * database.cpp
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/database.hpp>
#include <tip/db/pg/detail/database_impl.hpp>

#include <tip/db/pg/log.hpp>

#include <stdexcept>
#include <mutex>

namespace tip {
namespace db {
namespace pg {

LOCAL_LOGGING_FACILITY_CFG(PGSVC, config::SERVICE_LOG);

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
        pimpl_.reset(new detail::database_impl(pool_size, defaults));
    } else {
        pimpl_->set_defaults(pool_size, defaults);
    }
}

void
db_service::add_connection(std::string const& connection_string, optional_size pool_size)
{
    impl()->add_connection(connection_string, pool_size);
}

void
db_service::add_connection(connection_options const& co, optional_size pool_size)
{
    impl()->add_connection(co, pool_size);
}

void
db_service::begin(dbalias const& alias,
        transaction_callback const& result,
        error_callback const& error,
        transaction_mode const& mode)
{
    // TODO Wrap callbacks in strands
    impl()->get_connection(alias, result, error, mode);
}

transaction_ptr
db_service::begin(dbalias const& alias, transaction_mode const& mode)
{
    auto future = begin_async(alias, mode);
    return future.get();
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
    local_log(logger::INFO) << "Stop db service";
    if (pimpl_) {
        pimpl_->stop();
    }
    pimpl_.reset();
}

asio_config::io_service_ptr
db_service::io_service()
{
    return impl()->io_service();
}

}  // namespace pg
}  // namespace db
}  // namespace tip
