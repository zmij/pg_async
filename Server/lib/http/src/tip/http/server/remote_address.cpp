/*
 * remote_address.cpp
 *
 *  Created on: Sep 2, 2015
 *      Author: zmij
 */

#include <tip/http/server/remote_address.hpp>

namespace tip {
namespace http {
namespace server {

reply::id remote_address::id;

remote_address::remote_address(reply const& r) :
		reply::context(r)
{
}

remote_address::remote_address(reply const& r, endpoint_ptr p) :
		reply::context(r), peer_address_(p->address())
{
	auto f = r.request_headers().find("X-Real-IP");
	if (f != r.request_headers().end()) {
		try {
			headers_address_ = ip_address::from_string( f->second );
		} catch (...) {
		}
	}
}

remote_address::~remote_address()
{
}

remote_address::ip_address const&
remote_address::address() const
{
	if (!headers_address_.is_unspecified()) {
		return headers_address_;
	}
	return peer_address_;
}

} /* namespace server */
} /* namespace http */
} /* namespace tip */
