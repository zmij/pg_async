/*
 * response.cpp
 *
 *  Created on: Aug 24, 2015
 *      Author: zmij
 */

#include <tip/http/common/response.hpp>

#include <tip/http/common/grammar/response_parse.hpp>
#include <tip/http/common/grammar/response_generate.hpp>
#include <tip/http/common/grammar/cookie_parse.hpp>
#include <tip/http/common/grammar/cookie_generate.hpp>

#include <tip/util/misc_algorithm.hpp>

#include <boost/spirit/include/support_istream_iterator.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>
#include <boost/lexical_cast.hpp>

#include <tip/log.hpp>

namespace tip {
namespace http {

LOCAL_LOGGING_FACILITY(HTTPRESP, TRACE);

namespace {

const std::map< response_status::status_type, std::string > STATUS_STRINGS {
	{ response_status::continue_,						"Continue"						},
	{ response_status::switching_protocols,				"Switching Protocols"			},

	{ response_status::ok,								"OK"							},
	{ response_status::created,							"Created"						},
	{ response_status::accepted,						"Accepted"						},
	{ response_status::non_authoritative_information,	"Non-Authoritative Information"	},
	{ response_status::no_content,						"No Content"					},
	{ response_status::reset_content,					"Reset Content"					},
	{ response_status::partial_content,					"Partial Content"				},

	{ response_status::multiple_choices,				"Multiple Choices"				},
	{ response_status::moved_permanently,				"Moved Permanently"				},
	{ response_status::found,							"Found"							},
	{ response_status::see_other,						"See Other"						},
	{ response_status::not_modified,					"Not Modified"					},
	{ response_status::use_proxy,						"Use Proxy"						},
	{ response_status::temporary_redirect,				"Temporary Redirect"			},

	{ response_status::bad_request,						"Bad Request"					},
	{ response_status::unauthorized,					"Unauthorized"					},
	{ response_status::payment_required,				"Payment Required"				},
	{ response_status::forbidden,						"Forbidden"						},
	{ response_status::not_found,						"Not Found"						},
	{ response_status::method_not_allowed,				"Method Not Allowed"			},
	{ response_status::not_acceptable,					"Not Acceptable"				},
	{ response_status::proxy_authentication_required,	"Proxy Authentication Required"	},
	{ response_status::request_timeout,					"Request Timeout"				},
	{ response_status::conflict,						"Conflict"						},
	{ response_status::gone,							"Gone"							},
	{ response_status::length_required,					"Length Required"				},
	{ response_status::precondition_failed,				"Precondition Failed"			},
	{ response_status::request_entity_too_large,		"Request Entity Too Large"		},
	{ response_status::request_uri_too_long,			"Request-URI Too Long"			},
	{ response_status::unsupported_media_type,			"Unsupported Media Type"		},
	{ response_status::requested_range_not_satisfiable,	"Requested Range Not Satisfiable"},
	{ response_status::expectation_failed,				"Expectation Failed"			},

	{ response_status::internal_server_error,			"Internal Server Error"			},
	{ response_status::not_implemented,					"Not Implemented"				},
	{ response_status::bad_gateway,						"Bad Gateway"					},
	{ response_status::service_unavailable,				"Service Unavailable"			},
	{ response_status::gateway_timeout,					"Gateway Timeout"				},
	{ response_status::http_version_not_supported,		"HTTP Version Not Supported"	}
};

const std::set<response_status::status_type> NO_BODY = {
	response_status::no_content,
	response_status::reset_content
};

std::map<response_status::status_type, response_const_ptr> STOCK_RESPONSES;

}  // namespace

void
response::set_status(response_status::status_type s)
{
	if (s != status) {
		status = s;
		auto f = STATUS_STRINGS.find(s);
		if (f != STATUS_STRINGS.end()) {
			status_line = f->second;
		}
	}
}

size_t
response::content_length() const
{
	return http::content_length(headers_);
}

bool
response::is_chunked() const
{
	return chunked_transfer_encoding(headers_);
}

bool
response::read_headers(std::istream& is)
{
	namespace spirit = boost::spirit;
	namespace qi = boost::spirit::qi;

	typedef std::istreambuf_iterator<char> istreambuf_iterator;
	typedef boost::spirit::multi_pass< istreambuf_iterator > stream_iterator;
	typedef grammar::parse::response_grammar< stream_iterator > response_grammar;

	stream_iterator f = spirit::make_default_multi_pass(istreambuf_iterator(is));
	stream_iterator l = spirit::make_default_multi_pass(istreambuf_iterator());

	return (qi::parse(f, l, response_grammar(), *this));
}

response::read_result_type
response::read_body(std::istream& is)
{
	size_t cl = content_length();
	if (cl > 0) {
		// read ordinary body
		body_.reserve(cl);
		return read_body_content_length(is, cl);
	} else if (is_chunked()) {
		// read chunked body
		return read_body_chunks(is, 0);
	}
	return {false, read_callback()};
}

response::read_result_type
response::read_body_content_length(std::istream& is, size_t remain)
{
	if (remain == 0) {
		// Don't need more data
		return read_result_type{true, read_callback()};
	}
	is.unsetf(std::ios_base::skipws);
	std::istream_iterator<char> f(is);
	std::istream_iterator<char> l;
	size_t consumed = util::copy_max(f, l, remain, std::back_inserter(body_));
	if (consumed == 0) {
		// Failed to read anything
		return read_result_type{false, read_callback()};
	}
	if (consumed == remain) {
		// Don't need more data
		return read_result_type{true, read_callback()};
	}

	return read_result_type{boost::indeterminate,
			std::bind(&response::read_body_content_length,
					this, std::placeholders::_1, remain - consumed)};
}

template < typename InputIterator >
response::read_result_type
response::read_chunk_data(InputIterator& f, InputIterator l, size_t size)
{
	size_t consumed = util::copy_max(f, l, size, std::back_inserter(body_));
	if (consumed == 0) {
		return read_result_type{false, read_callback()};
	} else if (consumed < size) {
		// need more data
		return read_result_type{boost::indeterminate,
			std::bind( &response::read_body_chunks,
				this, std::placeholders::_1, size - consumed )};
	}
	return read_result_type{ true, read_callback() };
}
response::read_result_type
response::read_body_chunks(std::istream& is, size_t tail)
{
	namespace qi = boost::spirit::qi;
	typedef boost::spirit::istream_iterator istream_iterator;

	static qi::rule<istream_iterator> crlf = qi::lit("\r\n");
	static qi::rule<istream_iterator, std::uint32_t()> hexnumber =
			qi::uint_parser< std::uint32_t, 16, 1 >();
	static qi::rule<istream_iterator, size_t()> chunk_size_rule =
			hexnumber >> crlf;

	is.unsetf(std::ios_base::skipws);
	istream_iterator f(is);
	istream_iterator l;
	if ( tail > 0 ) {
		read_result_type r = read_chunk_data(f, l, tail);
		if (r.result) {
			qi::parse(f, l, crlf);
		} else {
			return r;
		}
	}
	size_t chunk_size = 0;
	while (f != l && qi::parse(f, l, chunk_size_rule, chunk_size)) {
		if (chunk_size > 0) {
			body_.reserve(body_.size() + chunk_size);
			read_result_type r = read_chunk_data(f, l, chunk_size);
			if (r.result) {
				qi::parse(f, l, crlf);
			} else {
				return r;
			}
		} else {
			qi::parse(f, l, crlf);
			return read_result_type{true, read_callback()};
		}
	}

	return read_result_type{boost::indeterminate,
		std::bind( &response::read_body_chunks,
			this, std::placeholders::_1, 0 )};
}

void
response::add_header(header const& h)
{
	headers_.insert(h);
}

void
response::add_header(header && h)
{
	headers_.insert(std::move(h));
}

void
response::add_cookie(cookie const& c)
{
	namespace karma = boost::spirit::karma;
	typedef std::ostream_iterator<char> output_iterator;
	typedef grammar::gen::set_cookie_grammar< output_iterator > set_cookie_grammar;
	static set_cookie_grammar gen;

	std::ostringstream os;
	output_iterator out(os);
	karma::generate(out, gen, c);
	add_header( {SetCookie, os.str()} );
}

void
response::get_cookies(cookies& cookie_container)
{
	namespace qi = boost::spirit::qi;
	typedef std::string::const_iterator string_iterator;
	typedef grammar::parse::set_cookie_grammar<string_iterator> set_cookie_grammar;
	typedef headers::const_iterator headers_iterator;
	typedef std::pair< headers_iterator, headers_iterator > headers_range;

	static set_cookie_grammar parser;
	headers_range ch = headers_.equal_range(SetCookie);
	while (ch.first != ch.second) {
		string_iterator f = ch.first->second.begin();
		string_iterator l = ch.first->second.end();
		cookie c;

		if (qi::parse(f, l, parser, c)) {
			cookie_container.push_back(c);
		}

		++ch.first;
	}
}

bool
response::operator ==(response const& rhs) const
{
	return status == rhs.status && headers_ == rhs.headers_
			&& body_ == rhs.body_;
}

std::ostream&
operator << (std::ostream& os, response const& val)
{
	namespace karma = boost::spirit::karma;
	typedef std::ostream_iterator<char> output_iterator;
	typedef grammar::gen::response_grammar<output_iterator> response_head;
	typedef grammar::gen::header_grammar<output_iterator> header_grammar;

	std::ostream::sentry s(os);
	if (s) {
		output_iterator out(os);
		karma::generate(out, response_head(), val);
	}
	return os;
}

response_const_ptr
response::stock_response(response_status::status_type status)
{
	namespace karma = boost::spirit::karma;
	typedef std::back_insert_iterator< body_type > output_iterator;
	typedef grammar::gen::stock_response_grammar< output_iterator > stock_response;

	auto f = STOCK_RESPONSES.find(status);
	if (f == STOCK_RESPONSES.end()) {
		response_ptr resp( new response{{1, 1}});
		resp->set_status(status);
		resp->add_header({ ContentType, "text/html" });

		karma::generate(std::back_inserter(resp->body_), stock_response(), *resp);
		resp->add_header({
			ContentLength,
			boost::lexical_cast<std::string>( resp->body_.size() )
		});
		f = STOCK_RESPONSES.insert(std::make_pair(status, resp)).first;
	}
	return f->second;
}

}  // namespace http
}  // namespace tip
