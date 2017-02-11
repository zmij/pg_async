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
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>

namespace tip {
namespace db {
namespace pg {
namespace detail {

LOCAL_LOGGING_FACILITY_CFG(PGPOOL, config::CONNECTION_LOG);

struct connection_pool::impl {
    using connections_container = ::std::vector<connection_ptr>;
    using connections_queue     = ::std::queue<connection_ptr>;

    using request_callbacks_queue = ::std::queue< events::begin >;

    using mutex_type            = ::std::recursive_mutex;
    using lock_type             = ::std::lock_guard<mutex_type>;

    using atomic_flag           = ::std::atomic_bool;

    io_service_ptr          service_;
    size_t                  pool_size_;
    connection_options      co_;
    client_options_type     params_;

    mutex_type              event_mutex_;
    mutex_type              conn_mutex_;

    connections_container   connections_;
    connections_queue       ready_connections_;

    request_callbacks_queue queue_;

    atomic_flag             closed_;
    simple_callback         closed_callback_;

    impl(io_service_ptr service,
        size_t pool_size,
        connection_options const& co,
        client_options_type const& params)
    : service_(service),
      pool_size_(pool_size),
      co_(co),
      params_(params),
      closed_(false)
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

    dbalias const&
    alias() const
    { return co_.alias; }

    //@{
    /** @name Connection granular work */
    bool
    get_idle_connection(connection_ptr& conn)
    {
        if (closed_)
            return false;
        lock_type lock{conn_mutex_};
        if (!ready_connections_.empty()) {
            conn = ready_connections_.front();
            ready_connections_.pop();
            return true;
        }
        return false;
    }
    void
    add_idle_connection(connection_ptr conn)
    {
        if (!closed_) {
            lock_type lock{conn_mutex_};
            ready_connections_.push(conn);
            local_log() << alias() << " idle connections " << ready_connections_.size();
        }
    }
    void
    erase_connection(connection_ptr conn)
    {
        local_log() << "Erase connection from the connection pool";
        lock_type lock{conn_mutex_};
        auto f = std::find(connections_.begin(), connections_.end(), conn);
        if (f != connections_.end()) {
            connections_.erase(f);
        }
    }
    //@}

    //@{
    /** @name Event queue */
    bool
    next_event(events::begin& evt)
    {
        lock_type lock{event_mutex_};
        if (!queue_.empty()) {
            local_log()
                    << (util::CLEAR) << (util::RED | util::BRIGHT)
                    << alias()
                    << logger::severity_color()
                    << " queue size " << queue_.size() << " (dequeue)";
            evt = queue_.front();
            queue_.pop();
            return true;
        }
        return false;
    }
    void
    enqueue_event(events::begin&& evt)
    {
        lock_type lock{event_mutex_};
        queue_.push(::std::move(evt));

        local_log()
                << (util::CLEAR) << (util::RED | util::BRIGHT)
                << alias()
                << logger::severity_color()
                << " queue size " << queue_.size() << " (enqueue)";
    }
    void
    clear_queue(error::connection_error const& ec)
    {
        lock_type lock(event_mutex_);
        while (!queue_.empty()) {
            auto req = queue_.front();
            queue_.pop();
            if (req.error) {
                req.error(ec);
            }
        }
    }
    //@}

    void
    create_new_connection(connection_pool_ptr pool)
    {
        if (closed_)
            return;
        {
            local_log(logger::INFO)
                    << "Create new "
                    << (util::CLEAR) << (util::RED | util::BRIGHT)
                    << alias()
                    << logger::severity_color()
                    << " connection";
        }
        connection_ptr conn = basic_connection::create(
            service_, co_, params_,
            {
                [pool](connection_ptr c)
                { pool->connection_ready(c); },
                [pool](connection_ptr c)
                { pool->connection_terminated(c); },
                [pool](connection_ptr c, error::connection_error const& ec)
                { pool->connection_error(c, ec); }
            });

        {
            lock_type lock{conn_mutex_};
            connections_.push_back(conn);

            local_log()
                << (util::CLEAR) << (util::RED | util::BRIGHT)
                << alias()
                << logger::severity_color()
                << " pool size " << connections_.size();
        }
    }

    void
    connection_ready(connection_ptr c)
    {
        {
            local_log()
                << "Connection "
                << (util::CLEAR) << (util::RED | util::BRIGHT)
                << alias()
                << logger::severity_color()
                << " ready";
        }

        events::begin evt;
        if (next_event(evt)) {
            c->begin(::std::move(evt));
        } else {
            if (closed_) {
                close_connections();
            } else {
                add_idle_connection(c);
            }
        }
    }

    void
    connection_terminated(connection_ptr c)
    {
        {
            local_log(logger::INFO)
                    << "Connection "
                    << (util::CLEAR) << (util::RED | util::BRIGHT)
                    << alias()
                    << logger::severity_color()
                    << " gracefully terminated";
        }
        {
            erase_connection(c);

            if (connections_.empty() && closed_ && closed_callback_) {
                closed_callback_();
            }
        }

        {
            local_log()
                << (util::CLEAR) << (util::RED | util::BRIGHT)
                << alias()
                << logger::severity_color()
                << " pool size " << connections_.size();
        }
    }

    void
    connection_error(connection_ptr c, error::connection_error const& ec)
    {
        local_log(logger::ERROR) << "Connection " << alias() << " error: "
                << ec.what();
        erase_connection(c);
        clear_queue(ec);
    }

    void
    get_connection(transaction_callback const& conn_cb,
        error_callback const& err,
        transaction_mode const& mode,
        connection_pool_ptr pool)
    {
        if (closed_) {
            err( error::connection_error("Connection pool is closed") );
            return;
        }
        connection_ptr conn;
        if (get_idle_connection(conn)) {
            local_log() << "Connection to "
                    << (util::CLEAR) << (util::RED | util::BRIGHT)
                    << alias()
                    << logger::severity_color()
                    << " is idle";
            conn->begin({conn_cb, err, mode});
        } else {
            if (!closed_ && connections_.size() < pool_size_) {
                create_new_connection(pool);
            }
            enqueue_event({conn_cb, err, mode});
        }
    }

    void
    close(simple_callback close_cb)
    {
        bool expected = false;
        if (closed_.compare_exchange_strong(expected, true)) {
            closed_callback_ = close_cb;

            lock_type lock{event_mutex_};
            if (queue_.empty()) {
                close_connections();
            } else {
                local_log() << "Wait for outstanding tasks to finish";
            }
        }
    }

    void
    close_connections()
    {
        local_log() << "Close connection pool "
                << (util::CLEAR) << (util::RED | util::BRIGHT)
                << alias()
                << logger::severity_color()
                << " pool size " << connections_.size();
        if (connections_.size() > 0) {
            lock_type lock(conn_mutex_);
            connections_container copy = connections_;
            for ( auto c : copy ) {
                c->terminate();
            }
        } else if (closed_callback_) {
            closed_callback_();
        }
    }
};

connection_pool::connection_pool(io_service_ptr service,
        size_t pool_size,
        connection_options const& co,
        client_options_type const& params)
    : pimpl_(new impl(service, pool_size, co, params))
{
}

connection_pool::~connection_pool()
{
    local_log(logger::DEBUG) << "*** connection_pool::~connection_pool()";
}

dbalias const&
connection_pool::alias() const
{
    return pimpl_->alias();
}

connection_pool::connection_pool_ptr
connection_pool::create(io_service_ptr service,
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
    auto _this = shared_from_this();
    pimpl_->create_new_connection(_this);
}

void
connection_pool::connection_ready(connection_ptr c)
{
    pimpl_->connection_ready(c);
}

void
connection_pool::connection_terminated(connection_ptr c)
{
    pimpl_->connection_terminated(c);
}

void
connection_pool::connection_error(connection_ptr c, error::connection_error const& ec)
{
    pimpl_->connection_error(c, ec);
}

void
connection_pool::get_connection(transaction_callback const& conn_cb,
        error_callback const& err, transaction_mode const& mode)
{
    auto _this = shared_from_this();
    pimpl_->get_connection(conn_cb, err, mode, _this);
}

void
connection_pool::close(simple_callback close_cb)
{
    pimpl_->close(close_cb);
}

void
connection_pool::close_connections()
{
    pimpl_->close_connections();
}

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */
