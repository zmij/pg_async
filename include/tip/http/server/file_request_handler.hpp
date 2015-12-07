/**
 * @file /tip-server/include/tip/http/server/file_request_handler.hpp
 * @brief
 * @date Jul 9, 2015
 * @author: zmij
 */

#ifndef TIP_HTTP_SERVER_FILE_REQUEST_HANDLER_HPP_
#define TIP_HTTP_SERVER_FILE_REQUEST_HANDLER_HPP_

#include "tip/http/server/request_handler.hpp"

namespace tip {
namespace http {
namespace server {

/**
 * Demo handler replying with files
 */
class file_request_handler : public request_handler {
public:
	/// Construct with a directory containing files to be served.
	explicit file_request_handler(std::string const& doc_root);
	virtual ~file_request_handler() {}

private:
	/** Handle a request and produce a reply. */
	void
	do_handle_request(reply);
	std::string doc_root_; /**< The directory containing the files to be served. */

};

}  // namespace server
}  // namespace http
}  // namespace tip



#endif /* TIP_HTTP_SERVER_FILE_REQUEST_HANDLER_HPP_ */
