/*
 * remote_address.hpp
 *
 *  Created on: Sep 2, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_REMOTE_ADDRESS_HPP_
#define TIP_HTTP_SERVER_REMOTE_ADDRESS_HPP_

#include <tip/http/server/reply.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace tip {
namespace http {
namespace server {

class remote_address: public reply::context {
public:
	typedef std::shared_ptr<boost::asio::ip::tcp::endpoint> endpoint_ptr;
	typedef boost::asio::ip::address ip_address;
public:
	static reply::id id;
public:
	remote_address(reply const& r);
	remote_address(reply const& r, endpoint_ptr p);
	virtual ~remote_address();

	ip_address const&
	address() const;
private:
	ip_address peer_address_;
	ip_address headers_address_;
};

} /* namespace server */
} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_SERVER_REMOTE_ADDRESS_HPP_ */
