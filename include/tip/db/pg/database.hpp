/**
 *  @file tip/db/pg/database.hpp
 *
 *  @date Jul 10, 2015
 *  @author zmij
 */

#ifndef TIP_DB_PG_DATABASE_HPP_
#define TIP_DB_PG_DATABASE_HPP_

#include <string>
#include <memory>
#include <functional>
#include <map>
#include <future>
#include <boost/optional.hpp>

#include <tip/db/pg/asio_config.hpp>
#include <tip/db/pg/common.hpp>
#include <tip/db/pg/error.hpp>

namespace tip {
namespace db {
namespace pg {

namespace detail {
struct database_impl;
}
/**
 * Database connection manager.
 * Synopsis:
 * @code
 * // set up connection
 * database::add_connection(connection_string);
 * // get connection with a connection string
 * database::get_connection_async(
 *         connection_string,
 *         [](database::connection_ptr c) {
 *             // run queries here
 *         },
 *         [](error_code const&) {
 *             // handle the connection error here
 *         }
 * );
 * // get a connection created with alias
 * database::get_connection_async(
 *         dbalias,
 *         [](database::connection_ptr c) {
 *             // run queries here
 *         },
 *         [](error_code const&) {
 *             // handle the connection error here
 *         }
 * );
 * //
 * @endcode
 */
class db_service {
public:
    typedef std::map< std::string, std::string > connection_params;
    typedef boost::optional<size_t> optional_size;
public:
    static const size_t DEFAULT_POOOL_SIZE = 4;
public:
    /**
     * @brief Initialize the database service with the default pool_size per
     *         alias and default connection parameters.
     * @param pool_size number of connections per alias
     * @param defaults default settings for the connection
     */
    static void
    initialize(size_t pool_size, connection_params const& defaults);

    /**
     *    @brief Add a connection specification.
     *
     *    Requires an alias, for the database to be referenced by it later.
     *    @param connection_string
     *    @param pool_size A connection can have a pool size different from
     *             other connections.
     *    @throws tip::db::pg::error::connection_error if the connection string
     *            cannot be used.
     */
    static void
    add_connection(std::string const& connection_string,
            optional_size pool_size = optional_size());

    static void
    add_connection(connection_options const& co,
            optional_size pool_size = optional_size());
    /**
     *     @brief Create a connection or retrieve a connection from the connection pool
     *         and start a transaction.
     *
     *    Will lookup a connection by alias. If a new connection must be created,
     *     it will be created with the connection string associated with the alias.
     *    If there is an idle connection in the pool, will return it.
     *    If no idle connections are available, and the size of connection pool
     *    didn't reach it's limit, will create a new one.
     *    If the pool is full and no idle connections are available,
     *    will return the first connection that becomes idle.
     *
     *    @param alias database alias
     *    @param result callback function that will be called when a connection
     *             becomes available and transaction is started.
     *    @param error callback function that will be called in case of an error.
     *    @param isolation transaction isolation level
     *    @throws tip::db::pg::error::connection_error if the alias is not
     *          registered with the database service.
     */
    static void
    begin(dbalias const&, transaction_callback const&,
            error_callback const&, transaction_mode const& = transaction_mode{});

    /**
     * Wrap async call to begin to a future
     * @param
     * @param
     * @return
     */
    template < template <typename> class _Promise = ::std::promise >
    static auto
    begin_async(dbalias const& alias, transaction_mode const& mode = transaction_mode{})
        -> decltype(::std::declval<_Promise<transaction_ptr>>().get_future())
    {
        auto promise = ::std::make_shared<_Promise<transaction_ptr>>();

        begin(
            alias,
            [promise](transaction_ptr trx)
            {
                promise->set_value(trx);
            },
            [promise](error::db_error const& e)
            {
                promise->set_exception(::std::make_exception_ptr(e));
            }, mode
        );

        return promise->get_future();
    }

    static transaction_ptr
    begin(dbalias const& alias, transaction_mode const& mode = transaction_mode{});

    static void
    run();
    static void
    stop();

    static asio_config::io_service_ptr
    io_service();
private:
    // No instances
    db_service() {}

    typedef std::shared_ptr<detail::database_impl> pimpl;
    static pimpl pimpl_;

    static pimpl
    impl(size_t pool_size = DEFAULT_POOOL_SIZE,
            connection_params const& defaults = {});
};

}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* TIP_DB_PG_DATABASE_HPP_ */
