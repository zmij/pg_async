/*
 * request.hpp
 *
 *  Created on: Aug 17, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_REQUEST_HPP_
#define TIP_HTTP_COMMON_REQUEST_HPP_

#include <tip/http/common/header.hpp>
#include <tip/http/common/request_fwd.hpp>
#include <tip/iri.hpp>
#include <tip/util/read_result.hpp>

#include <memory>
#include <boost/asio/buffer.hpp>

namespace tip {
namespace http {

class request {
public:
	typedef std::pair< std::int32_t, std::int32_t >		version_type;
	typedef std::pair< std::string, std::string >		query_param_type;
	typedef std::multimap< std::string, std::string >	query_type;
	typedef std::vector< char >							body_type;
	typedef tip::iri::basic_iri<query_type>				iri_type;
	typedef util::read_result< std::istream& >			read_result_type;
	typedef read_result_type::read_callback_type		read_callback;


	request_method	method;
	version_type	version;
	iri::path		path;
	query_type		query;
	iri::fragment	fragment;
	mutable headers	headers_;

	body_type		body_;

	size_t
	content_length() const;

	void
	host(tip::iri::host const&);
	tip::iri::host
	host() const;

	bool
	read_headers(std::istream&);

	read_result_type
	read_body(std::istream&);
public:
	static request_ptr
	create(request_method method, iri_type const& iri,
			body_type const& body = body_type());

	static request_ptr
	create(request_method method, iri_type const& iri,
			body_type&& body);

	static request_ptr
	create(request_method method, std::string const& iri_str);

	static bool
	parse_iri(std::string const&, iri_type&);
private:
	read_result_type
	read_body_content_length(std::istream&, size_t remain);
};

std::ostream&
operator << (std::ostream&, request const&);

} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_COMMON_REQUEST_HPP_ */
