/**
 * @file tip/db/pg/query.hpp
 *
 *    @date Jul 11, 2015
 *  @author zmij
 */

#ifndef TIP_DB_PG_QUERY_HPP_
#define TIP_DB_PG_QUERY_HPP_

#include <tip/db/pg/common.hpp>

#include <memory>
#include <functional>

namespace tip {
namespace db {
namespace pg {

/**
 *  @brief Asynchronous query class.
 *
 *  Synopsis:
 *  @code
 *  // Simple query
 *  query (alias, "select * from pg_catalog.pg_tables")
 *  ([](result_ptr res, bool complete)
 *     {
 *         // process the query results
 *     },
 *     [](error_code const& ec)
 *     {
 *         // handle the error here
 *     });
 *     // Extended query
 *     query (alias, "select * from pg_catalog.pg_type where typelem = $1", 26)
 *     ([](result_ptr res, bool complete)
 *     {
 *         // process the query results
 *     },
 *     [](error_code const& ec)
 *     {
 *         // handle the error here
 *     });
 * @endcode
 *
 * @see @ref querying
 * @see @ref results
 */
class query {
public:
    /**
     * @brief Construct a query.
     *
     * Query will start a transaction in a connection with the alias.
     * @param alias Database connection alias.
     * @param expression SQL query expression
     */
    query(dbalias const& alias, std::string const& expression);
    /**
     * @brief Construct a query.
     *
     * Query will start a transaction in a connection with the alias. The
     * transaction will be started with the specified mode
     *
     * @param alias Database connection alias.
     * @param mode  Transaction mode
     * @param expression SQL query expression
     */
    query(dbalias const& alias, transaction_mode const& mode,
            std::string const& expression);

    /**
     * @brief Construct a prepared query with params to bind.
     *
     * Query will start a transaction in a connection with the alias.
     * @param alias database alias
     * @param expression SQL query expression
     * @param params parameters to bind
     * @tparam T query parameter types
     */
    template < typename ... T >
    query(dbalias const& alias, std::string const& expression,
            T const& ... params);

    /**
     * @brief Construct a prepared query with params to bind.
     *
     * Query will start a transaction in a connection with the alias. The
     * transaction will be started with the specified mode
     *
     * @param alias database alias
     * @param mode  Transaction mode
     * @param expression SQL query expression
     * @param params parameters to bind
     * @tparam T query parameter types
     */
    template < typename ... T >
    query(dbalias const& alias, transaction_mode const& mode,
            std::string const& expression, T const& ... params);

    /**
     * @brief Construct a query.
     * @pre Transaction must be started with a db_service::begin or another
     *         query.
     * @param tran transaction object pointer
     * @param expression SQL query expression
     */
    query(transaction_ptr tran, std::string const& expression);

    /**
     * @brief Construct a prepared query with params to bind
     * @pre Transaction must be started with a db_service::begin or another
     *         query.
     * @param tran transaction object pointer
     * @param expression SQL query expression
     * @param params parameters to bind
     * @tparam T query parameter types
     */
    template < typename ... T >
    query(transaction_ptr tran, std::string const& expression,
            T const& ... params);

    /**
     * @brief Bind parameters for the query
     * @pre Query constructed
     * @param params prepared query parameters
     * @tparam T query parameter types
     */
    template < typename ... T >
    query&
    bind(T const& ... params);

    /**
     * @brief Mark the query as prepared statement.
     */
    query&
    bind();
    /**
     * @brief Start running the query
     * @pre If a query was constructed with an alias - the database connection
     *         with the alias must be registered with the database service.
     * @pre Query has been constructed and parameters bound.
     * @param result result callback
     * @param error error callback
     * @see @ref callbacks
     */
    void
    run_async(query_result_callback const& result, error_callback const& error) const;
    /**
     * Shortcut for @ref tip::db::pg::query::run_async
     * @param result result callback
     * @param error error callback
     */
    void
    operator()(query_result_callback const& result, error_callback const& error) const;
private:
    using params_buffer = std::vector<byte>;
    struct impl;
    using pimpl = std::shared_ptr<impl>;

    type_oid_sequence&
    param_types();
    params_buffer&
    buffer();
    mutable pimpl pimpl_;
private:
    template < typename ... T >
    static pimpl
    create_impl(dbalias const&, transaction_mode const& mode,
            std::string const& expression, T const& ... params);

    static pimpl
    create_impl(dbalias const&, transaction_mode const& mode,
            std::string const& expression,
            type_oid_sequence&& param_types, params_buffer&& params);

    template < typename ... T >
    static pimpl
    create_impl(transaction_ptr, std::string const& expression,
            T const& ... params);

    static pimpl
    create_impl(transaction_ptr, std::string const& expression,
            type_oid_sequence&& param_types, params_buffer&& params);

};

}  // namespace pg
}  // namespace db
}  // namespace tip

#include <tip/db/pg/query.inl>

#endif /* TIP_DB_PG_QUERY_HPP_ */
