/*
 * database_impl.cpp
 *
 *  Created on: Jul 13, 2015
 *      Author: zmij
 */

#include <tip/db/pg/detail/database_impl.hpp>
#include <tip/db/pg/detail/connection_pool.hpp>
#include <tip/db/pg/error.hpp>
#include <stdexcept>

#include <tip/db/pg/version.hpp>
#include <tip/db/pg/log.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

LOCAL_LOGGING_FACILITY_CFG(PGDB, config::SERVICE_LOG);

database_impl::database_impl(size_t pool_size, client_options_type const& defaults)
    : service_( std::make_shared<asio_config::io_service>() ),
      pool_size_(pool_size), defaults_(defaults),
      state_(running)
{
    local_log() << "Initializing postgre db service";
}

database_impl::~database_impl()
{
    stop();
}

void
database_impl::set_defaults(size_t pool_size, client_options_type const& defaults)
{
    pool_size_ = pool_size;
    defaults_ = defaults;
}

void
database_impl::add_connection(std::string const& connection_string,
        db_service::optional_size pool_size,
        client_options_type const& params)
{
    if (state_ != running)
        throw error::connection_error("Database service is not running");
    connection_options co = connection_options::parse(connection_string);
    add_connection(co, pool_size, params);
}

void
database_impl::add_connection(connection_options co,
        db_service::optional_size pool_size,
        client_options_type const& params)
{
    if (state_ != running)
        throw error::connection_error("Database service is not running");

    if (co.uri.empty())
        throw error::connection_error("No URI in database connection string");

    if (co.database.empty())
        throw error::connection_error("No database name in database connection string");

    if (co.user.empty())
        throw error::connection_error("No user name in database connection string");

    if (co.alias.empty()) {
        co.generate_alias();
    }

    add_pool(co, pool_size, params);
}

database_impl::connection_pool_ptr
database_impl::add_pool(connection_options const& co,
        db_service::optional_size pool_size,
        client_options_type const& params)
{
    if (!connections_.count(co.alias)) {
        if (!pool_size.is_initialized()) {
            pool_size = pool_size_;
        }
        local_log(logger::INFO) << "Create a new connection pool " << co.alias;
        client_options_type parms(params);
        for (auto p : defaults_) {
            if (!parms.count(p.first)) {
                parms.insert(p);
            }
        }
        local_log(logger::INFO) << "Register new connection " << co.uri
                << "[" << co.database << "]" << " with alias " << co.alias;
        connections_.insert(std::make_pair(co.alias,
                connection_pool_ptr(connection_pool::create(service_,
                        *pool_size, co, parms) )));
    }
    return connections_[co.alias];
}

void
database_impl::get_connection(dbalias const& alias,
        transaction_callback const& cb,
        error_callback const& err,
        transaction_mode const& mode)
{
    if (state_ != running)
        throw error::connection_error("Database service is not running");

    if (!connections_.count(alias)) {
        throw error::connection_error("Database alias is not registered");
    }
    connection_pool_ptr pool = connections_[alias];
    pool->get_connection(cb, err, mode);
}

void
database_impl::run()
{
    service_->run();
}

void
database_impl::stop()
{
    if (state_ == running) {
        state_ = closing;
        std::shared_ptr< size_t > pool_count =
                std::make_shared< size_t >(connections_.size());
        asio_config::io_service_ptr svc = service_;

        for (auto c: connections_) {
            // FIXME Pass a close callback. Call stop
            // only when all connections are closed, may be with some timeout
            c.second->close(
            [pool_count, svc](){
                --(*pool_count);
                if (*pool_count == 0) {
                    svc->stop();
                }
            });
        }
        connections_.clear();
    }
}

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */
