/**
 *    @file tip/pg/db/transaction.hpp
 *
 *  @date Jul 14, 2015
 *  @author zmij
 */

#ifndef TIP_DB_PG_DETAIL_CONNECTION_LOCK_HPP_
#define TIP_DB_PG_DETAIL_CONNECTION_LOCK_HPP_

#include <memory>
#include <functional>
#include <atomic>

#include <tip/db/pg/common.hpp>
#include <tip/db/pg/future_config.hpp>

namespace tip {
namespace db {
namespace pg {
class basic_connection;
namespace detail {
template < typename Mutex, typename TransportType, typename SharedType >
struct connection_fsm_def;
}  /* namespace detail */
/**
 *  @brief RAII transaction object.
 *
 *  It is created by the library internally when a transaction is started. It
 *  will rollback the transaction if it wasn't explicitly committed.
 *
 *     @warning A tip::db::pg::transaction object shoudn't be stored and accessed
 *     concurrently.
 */
class transaction : public std::enable_shared_from_this< transaction > {
public:
    using connection_ptr        = ::std::shared_ptr<basic_connection>;
    using pointer               = basic_connection*;
    using const_pointer         = basic_connection const*;

    using notification_callback = ::std::function< void() >;
    using atomic_flag           = ::std::atomic_flag;
public:
    transaction(connection_ptr);
    ~transaction();

    transaction(transaction const&) = delete;
    transaction&
    operator = (transaction const&) = delete;
    transaction(transaction&&) = delete;
    transaction&
    operator = (transaction&&) = delete;

    //@{
    /** Access to underlying connection object */
    pointer
    operator-> ()
    {
        return connection_.get();
    }
    const_pointer
    operator-> () const
    {
        return connection_.get();
    }

    pointer
    operator* ()
    {
        return connection_.get();
    }
    const_pointer
    operator* () const
    {
        return connection_.get();
    }
    //@}

    dbalias const&
    alias() const;

    bool
    in_transaction() const;

    void
    commit_async(notification_callback = notification_callback(),
            error_callback = error_callback());
    template < template<typename> class _Promise = promise >
    auto
    commit_future() -> decltype(::std::declval<_Promise<void>>().get_future())
    {
        auto promise = ::std::make_shared<_Promise<void>>();
        commit_async(
        [promise]()
        {
            promise->set_value();
        },
        [promise](error::db_error const& err)
        {
            promise->set_exception(::std::make_exception_ptr(err));
        }
        );
        return promise->get_future();
    }
    template < template<typename> class _Promise = promise >
    void
    commit()
    {
        auto future = commit_future();
        future.get();
    }

    void
    rollback_async(notification_callback = notification_callback(),
            error_callback = error_callback());
    template < template<typename> class _Promise = promise >
    auto
    rollback_future() -> decltype(::std::declval<_Promise<void>>().get_future())
    {
        auto promise = ::std::make_shared<_Promise<void>>();
        rollback_async(
        [promise]()
        {
            promise->set_value();
        },
        [promise](error::db_error const& err)
        {
            promise->set_exception(::std::make_exception_ptr(err));
        }
        );
        return promise->get_future();
    }
    template < template<typename> class _Promise = promise >
    void
    rollback()
    {
        auto future = rollback_future();
        future.get();
    }

    void
    execute(std::string const& query, query_result_callback,
            query_error_callback);
    void
    execute(std::string const& query, type_oid_sequence const& param_types,
            std::vector< byte > params_buffer,
            query_result_callback, query_error_callback);
private:
    template < typename Mutex, typename TransportType, typename SharedType >
    friend struct detail::connection_fsm_def;
    void
    mark_done()
    { finished_.test_and_set(); }
    void
    handle_results(resultset, bool, query_result_callback);
    void
    handle_query_error(error::query_error const&, query_error_callback);
    connection_ptr  connection_;
    atomic_flag     finished_;
};

} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* TIP_DB_PG_DETAIL_CONNECTION_LOCK_HPP_ */
