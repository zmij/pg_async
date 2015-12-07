/**
 * @file /tip-server/include/tip/http/server/request_parser.hpp
 * @brief
 * @date Jul 8, 2015
 * @author: zmij
 */

#ifndef TIP_HTTP_SERVER_REQUEST_HANDLER_HPP
#define TIP_HTTP_SERVER_REQUEST_HANDLER_HPP

#include <string>
#include <boost/noncopyable.hpp>
#include <tip/http/server/reply.hpp>
#include <memory>
#include <functional>


namespace tip {
namespace http {
namespace server {
/**
 * Base class for handling requests
 */
class request_handler :
		public std::enable_shared_from_this< request_handler >,
		private boost::noncopyable {
public:
public:
	virtual ~request_handler() {}

	/**
	 * Process the request, form the reply and call the callback when
	 * the reply is ready.
	 * @param req
	 * @param rep
	 * @param cb
	 */
	virtual void
	handle_request(reply);
protected:
	/// Perform URL-decoding on a string. Returns false if the encoding was
	/// invalid.
	static bool
	url_decode(const std::string& in, std::string& out);

	template < typename T >
	std::shared_ptr< T >
	shared_this()
	{
		return std::dynamic_pointer_cast< T >(shared_from_this());
	}

	template < typename T >
	std::shared_ptr< T const >
	shared_this() const
	{
		return std::dynamic_pointer_cast< T const >(shared_from_this());
	}
private:
	virtual void
	do_handle_request(reply) = 0;
};

typedef std::shared_ptr<request_handler> request_handler_ptr;

}  // namespace server
}  // namespace http
}  // namespace tip

#endif /* TIP_HTTP_SERVER_REQUEST_HANDLER_HPP */
