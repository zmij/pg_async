/*
 * request_dispatcher.hpp
 *
 *  Created on: Jul 17, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_REQUEST_DISPATCHER_HPP_
#define TIP_HTTP_SERVER_REQUEST_DISPATCHER_HPP_

#include <tip/http/server/request_handler.hpp>
#include <set>

namespace tip {
namespace http {
namespace server {

struct reply;
struct request;

class request_dispatcher: public tip::http::server::request_handler {
public:
	typedef std::set<request_method> request_method_set;
public:
	request_dispatcher();
	virtual ~request_dispatcher();

	void
	add_handler( request_method, std::string const&, request_handler_ptr );
	void
	add_handler( request_method_set const&, std::string const&, request_handler_ptr );

	template < typename T >
	void
	add_handler( request_method method, std::string const& path)
	{
		add_handler( method, path, std::make_shared< T >() );
	}
	template < typename T >
	void
	add_handler( request_method_set const& methods, std::string const& path)
	{
		add_handler( methods, path, std::make_shared< T >() );
	}

	void
	get(std::string const&, request_handler_ptr);
	void
	post(std::string const&, request_handler_ptr);
private:
	virtual void
	do_handle_request(reply);
private:
	struct impl;
	typedef std::shared_ptr<impl> pimpl;
	pimpl pimpl_;
};

typedef std::shared_ptr<request_dispatcher> request_dispatcher_ptr;

} /* namespace server */
} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_SERVER_REQUEST_DISPATCHER_HPP_ */
