/*
 * connection_pool.h
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_CONNECTION_POOL_H_
#define TIP_DB_PG_DETAIL_CONNECTION_POOL_H_

#include <memory>

#include <boost/noncopyable.hpp>
#include <tip/db/pg/asio_config.hpp>

#include <tip/db/pg/database.hpp>

namespace tip {
namespace db {
namespace pg {

namespace detail {

/**
 * Container of connections to the same database
 */
class connection_pool : public ::std::enable_shared_from_this<connection_pool>,
        private boost::noncopyable {
public:
    using io_service_ptr        = asio_config::io_service_ptr;
    using connection_pool_ptr   = ::std::shared_ptr<connection_pool>;
public:
    static connection_pool_ptr
    create(io_service_ptr service, size_t pool_size,
            connection_options const& co,
            client_options_type const& = client_options_type());

    ~connection_pool();

    dbalias const&
    alias() const;

    void
    get_connection(transaction_callback const&, error_callback const&,
            transaction_mode const&);

    void
    close(simple_callback);
private:
    connection_pool(io_service_ptr service, size_t pool_size,
            connection_options const& co,
            client_options_type const&);

    void
    create_new_connection();
    void
    connection_ready(connection_ptr c);
    void
    connection_terminated(connection_ptr c);
    void
    connection_error(connection_ptr c, error::connection_error const& ec);

    void
    close_connections();
private:
    struct impl;
    using pimpl = ::std::unique_ptr<impl>;
    pimpl pimpl_;
};

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* TIP_DB_PG_DETAIL_CONNECTION_POOL_H_ */
