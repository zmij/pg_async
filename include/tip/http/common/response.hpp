/*
 * responce.hpp
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_RESPONSE_HPP_
#define TIP_HTTP_COMMON_RESPONSE_HPP_

#include <tip/http/common/header.hpp>
#include <tip/http/common/response_status.hpp>
#include <tip/http/common/response_fwd.hpp>

#include <iosfwd>
#include <vector>
#include <functional>

#include <tip/util/read_result.hpp>

namespace tip {
namespace http {

struct response;
typedef std::shared_ptr<response> response_ptr;
struct cookie;
typedef std::vector<cookie> cookies;

struct response {
	typedef std::pair< std::int32_t, std::int32_t >	version_type;
	typedef std::vector<char>						body_type;

	typedef util::read_result< std::istream& >		read_result_type;
	typedef read_result_type::read_callback_type	read_callback;

	version_type					version;
	response_status::status_type	status;
	std::string						status_line;
	headers							headers_;

	body_type						body_;

	bool
	operator == (response const&) const;

	void
	set_status(response_status::status_type);
	size_t
	content_length() const;

	bool
	is_chunked() const;

	bool
	read_headers(std::istream& is);

	/**
	 * Read response body from stream
	 * @param is
	 * @return first: true if reading is complete.
	 * 				false if reading failed
	 * 				indeterminate if needs more data
	 * 			second: callback for reading continuation
	 */
	read_result_type
	read_body(std::istream& is);

	void
	add_header(header const&);
	void
	add_header(header &&);

	void
	add_cookie(cookie const&);
	void
	get_cookies(cookies&);

	static response_const_ptr
	stock_response(response_status::status_type status);
private:
	read_result_type
	read_body_content_length(std::istream& is, size_t remain);
	read_result_type
	read_body_chunks(std::istream& is, size_t tail);
	template < typename InputIterator >
	read_result_type
	read_chunk_data(InputIterator& f, InputIterator l, size_t size);
};

std::ostream&
operator << (std::ostream&, response const&);

}  // namespace http
}  // namespace tip


#endif /* TIP_HTTP_COMMON_RESPONSE_HPP_ */
