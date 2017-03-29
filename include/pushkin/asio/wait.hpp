/*
 * wait.hpp
 *
 *  Created on: Mar 26, 2017
 *      Author: zmij
 */

#ifndef PUSHKIN_ASIO_FIBER_INCLUDE_PUSHKIN_ASIO_WAIT_HPP_
#define PUSHKIN_ASIO_FIBER_INCLUDE_PUSHKIN_ASIO_WAIT_HPP_

#include <pushkin/asio/asio_config.hpp>
#include <thread>
#include <type_traits>

#ifdef WITH_BOOST_FIBERS
#include <boost/fiber/fiber.hpp>
#endif

namespace psst {
namespace asio {

namespace detail {

enum class wait_type {
    same_thread,
    other_thread,
    fiber
};

template < wait_type V >
using wait_constant = ::std::integral_constant<wait_type, V>;

template < typename Pred >
void
run_while( io_service_ptr svc, Pred pred, wait_constant< wait_type::same_thread > const& )
{
    while(pred())
        svc->poll();
}

template < typename Pred >
void
run_while( io_service_ptr svc, Pred pred, wait_constant< wait_type::other_thread > const& )
{
    ::std::thread t{
    [svc, pred](){
        while(pred())
            svc->poll();
    }};
    t.join();
}

#ifdef WITH_BOOST_FIBERS
template < typename Pred >
void
run_while( io_service_ptr svc, Pred pred, wait_constant< wait_type::fiber > const& )
{
    namespace this_fiber = ::boost::this_fiber;
    fiber f{
        [svc, pred](){
            while(pred()) {
                svc->poll();
                this_fiber::yield();
            }
        }};
    f.join();
}
#endif

template < typename Pred >
void
run_until( io_service_ptr svc, Pred pred, wait_constant< wait_type::same_thread > const& )
{
    while(!pred())
        svc->poll();
}

template < typename Pred >
void
run_until( io_service_ptr svc, Pred pred, wait_constant< wait_type::other_thread > const& )
{
    ::std::thread t{
    [svc, pred](){
        while(!pred())
            svc->poll();
    }};
    t.join();
}

#ifdef WITH_BOOST_FIBERS
template < typename Pred >
void
run_until( io_service_ptr svc, Pred pred, wait_constant< wait_type::fiber > const& )
{
    namespace this_fiber = ::boost::this_fiber;
    fiber f{
        [svc, pred](){
            while(!pred()) {
                svc->poll();
                this_fiber::yield();
            }
        }};
    f.join();
}
#endif

static wait_type constexpr default_wait_type =
#ifdef WITH_BOOST_FIBERS
        wait_type::fiber;
#elif ASIO_WAIT_SAME_THREAD
        wait_type::same_thread;
#else
        wait_type::other_thread;
#endif

} /* namespace detail */

template < typename Pred >
void
wait_while( io_service_ptr svc, Pred pred )
{
    detail::run_while(svc, ::std::move(pred),
            detail::wait_constant< detail::default_wait_type >{});
}

template < typename Pred >
void
wait_until( io_service_ptr svc, Pred pred )
{
    detail::run_until(svc, ::std::move(pred),
            detail::wait_constant< detail::default_wait_type >{});
}

} /* namespace asio */
} /* namespace psst */


#endif /* PUSHKIN_ASIO_FIBER_INCLUDE_PUSHKIN_ASIO_WAIT_HPP_ */
