/*
 * request_grammar.hpp
 *
 *  Created on: Aug 17, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_GRAMMAR_REQUEST_PARSE_HPP_
#define TIP_HTTP_COMMON_GRAMMAR_REQUEST_PARSE_HPP_

#include <tip/http/common/grammar/header_parse.hpp>
#include <tip/http/common/request.hpp>
#include <tip/iri/grammar/iri_parse.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace tip {
namespace http {
namespace grammar {
namespace parse {

struct method_grammar :
		boost::spirit::qi::symbols<char, request_method> {
	method_grammar();
};

template < typename InputIterator >
struct query_param_grammar :
		boost::spirit::qi::grammar< InputIterator, request::query_param_type() > {
	query_param_grammar() : query_param_grammar::base_type(query_param)
	{
		using boost::spirit::qi::char_;
		query_char %= iunreserved |
				pct_encoded |
				iprivate |
				char_("!$'()*+,;:@/?");

		query_param = +query_char >> '='
				>> *(query_char|char_('='));
	}

	boost::spirit::qi::rule< InputIterator, request::query_param_type() > query_param;

	tip::iri::grammar::parse::iunreserved_grammar< InputIterator > iunreserved;
	tip::iri::grammar::parse::pct_encoded_grammar< InputIterator > pct_encoded;
	tip::iri::grammar::parse::iprivate_grammar< InputIterator > iprivate;
	boost::spirit::qi::rule< InputIterator, std::string() > query_char;
};

template < typename InputIterator >
struct query_grammar :
		boost::spirit::qi::grammar< InputIterator, request::query_type() > {
	typedef request::query_type value_type;

	query_grammar() : query_grammar::base_type(query)
	{
		using boost::spirit::qi::char_;
		using boost::spirit::qi::_val;
		using boost::spirit::qi::_1;
		using boost::phoenix::at_c;

		query %= query_param >> *('&' >> query_param);
	}

	boost::spirit::qi::rule< InputIterator, request::query_type() > query;
	query_param_grammar< InputIterator > query_param;

};

template < typename InputIterator >
struct request_first_line_grammar :
		boost::spirit::qi::grammar< InputIterator, request()> {
	typedef request value_type;
	request_first_line_grammar() : request_first_line_grammar::base_type(req)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;
		using qi::_val;
		using qi::_1;

		version %= qi::lit("HTTP/") >> qi::int_ >> '.' >> qi::int_;
		crlf = qi::lit("\r\n");
		req = 	method				[ phx::bind(&request::method, _val) = _1 ] 	>>
				+qi::space 														>>
				ipath				[ phx::bind(&request::path, _val) = _1 ] 	>>
				-('?' >> -query		[ phx::bind(&request::query, _val) = _1 ]) 	>>
				-('#' >> -fragment	[ phx::bind(&request::fragment, _val) = _1 ]) >>
				+qi::space 														>>
				version				[ phx::bind(&request::version, _val) = _1 ] >>
				crlf;
	}
	boost::spirit::qi::rule< InputIterator, value_type()> req;
	method_grammar method;
	iri::grammar::parse::ipath_absolute_grammar< InputIterator > ipath;
	query_grammar< InputIterator > query;
	iri::grammar::parse::ifragment_grammar< InputIterator > fragment;
	boost::spirit::qi::rule< InputIterator, request::version_type() > version;
	boost::spirit::qi::rule< InputIterator > crlf;
};

template < typename InputIterator >
struct request_grammar : boost::spirit::qi::grammar< InputIterator, request() > {
	typedef request value_type;
	request_grammar() : request_grammar::base_type(req)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;
		using qi::_val;
		using qi::_1;

		crlf = qi::lit("\r\n");
		req =	first_line[ _val = _1 ] >>
				_headers[ phx::bind(&request::headers_, _val) = _1 ] >>
				crlf;
	}
	boost::spirit::qi::rule< InputIterator, request() > req;
	request_first_line_grammar< InputIterator > first_line;
	boost::spirit::qi::rule< InputIterator > crlf;
	headers_grammar< InputIterator > _headers;
};

}  // namespace parse
}  // namespace grammar
}  // namespace http
}  // namespace tip


#endif /* TIP_HTTP_COMMON_GRAMMAR_REQUEST_PARSE_HPP_ */
