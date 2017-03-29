/*
 * async_ssl_ops.hpp
 *
 *  Created on: Mar 27, 2017
 *      Author: zmij
 */

#ifndef PUSHKIN_ASIO_ASYNC_SSL_OPS_HPP_
#define PUSHKIN_ASIO_ASYNC_SSL_OPS_HPP_

#include <pushkin/asio/async_ops.hpp>
#include <boost/asio/ssl.hpp>

namespace psst {
namespace asio {

namespace detail {

template < typename StreamType >
struct socket_ops<::asio_ns::ssl::stream<StreamType>> {
    using socket_type = ::asio_ns::ssl::stream<StreamType>;

    template < typename Handler >
    static auto
    async_handshake(socket_type& sock,
            ::asio_ns::ssl::stream_base::handshake_type t,
            Handler handler)
    {
        return sock.async_handshake(t, ::std::move(handler));
    }

    template < typename Buffer, typename Handler >
    static auto
    async_write(socket_type& sock, Buffer const& buff, Handler handler)
        -> decltype(asio_ns::async_write(sock, buff, handler))
    {
        return asio_ns::async_write(sock, buff, ::std::move(handler));
    }

    template < typename Buffer, typename Handler >
    static auto
    async_read(socket_type& sock, Buffer&& buff, Handler handler)
    {
        return asio_ns::async_read(sock, ::std::forward<Buffer>(buff),
            ::asio_ns::transfer_at_least(1), ::std::move(handler));
    }
};

} /* namespace detail */

template < typename StreamType, typename Handler >
void
async_handshake(::asio_ns::ssl::stream<StreamType>& sock,
        ::asio_ns::ssl::stream_base::handshake_type t, Handler handler)
{
    using socket_ops = detail::socket_ops<::asio_ns::ssl::stream<StreamType>>;
    socket_ops::async_handshake(sock, t, ::std::move(handler));
}

#ifdef WITH_BOOST_FIBERS

template < typename StreamType, typename Handler >
void
fiber_handshake(::asio_ns::ssl::stream<StreamType>& sock,
        ::asio_ns::ssl::stream_base::handshake_type t, Handler handler)
{
    using socket_ops = detail::socket_ops<::asio_ns::ssl::stream<StreamType>>;
    error_code ec;
    socket_ops::async_handshake(sock, t, fiber::yield_t{ec});
    handler(ec);
}

template < typename StreamType >
void
fiber_handshake(::asio_ns::ssl::stream<StreamType>& sock,
        ::asio_ns::ssl::stream_base::handshake_type t)
{
    using promise_type = ::boost::fibers::promise<void>;
    auto promise = ::std::make_shared<promise_type>();

    fiber_handshake(sock, t,
        [promise](error_code const& ec)
        {
            if (!ec) {
                promise->set_value();
            } else {
                promise->set_exception( ::std::make_exception_ptr( system_error{ ec } ) );
            }
        });

    promise->get_future().get();
}

#endif /* WITH_BOOST_FIBERS */

template < typename StreamType, typename Handler >
void
handshake(::asio_ns::ssl::stream<StreamType>& sock,
        ::asio_ns::ssl::stream_base::handshake_type t, Handler handler)
{
#ifdef WITH_BOOST_FIBERS
    fiber_handshake(sock, t, ::std::move(handler));
#else /* NO WITH_BOOST_FIBERS */
    async_handshake(sock, t, ::std::move(handler));
#endif /* WITH_BOOST_FIBERS */
}

} /* namespace asio */
} /* namespace psst */


#endif /* PUSHKIN_ASIO_ASYNC_SSL_OPS_HPP_ */
