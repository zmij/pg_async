/*
 * fiber_round_robin.hpp
 *
 *  Created on: Mar 25, 2017
 *      Author: zmij
 */

#ifndef PUSHKIN_ASIO_FIBER_SHARED_WORK_HPP_
#define PUSHKIN_ASIO_FIBER_SHARED_WORK_HPP_

#include <pushkin/asio/asio_config.hpp>

#include <boost/asio/steady_timer.hpp>

#include <boost/fiber/scheduler.hpp>
#include <boost/fiber/context.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/condition_variable.hpp>
#include <boost/fiber/operations.hpp>

namespace psst {
namespace asio {
namespace fiber {

/**
 * Scheduling algorithm for fiber dispatcher for use with one
 * ::boost::asio::io_service in multiple threads.
 *
 * Usage synopsis:
 *
 * In each thread that should be running io_service loop together with
 * fiber dispatcher:
 *
 * @code
 * auto runner = ::psst::asio::fiber::use_shared_work_algorithm( io_svc );
 * runner->run();
 * @endcode
 *
 */

class runner {
public:
    using time_point            = ::std::chrono::steady_clock::time_point;

    using context               = ::boost::fibers::context;
    using context_type          = ::boost::fibers::type;
    using ready_queue_t         = ::boost::fibers::scheduler::ready_queue_t;

    using mutex_type            = ::boost::fibers::mutex;
    using condition_variable    = ::boost::fibers::condition_variable;
    using lock_type             = ::std::unique_lock<mutex_type>;
private:
    struct service : io_service::service {
        using mutex_type        = ::std::mutex;
        using lock_type         = ::std::unique_lock<mutex_type>;

        static io_service::id                   id;
        ::std::unique_ptr< io_service::work >   work_;

        mutex_type                              mtx_{};
        ready_queue_t                           queue_{};

        service(io_service& io_svc)
            : io_service::service(io_svc),
              work_{ new io_service::work{ io_svc } }
        {}

        virtual ~service() = default;
        service(service const&) = delete;
        service(service&&) = delete;
        service&
        operator =(service const&) = delete;
        service&
        operator =(service&&) = delete;

        void
        shutdown_service() override final
        {
            work_.reset();
        }
    };

public:
    runner(io_service_ptr io_svc)
        : io_svc_{io_svc},
          suspend_timer_{ *io_svc }
    {
        try {
            ::std::unique_ptr< service > svc{new service(*io_svc_)};
            ::asio_ns::add_service(*io_svc_, svc.get());
            svc.release();
        } catch (::asio_ns::service_already_exists const& e) {
            // Ignore it here
        }
        shared_ = &::asio_ns::use_service< service >(*io_svc_);
    }

    ~runner()
    {
    }

    void
    run()
    {
        while (!io_svc_->stopped()) {
            if (has_ready_fibers()) {
                while (io_svc_->poll());

                lock_type lock{mtx_};
                cnd_.wait(lock);
            } else {
                if (!io_svc_->run_one()) {
                    break;
                }
            }
        }
    }

    void
    stop()
    {
        io_svc_->stop();
        cnd_.notify_one();
    }

    bool
    has_ready_fibers() const noexcept override
    {
        service::lock_type lock{ shared_->mtx_ };
        return !shared_->queue_.empty() || !local_queue_.empty();
    }

    void
    awakened(context* ctx) noexcept override
    {
        if (ctx->is_context( context_type::pinned_context )) {
            ctx->ready_link(local_queue_);
        } else {
            ctx->detach();
            service::lock_type lock{ shared_->mtx_ };
            ctx->ready_link( shared_->queue_ );
        }
    }
    context*
    pick_next() noexcept override
    {
        context* ctx{ nullptr };
        service::lock_type lock{ shared_->mtx_ };
        if (!shared_->queue_.empty()) {
            ctx = &shared_->queue_.front();
            shared_->queue_.pop_front();
            lock.unlock();
            context::active()->attach(ctx);
        } else {
            lock.unlock();
            if (!local_queue_.empty()) {
                ctx = &local_queue_.front();
                local_queue_.pop_front();
            }
        }

        return ctx;
    }

    void
    suspend_until(time_point const& abs_time) noexcept
    {
        if (abs_time != time_point::max()) {
            suspend_timer_.expires_at(abs_time);
            suspend_timer_.async_wait(
                []( error_code const& )
                {
                    ::boost::this_fiber::yield();
                });
        }
        cnd_.notify_one();
    }

    void
    notify() noexcept
    {
        suspend_timer_.async_wait([]( error_code const& )
                {
                    ::boost::this_fiber::yield();
                });
        suspend_timer_.expires_at(::std::chrono::steady_clock::now());
    }
private:
    io_service_ptr                          io_svc_;
    ::asio_ns::steady_timer                 suspend_timer_;

    mutex_type                              mtx_{};
    condition_variable                      cnd_{};
    ready_queue_t                           local_queue_{};

    service*                                shared_{nullptr};
};
using runner_ptr = ::std::shared_ptr<runner>;


class shared_work : public ::boost::fibers::algo::algorithm {
public:
    using time_point            = ::std::chrono::steady_clock::time_point;
    using context               = ::boost::fibers::context;
public:
    shared_work(runner_ptr r)
        : pimpl_{ r } {}
    ~shared_work()
    {
    }

    void
    awakened(context* ctx) noexcept
    {
        pimpl_->awakened(ctx);
    }

    context*
    pick_next() noexcept
    {
        return pimpl_->pick_next();
    }

    bool
    has_ready_fibers() const noexcept
    {
        return pimpl_->has_ready_fibers();
    }

    void
    suspend_until(time_point const& abs_time) noexcept override
    {
        pimpl_->suspend_until(abs_time);
    }

    void
    notify() noexcept override
    {
        pimpl_->notify();
    }
private:
    ::std::shared_ptr<runner>                   pimpl_;
};

using runner_ptr = ::std::shared_ptr<runner>;

io_service::id runner::service::id;

runner_ptr
use_shared_work_algorithm(io_service_ptr io_svc)
{
    auto r = ::std::make_shared< runner >(io_svc);
    ::boost::fibers::use_scheduling_algorithm< shared_work >( r );
    return r;
}

} /* namespace fiber */
} /* namespace asio */
} /* namespace psst */


#endif /* PUSHKIN_ASIO_FIBER_SHARED_WORK_HPP_ */
