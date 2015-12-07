/*
 * session.hpp
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_CLIENT_SESSION_HPP_
#define TIP_HTTP_CLIENT_SESSION_HPP_

#include <memory>
#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>

#include <tip/http/common/request.hpp>

namespace tip {
namespace http {

class response;
typedef std::shared_ptr< response > response_ptr;

namespace client {

class session;
typedef std::shared_ptr< session > session_ptr;
typedef std::weak_ptr< session > session_weak_ptr;

class session : boost::noncopyable {
public:
	typedef boost::asio::io_service io_service;
	typedef std::vector<char> body_type;
	typedef std::function< void(request_ptr, response_ptr) > response_callback;
	typedef std::function< void(session_ptr) > session_callback;
public:
	virtual ~session();

	void
	send_request(request_method method, request::iri_type const&,
			body_type const&, response_callback cb);
	void
	send_request(request_method method, request::iri_type const&,
			body_type&&, response_callback cb);
	void
	send_request(request_ptr req, response_callback cb);

	void
	close();

	static session_ptr
	create(io_service& svc, request::iri_type const&, session_callback on_close,
			headers const& default_headers);
protected:
	session();

	virtual void
	do_send_request(request_method method, request::iri_type const&,
			body_type const&, response_callback cb) = 0;
	virtual void
	do_send_request(request_method method, request::iri_type const&,
			body_type&&, response_callback cb) = 0;
	virtual void
	do_send_request(request_ptr req, response_callback cb) = 0;

	virtual void
	do_close() = 0;
};

} /* namespace client */
} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_CLIENT_SESSION_HPP_ */
