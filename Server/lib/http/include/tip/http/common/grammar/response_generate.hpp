/*
 * response_generate.hpp
 *
 *  Created on: Aug 25, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_GRAMMAR_RESPONSE_GENERATE_HPP_
#define TIP_HTTP_COMMON_GRAMMAR_RESPONSE_GENERATE_HPP_

#include <boost/spirit/include/support_adapt_adt_attributes.hpp>
#include <tip/http/common/grammar/header_generate.hpp>
#include <tip/iri/grammar/iri_generate.hpp>
#include <tip/http/common/response.hpp>

BOOST_FUSION_ADAPT_ADT(
tip::http::response,
(tip::http::response::version_type&, tip::http::response::version_type const&, obj.version, /**/)
(int, int, obj.status, /**/)
(std::string&, std::string const&, obj.status_line, /**/)
(tip::http::headers&, tip::http::headers const&, obj.headers_, /**/)
);

namespace tip {
namespace http {
namespace grammar {
namespace gen {

template < typename OutputIterator >
struct response_grammar :
		boost::spirit::karma::grammar< OutputIterator, response()> {
	response_grammar() : response_grammar::base_type(root)
	{
		namespace karma = boost::spirit::karma;
		version = "HTTP/" << karma::int_ << "." << karma::int_;
		crlf = karma::lit("\r\n");

		root = version << ' ' << karma::int_ << ' ' << karma::string << crlf
				<< headers;
	}
	boost::spirit::karma::rule< OutputIterator, response()> root;
	boost::spirit::karma::rule< OutputIterator, response::version_type()> version;
	boost::spirit::karma::rule< OutputIterator> crlf;
	headers_grammar< OutputIterator > headers;
};

template < typename OutputIterator >
struct stock_response_grammar :
		boost::spirit::karma::grammar< OutputIterator, response()> {
	typedef response value_type;
	stock_response_grammar() : stock_response_grammar::base_type(root)
	{
		namespace karma = boost::spirit::karma;
		namespace phx = boost::phoenix;
		using karma::_val;
		using karma::_1;
		using karma::_2;
		using karma::_3;
		root =
		(
			"<html>"
			"<head><title>" << karma::string << "</title></head>"
			"<body><h1>" << karma::int_ << " " << karma::string << "</h1></body>"
			"</html>"
		)[
		  _1 = (phx::bind(&response::status_line, _val)),
		  _2 = (phx::bind(&response::status, _val)),
		  _3 = (phx::bind(&response::status_line, _val))
		];
	}
	boost::spirit::karma::rule< OutputIterator, value_type()> root;
};


}  // namespace gen
}  // namespace grammar
}  // namespace http
}  // namespace tip

#endif /* TIP_HTTP_COMMON_GRAMMAR_RESPONSE_GENERATE_HPP_ */
