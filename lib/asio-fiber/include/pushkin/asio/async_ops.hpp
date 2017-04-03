/*
 * async_ops.hpp
 *
 *  Created on: Mar 26, 2017
 *      Author: zmij
 */

#ifndef PUSHKIN_ASIO_ASYNC_OPS_HPP_
#define PUSHKIN_ASIO_ASYNC_OPS_HPP_

#include <pushkin/asio/asio_config.hpp>

#ifdef WITH_BOOST_FIBERS
#include <pushkin/asio/fiber/yield.hpp>
#include <boost/fiber/future.hpp>
#endif

namespace psst {
namespace asio {

namespace detail {

template < typename Socket >
struct socket_ops; //{
//    template < typename Resolver, typename Query, typename Handler >
//    static void
//    async_resolve(Resolver& res, Query const& query, Handler h);
//
//    template < typename Acceptor, typename Handler >
//    static void
//    async_accept(Acceptor& acceptor, Socket& socket, Handler h);
//
//    template < typename Endpoint, typename Handler >
//    static void
//    async_connect(Socket& sock, Endpoint const& ep, Handler h);
//
//    template < typename Buffer, typename Handler >
//    static ::std::size_t
//    async_write(Socket& sock, Buffer const& buff, Handler h);
//
//    template < typename Buffer, typename Handler >
//    static ::std::size_t
//    async_read(Socket& sock, Buffer& buff, Handler h);
//};

template < typename Resolver >
struct resolver_to_socket {
    using socket_type   = void;
};

template <>
struct resolver_to_socket<tcp::resolver> {
    using socket_type   = tcp::socket;
};

template <>
struct resolver_to_socket<udp::resolver> {
    using socket_type   = udp::socket;
};

template < template < typename, typename > class SocketType >
struct socket_ops< SocketType< tcp, asio_ns::stream_socket_service<tcp> > > {
    using socket_type   = SocketType< tcp, asio_ns::stream_socket_service<tcp> >;
    using resolver_type = tcp::resolver;
    using query_type    = tcp::resolver::query;
    using acceptor_type = tcp::acceptor;

    template < typename Handler >
    static auto
    async_resolve(resolver_type& resolver, query_type const& query, Handler handler)
        -> decltype(resolver.async_resolve(query, ::std::move(handler)))
    {
        return resolver.async_resolve(query, ::std::move(handler));
    }

    template < typename Handler >
    static auto
    async_accept(acceptor_type& acceptor, socket_type& sock, Handler handler)
        -> decltype(acceptor.async_accept(sock, ::std::move(handler)))
    {
        return acceptor.async_accept(sock, ::std::move(handler));
    }

    template < typename Endpoint, typename Handler >
    static auto
    async_connect(socket_type& sock, Endpoint const& ep, Handler handler)
        -> decltype(asio_ns::async_connect(sock, ep, ::std::move(handler)))
    {
        return asio_ns::async_connect(sock, ep, ::std::move(handler));
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

template <>
struct socket_ops< udp::socket > {
    using socket_type = udp::socket;

    template < typename Endpoint, typename Handler >
    static auto
    async_connect(socket_type& sock, Endpoint const& ep, Handler handler)
        -> decltype(sock.async_connect(ep, ::std::move(handler)))
    {
        return sock.async_connect(ep, ::std::move(handler));
    }
};

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
template <>
struct socket_ops< local_socket::socket > {
    using socket_type = local_socket::socket;

    template < typename Endpoint, typename Handler >
    static auto
    async_connect(socket_type& sock, Endpoint const& ep, Handler handler)
        -> decltype(sock.async_connect(ep, ::std::move(handler)))
    {
        return sock.async_connect(ep, ::std::move(handler));
    }

    template < typename Buffer, typename Handler >
    static auto
    async_write(socket_type& sock, Buffer const& buff, Handler handler)
        -> decltype(asio_ns::async_write(sock, buff, handler))
    {
        return asio_ns::async_write(sock, buff, handler);
    }

    template < typename Buffer, typename Handler >
    static auto
    async_read(socket_type& sock, Buffer&& buff, Handler handler)
    {
        return asio_ns::async_read(sock, ::std::forward<Buffer>(buff),
            ::asio_ns::transfer_at_least(1), ::std::move(handler));
    }
};
#endif /* BOOST_ASIO_HAS_LOCAL_SOCKETS */

} /* namespace detail */

/**
 * Start async resolve operation
 * @param resolver
 * @param query
 * @param h
 */
template < typename Resolver, typename Query, typename Handler >
void
async_resolve(Resolver& resolver, Query const& query, Handler handler)
{
    using socket_type = typename detail::resolver_to_socket<Resolver>::socket_type;
    static_assert( !::std::is_same<void, socket_type>::value,
            "resolver_to_socket is not specialized" );
    using socket_ops  = detail::socket_ops<socket_type>;
    socket_ops::async_resolve(resolver, query, ::std::move(handler));
}

/**
 * Start async accept operation
 * @param acceptor
 * @param sock
 * @param h
 */
template < typename Acceptor, typename Socket, typename Handler >
void
async_accept(Acceptor& acceptor, Socket& sock, Handler handler)
{
    using socket_ops = detail::socket_ops<Socket>;
    socket_ops::async_accept(acceptor, sock, ::std::move(handler));
}

/**
 * Start async connection to endpoint.
 * @param sock
 * @param ep
 * @param h
 */
template < typename Socket, typename Endpoint, typename Handler >
void
async_connect(Socket& sock, Endpoint const& ep, Handler handler)
{
    using socket_ops = detail::socket_ops<Socket>;
    socket_ops::async_connect(sock, ep, ::std::move(handler));
}

/**
 * Start async write to the socket
 * @param s
 * @param buff
 * @param h
 */
template < typename Socket, typename Buffer, typename Handler >
void
async_write(Socket& s, Buffer const& buff, Handler handler)
{
    using socket_ops = detail::socket_ops<Socket>;
    socket_ops::async_write(s, buff, ::std::move(handler));
}

/**
 * Start async read from the socket
 * @param s
 * @param buff
 * @param h
 */
template < typename Socket, typename Buffer, typename Handler >
void
async_read(Socket& s, Buffer&& buff, Handler handler)
{
    using socket_ops = detail::socket_ops<Socket>;
    socket_ops::async_read(s, ::std::forward<Buffer>(buff), ::std::move(handler));
}

#ifdef WITH_BOOST_FIBERS

template < typename Resolver, typename Query, typename Handler >
void
fiber_resolve(Resolver& resolver, Query const& query, Handler handler)
{
    using socket_type = typename detail::resolver_to_socket<Resolver>::socket_type;
    static_assert( !::std::is_same<void, socket_type>::value,
            "resolver_to_socket is not specialized" );
    using socket_ops  = detail::socket_ops<socket_type>;
    error_code ec;
    auto iter = socket_ops::async_resolve(resolver, query, fiber::yield_t{ec});
    handler(ec, iter);
}

template < typename Resolver, typename Query >
typename Resolver::iterator
fiber_resolve(Resolver& resolver, Query const& query)
{
    using iterator = typename Resolver::iterator;
    using promise_type = ::boost::fibers::promise<iterator>;
    auto promise = ::std::make_shared<promise_type>();
    fiber_resolve(resolver, query,
        [promise](error_code const& ec, iterator it)
        {
            if (!ec) {
                promise->set_value(it);
            } else {
                promise->set_exception( ::std::make_exception_ptr( system_error{ ec } ) );
            }
        });

    return promise->get_future().get();
}

template < typename Acceptor,  typename Socket, typename Handler >
void
fiber_accept(Acceptor& acceptor, Socket& sock, Handler handler)
{
    using socket_ops = detail::socket_ops<Socket>;
    error_code ec;
    socket_ops::async_accept(acceptor, sock, fiber::yield_t{ec} );
    handler(ec);
}

template < typename Acceptor,  typename Socket >
void
fiber_accept(Acceptor& acceptor, Socket& sock)
{
    using promise_type = ::boost::fibers::promise<void>;
    auto promise = ::std::make_shared<promise_type>();

    fiber_accept(acceptor, sock,
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

/**
 * Connect to the specified endpoint. Suspend current fiber for the time of
 * operation. Handler signature for compatibility with async calls.
 * @param sock
 * @param ep
 * @param h
 */
template < typename Socket, typename Endpoint, typename Handler >
void
fiber_connect(Socket& sock, Endpoint const& ep, Handler handler)
{
    using socket_ops = detail::socket_ops<Socket>;
//    sock.get_io_service().post(
//        [&sock, ep, handler]() mutable
//        {
            error_code ec;
            socket_ops::async_connect(sock, ep, fiber::yield_t{ec} );
            handler(ec);
//        });
    //handler(ec);
}

template < typename Socket, typename Endpoint >
void
fiber_connect(Socket& sock, Endpoint const& ep)
{
    using promise_type = ::boost::fibers::promise<void>;
    auto promise = ::std::make_shared<promise_type>();

    fiber_connect(sock, ep,
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

template < typename Socket, typename Buffer, typename Handler >
void
fiber_write(Socket& sock, Buffer const& buff, Handler handler)
{
    using socket_ops = detail::socket_ops<Socket>;
//    sock.get_io_service().post(
//        [&sock, &buff, handler]()
//        {
            error_code ec;
            auto bytes = socket_ops::async_write(sock, buff, fiber::yield_t{ec} );
            handler(ec, bytes);
//        });
}

template < typename Socket, typename Buffer >
::std::size_t
fiber_write(Socket& sock, Buffer const& buff)
{
    using promise_type = ::boost::fibers::promise<::std::size_t>;
    auto promise = ::std::make_shared<promise_type>();

    fiber_write(sock, buff,
        [promise](error_code const& ec, ::std::size_t bytes)
        {
            // TODO Throw or don't throw on eof - that is the question
            if (!ec || ec == ::asio_ns::error::eof) {
                promise->set_value(bytes);
            } else {
                promise->set_exception( ::std::make_exception_ptr( system_error{ ec } ) );
            }
        });

    return promise->get_future().get();
}

template < typename Socket, typename Buffer, typename Handler >
void
fiber_read(Socket& sock, Buffer&& buff, Handler handler)
{
    using socket_ops = detail::socket_ops<Socket>;
    error_code ec;
    auto bytes = socket_ops::async_read(sock,
            ::std::forward<Buffer>(buff), fiber::yield_t{ec} );
    //sock.get_io_service().post( [handler, ec, bytes](){ handler(ec, bytes); } );
    handler(ec, bytes);
}

template < typename Socket, typename Buffer >
::std::size_t
fiber_read(Socket& sock, Buffer&& buff)
{
    using promise_type = ::boost::fibers::promise<::std::size_t>;
    auto promise = ::std::make_shared<promise_type>();

    fiber_read(sock, ::std::forward<Buffer>(buff),
        [promise](error_code const& ec, ::std::size_t bytes)
        {
            // TODO Throw or don't throw on eof - that is the question
            if (!ec || ec == ::asio_ns::error::eof) {
                promise->set_value(bytes);
            } else {
                promise->set_exception( ::std::make_exception_ptr( system_error{ ec } ) );
            }
        });

    return promise->get_future().get();
}


#endif /* WITH_BOOST_FIBERS */

/**
 * Resolve query async. Either with or without fiber support
 * @param resolver
 * @param query
 * @param h
 */
template < typename Resolver, typename Query, typename Handler >
void
resolve(Resolver& resolver, Query const& query, Handler h)
{
#ifdef WITH_BOOST_FIBERS
    fiber_resolve(resolver, query, ::std::move(h));
#else
    ::psst::asio::async_resolve(resolver, query, ::std::move(h));
#endif
}

template < typename Acceptor, typename Socket, typename Handler >
void
accept(Acceptor& acceptor, Socket& sock, Handler h)
{
#ifdef WITH_BOOST_FIBERS
    fiber_accept(acceptor, sock, ::std::move(h));
#else
    ::psst::asio::async_accept(acceptor, sock, ::std::move(h));
#endif
}

template < typename Socket, typename Endpoint, typename Handler >
void
connect(Socket& sock, Endpoint const& ep, Handler h)
{
#ifdef WITH_BOOST_FIBERS
    fiber_connect(sock, ep, ::std::move(h));
#else
    ::psst::asio::async_connect(sock, ep, ::std::move(h));
#endif
}

template < typename Socket, typename Buffer, typename Handler >
void
write(Socket& sock, Buffer const& buff, Handler h)
{
#ifdef WITH_BOOST_FIBERS
    fiber_write(sock, buff, ::std::move(h));
#else
    ::psst::asio::async_write(sock, buff, ::std::move(h));
#endif
}

template < typename Socket, typename Buffer, typename Handler >
void
read(Socket& sock, Buffer&& buff, Handler h)
{
#ifdef WITH_BOOST_FIBERS
    fiber_read(sock, ::std::forward<Buffer>(buff), ::std::move(h));
#else
    ::psst::asio::async_read(sock, ::std::forward<Buffer>(buff), ::std::move(h));
#endif
}

} /* namespace asio */
} /* namespace psst */



#endif /* PUSHKIN_ASIO_ASYNC_OPS_HPP_ */
