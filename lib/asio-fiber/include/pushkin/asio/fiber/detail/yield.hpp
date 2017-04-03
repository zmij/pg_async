/*
 * yield.hpp
 *
 *  Created on: Mar 26, 2017
 *      Author: zmij
 */

#ifndef PUSHKIN_ASIO_FIBER_DETAIL_YIELD_HPP_
#define PUSHKIN_ASIO_FIBER_DETAIL_YIELD_HPP_

#include <pushkin/asio/fiber/yield.hpp>

#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/handler_type.hpp>

#include <boost/fiber/all.hpp>
#include <mutex>

namespace psst {
namespace asio {
namespace fiber {
namespace detail {

struct yield_completion {
    using mutex_type    = ::boost::fibers::detail::spinlock;
    using lock_type     = ::std::unique_lock<mutex_type>;

    mutex_type  mtx_{};
    bool        completed_{false};

    void
    wait()
    {
        lock_type lock{mtx_};
        if (! completed_ ) {
            ::boost::fibers::context::active()->suspend(lock);
        }
    }
};

struct yield_handler_base {
    using context   = ::boost::fibers::context;

    yield_handler_base(yield_t const& y)
        : ctx_{ context::active() },
          yld_{y} {}

    void
    operator()(error_code const& ec)
    {
        assert(ycomp_ &&
                "Must inject yield_completion* "
                "before calling yield_handler_base::operator()");
        assert(yld_.ec_ &&
                "Must inject boost::system::error_code* "
                "before calling yield_handler_base::operator()");

        yield_completion::lock_type lock{ ycomp_->mtx_ };
        ycomp_->completed_ = true;
        *yld_.ec_ = ec;

        if (context::active() != ctx_) {
            context::active()->set_ready(ctx_);
        }
    }

//private:
    context*                        ctx_;
    yield_t                         yld_;
    yield_completion*               ycomp_{nullptr};
};

template < typename T >
struct yield_handler : yield_handler_base {
    explicit yield_handler(yield_t const& y)
        : yield_handler_base{y} {}

    void
    operator()(T t)
    { (*this)(error_code{}, ::std::move(t)); }

    void
    operator()(error_code const& ec, T t)
    {
        assert(value_ &&
                "Must inject pointer to value"
                "before calling yield_handler<T>::operator()");

        *value_ = ::std::move(t);
        yield_handler_base::operator ()(ec);
    }

    T*                              value_{nullptr};
};

template <>
struct yield_handler<void> : yield_handler_base {
    explicit yield_handler(yield_t const& y)
        : yield_handler_base{y} {}

    void
    operator()()
    { (*this)(error_code{}); }

    using yield_handler_base::operator();
};

template < typename Fn, typename T >
void
asio_handler_invoke(Fn fn, yield_handler<T>* h)
{
    fn();
}

class async_result_base {
public:
    explicit
    async_result_base(yield_handler_base& h)
    {
        h.ycomp_ = &ycomp_;
        if (!h.yld_.ec_) {
            h.yld_.ec_ = &ec_;
        }
    }

    void
    get()
    {
        ycomp_.wait();
        if (ec_) {
            throw ::boost::system::system_error{ ec_ };
        }
    }
private:
    error_code          ec_{};
    yield_completion    ycomp_{};
};

} /* namespace detail */
} /* namespace fiber */
} /* namespace asio */
} /* namespace psst */

namespace boost {
namespace asio {

template < typename T >
class async_result< ::psst::asio::fiber::detail::yield_handler<T> >
        : public ::psst::asio::fiber::detail::async_result_base {
public:
    using type          = T;
    using handler_type  = ::psst::asio::fiber::detail::yield_handler<T>;

    explicit
    async_result(handler_type& h)
        : async_result_base{h}
    {
        // Inject value pointer
        h.value_ = &value_;
    }
    type
    get()
    {
        async_result_base::get();
        return ::std::move(value_);
    }
private:
    type        value_{};
};

template <>
class async_result< ::psst::asio::fiber::detail::yield_handler<void> >
        : public ::psst::asio::fiber::detail::async_result_base {
public:
    using type          = void;
    using handler_type  = ::psst::asio::fiber::detail::yield_handler<void>;

    explicit
    async_result(handler_type& h)
        : async_result_base{h} {}
};

//@{
/** Handler specializations for yield_t */
template < typename ReturnType >
struct handler_type< ::psst::asio::fiber::yield_t, ReturnType() >
{ using type = ::psst::asio::fiber::detail::yield_handler<void>; };

template < typename ReturnType >
struct handler_type< ::psst::asio::fiber::yield_t, ReturnType( system::error_code ) >
{ using type = ::psst::asio::fiber::detail::yield_handler<void>; };

template < typename ReturnType, typename Arg >
struct handler_type< ::psst::asio::fiber::yield_t, ReturnType(Arg) >
{  using type = ::psst::asio::fiber::detail::yield_handler<Arg>;  };

template < typename ReturnType, typename Arg >
struct handler_type< ::psst::asio::fiber::yield_t, ReturnType(system::error_code, Arg) >
{  using type = ::psst::asio::fiber::detail::yield_handler<Arg>;  };
//@}

} /* namespace asio */
} /* namespace boost */


#endif /* PUSHKIN_ASIO_FIBER_DETAIL_YIELD_HPP_ */
