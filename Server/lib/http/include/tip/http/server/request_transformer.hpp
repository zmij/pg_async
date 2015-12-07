/*
 * request_transformer.hpp
 *
 *  Created on: Oct 6, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_REQUEST_TRANSFORMER_HPP_
#define TIP_HTTP_SERVER_REQUEST_TRANSFORMER_HPP_

#include <tip/http/server/request_handler.hpp>
#include <tip/http/server/prerequisite_handler.hpp>
#include <tip/http/server/error.hpp>

namespace tip {
namespace http {
namespace server {

/**
 * Template class to transform request from generic type to a concrete structure
 * and handle a typed request.
 * Transformer type must have an operator() that takes a reply object as a
 * parameter and returns a concrete structure. It also must have a member function
 * error(reply, error).
 * @tparam Transformer Class of request transformer.
 */
template < typename Transformer, typename ... Prerequisite >
class request_transformer : public request_handler {
public:
	typedef Transformer								transformer_type;
	typedef typename transformer_type::request_type	request_type;
	typedef typename transformer_type::pointer      request_pointer;
	typedef prereqiusites< Prerequisite ... >		prerequisites_type;
public:
	request_transformer() : transform_() {}
	virtual ~request_transformer() {}
protected:
	void
	send_error(reply r, error const& e) const
	{
		transform_.error(r, e);
	}
private:
	virtual void
	do_handle_request(reply r)
	{
		try {
			if (prerequisites_(r)) {
				request_pointer req(transform_(r));
				do_handle_request(r, req);
			}
		} catch ( error const& e) {
			transform_.error(r, e);
		}
	}
	virtual void
	do_handle_request(reply, request_pointer) = 0;
private:
	transformer_type transform_;
	prerequisites_type prerequisites_;
};

}  // namespace server
}  // namespace http
}  // namespace tip

#endif /* TIP_HTTP_SERVER_REQUEST_TRANSFORMER_HPP_ */
