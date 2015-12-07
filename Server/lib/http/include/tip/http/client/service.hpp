/*
 * service.h
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_CLIENT_SERVICE_HPP_
#define TIP_HTTP_CLIENT_SERVICE_HPP_

#include <memory>
#include <functional>
#include <boost/asio/io_service.hpp>
#include <tip/http/common/request.hpp>

namespace tip {
namespace http {

class response;
typedef std::shared_ptr< response > response_ptr;
typedef std::function< void(response_ptr) > response_callback;

namespace client {

class service : public boost::asio::io_service::service {
public:
	typedef boost::asio::io_service::service base_type;
	typedef boost::asio::io_service io_service;
	typedef std::vector<char> body_type;
public:
	static io_service::id id;
public:
	service(io_service& owner);
	virtual ~service();

	void
	set_defaults(headers const& h);

	void
	get(std::string const& url, response_callback);
	void
	post(std::string const& url, body_type const& body, response_callback);
	void
	post(std::string const& url, body_type&& body, response_callback);
private:
	virtual void
	shutdown_service();
private:
	struct impl;
	typedef std::shared_ptr< impl > pimpl;
	pimpl pimpl_;
};

} /* namespace client */
} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_CLIENT_SERVICE_HPP_ */
