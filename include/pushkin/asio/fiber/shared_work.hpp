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

class shared_work : public ::boost::fibers::algo::algorithm {
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

    struct impl {
        mutex_type                              mtx_{};
        condition_variable                      cnd_{};
        ready_queue_t                           local_queue_{};

        service*                                shared_{nullptr};

        bool
        has_ready_fibers() const noexcept
        {
            service::lock_type lock{ shared_->mtx_ };
            return !shared_->queue_.empty() || !local_queue_.empty();
        }
    };
public:
    shared_work(io_service_ptr io_svc)
        : io_svc_{io_svc},
          suspend_timer_{*io_svc},
          pimpl_{ ::std::make_shared<impl>() }
    {
        try {
            ::std::unique_ptr< service > svc{new service(*io_svc_)};
            ::asio_ns::add_service(*io_svc_, svc.get());
            svc.release();
        } catch (::asio_ns::service_already_exists const& e) {
            // Ignore it here
        }
        pimpl_->shared_ = &::asio_ns::use_service< service >(*io_svc_);
        auto pimpl = pimpl_;
        // Post the lambda
        io_svc_->post(
        [io_svc, pimpl]() mutable
        {
            while (!io_svc->stopped()) {
                if (pimpl->has_ready_fibers()) {
                    while (io_svc->poll());

                    lock_type lock{pimpl->mtx_};
                    pimpl->cnd_.wait_for(lock, ::std::chrono::milliseconds{10});
                } else {
                    if (!io_svc->run_one()) {
                        break;
                    }
                }
            }
        });
    }
    ~shared_work()
    {
        //cnd_.notify_one();
    }

    void
    awakened(context* ctx) noexcept
    {
        if (ctx->is_context( context_type::pinned_context )) {
            ctx->ready_link(pimpl_->local_queue_);
        } else {
            ctx->detach();
            service::lock_type lock{ pimpl_->shared_->mtx_ };
            ctx->ready_link( pimpl_->shared_->queue_ );
        }
    }

    context*
    pick_next() noexcept
    {
        context* ctx{ nullptr };
        service::lock_type lock{ pimpl_->shared_->mtx_ };
        if (!pimpl_->shared_->queue_.empty()) {
            ctx = &pimpl_->shared_->queue_.front();
            pimpl_->shared_->queue_.pop_front();
            lock.unlock();
            context::active()->attach(ctx);
        } else {
            lock.unlock();
            if (!pimpl_->local_queue_.empty()) {
                ctx = &pimpl_->local_queue_.front();
                pimpl_->local_queue_.pop_front();
            }
        }

        return ctx;
    }

    bool
    has_ready_fibers() const noexcept
    {
        return pimpl_->has_ready_fibers();
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
        pimpl_->cnd_.notify_one();
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
private:
    io_service_ptr                              io_svc_;
    ::asio_ns::steady_timer                     suspend_timer_;

    ::std::shared_ptr<impl>                     pimpl_;
};

io_service::id shared_work::service::id;

} /* namespace fiber */
} /* namespace asio */
} /* namespace psst */


#endif /* PUSHKIN_ASIO_FIBER_SHARED_WORK_HPP_ */
