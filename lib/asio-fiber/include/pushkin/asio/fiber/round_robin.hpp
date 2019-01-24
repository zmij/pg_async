/*
 * fiber_round_robin.hpp
 *
 *  Created on: Mar 25, 2017
 *      Author: zmij
 */

#ifndef PUSHKIN_ASIO_FIBER_ROUND_ROBIN_HPP_
#define PUSHKIN_ASIO_FIBER_ROUND_ROBIN_HPP_

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

class round_robin : public ::boost::fibers::algo::algorithm {
    using time_point            = ::std::chrono::steady_clock::time_point;

    using context               = ::boost::fibers::context;
    using ready_queue_t         = ::boost::fibers::scheduler::ready_queue_t;
    using mutex_type            = ::boost::fibers::mutex;
    using condition_variable    = ::boost::fibers::condition_variable;

    using lock_type             = ::std::unique_lock<mutex_type>;

    io_service_ptr                              io_svc_;
    ::asio_ns::steady_timer                     suspend_timer_;

    ready_queue_t                               rqueue_{};
    mutex_type                                  mtx_{};
    condition_variable                          cnd_{};

    ::std::size_t                               counter_{ 0 };
public:
    struct service : io_service::service {
        static io_service::id                   id;
        ::std::unique_ptr< io_service::work >   work_;

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

    round_robin(io_service_ptr io_svc)
        : io_svc_{io_svc},
          suspend_timer_{*io_svc}
    {
        // TODO Replace with try
        if (!::asio_ns::has_service< service >(*io_svc_)) {
            ::asio_ns::add_service(*io_svc_, new service(*io_svc_));
        }
        // Post the lambda
        io_svc_->post(
        [this]() mutable
        {
            while (!io_svc_->stopped()) {
                if (has_ready_fibers()) {
                    while (io_svc_->poll());

                    lock_type lock{mtx_};
                    cnd_.wait_for(lock, ::std::chrono::milliseconds{10});
                } else {
                    if (!io_svc_->run_one()) {
                        break;
                    }
                }
            }
        });
    }

    void
    awakened(context* ctx) noexcept
    {
        // TODO Check the contexts
        ctx->ready_link(rqueue_);
        if (!ctx->is_context( ::boost::fibers::type::dispatcher_context )) {
            ++counter_;
        }
    }

    context*
    pick_next() noexcept
    {
        context* ctx{ nullptr };
        if (!rqueue_.empty()) {
            ctx = &rqueue_.front();
            rqueue_.pop_front();
            if (!ctx->is_context( ::boost::fibers::type::dispatcher_context )) {
                --counter_;
            }
        }
        return ctx;
    }

    bool
    has_ready_fibers() const noexcept
    {
        return counter_ > 0;
    }

    void
    suspend_until(time_point const& abs_time) noexcept override
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
    notify() noexcept override
    {
        suspend_timer_.async_wait([]( error_code const& )
                {
                    ::boost::this_fiber::yield();
                });
        suspend_timer_.expires_at(::std::chrono::steady_clock::now());
    }
};

io_service::id round_robin::service::id;

} /* namespace fiber */
} /* namespace asio */
} /* namespace psst */


#endif /* PUSHKIN_ASIO_FIBER_ROUND_ROBIN_HPP_ */
