/*
 * request_generate.hpp
 *
 *  Created on: Aug 19, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_GRAMMAR_REQUEST_GENERATE_HPP_
#define TIP_HTTP_COMMON_GRAMMAR_REQUEST_GENERATE_HPP_

#include <boost/spirit/include/support_adapt_adt_attributes.hpp>
#include <tip/http/common/grammar/header_generate.hpp>
#include <tip/iri/grammar/iri_generate.hpp>
#include <tip/http/common/request.hpp>

BOOST_FUSION_ADAPT_ADT(
tip::http::request,
(tip::http::request_method, tip::http::request_method, obj.method, /**/)
(tip::iri::path&, tip::iri::path const&, obj.path, /**/)
(tip::http::request::query_type&, tip::http::request::query_type const&, obj.query, /**/)
(tip::iri::fragment&, tip::iri::fragment const&, obj.fragment, /**/)
(tip::http::request::version_type&, tip::http::request::version_type const&, obj.version, /**/)
(tip::http::headers&, tip::http::headers const&, obj.headers_, /**/)
);

namespace tip {
namespace http {
namespace grammar {
namespace gen {

struct method_grammar :
		boost::spirit::karma::symbols<request_method, char const*> {
	method_grammar();
};

template < typename OutputIterator >
struct query_param_grammar :
		boost::spirit::karma::grammar< OutputIterator, request::query_param_type()> {
	query_param_grammar() : query_param_grammar::base_type(query_param)
	{
		namespace karma = boost::spirit::karma;
		namespace phx = boost::phoenix;
		using karma::_val;
		using karma::_a;
		using karma::_1;

		iri::grammar::gen::is_hex_escaped ihe;
		// TODO iprivate range check
		iprivate = "%x" << +karma::xdigit;
		param_name = karma::eps[ _a = phx::begin(_val) ] <<
			+(
				iprivate[ihe] |
				(unreserved
					| karma::char_("!$'()*+,;:@/?")
					| pct_encoded)[ karma::_pass = _a != phx::end(_val), _1 = *_a++ ]
			);
		param_value = karma::eps[ _a = phx::begin(_val) ] <<
			*(
				iprivate[ihe] |
				(unreserved
					| karma::char_("!$'()*+,;:@/?=")
					| pct_encoded)[ karma::_pass = _a != phx::end(_val), _1 = *_a++ ]
			);
		query_param = param_name << '=' << param_value;
	}
	boost::spirit::karma::rule< OutputIterator, request::query_param_type()> query_param;
	tip::iri::grammar::gen::unreserved_grammar< OutputIterator > unreserved;
	tip::iri::grammar::gen::pct_encoded_grammar< OutputIterator > pct_encoded;

	boost::spirit::karma::rule< OutputIterator, std::string()> iprivate;
	boost::spirit::karma::rule< OutputIterator, std::string(),
			boost::spirit::karma::locals< std::string::const_iterator >> param_name;
	boost::spirit::karma::rule< OutputIterator, std::string(),
			boost::spirit::karma::locals< std::string::const_iterator >> param_value;
};

template < typename OutputIterator >
struct query_grammar :
		boost::spirit::karma::grammar< OutputIterator, request::query_type()> {
	query_grammar() : query_grammar::base_type(root)
	{
		namespace karma = boost::spirit::karma;
		root %= -(query_param << *('&' << query_param));
	}
	boost::spirit::karma::rule< OutputIterator, request::query_type()> root;
	query_param_grammar< OutputIterator > query_param;
};

template < typename OutputIterator >
struct request_grammar :
		boost::spirit::karma::grammar< OutputIterator, request()> {
	typedef boost::spirit::context<
		boost::fusion::cons< request const&, boost::fusion::nil_ >,
		boost::fusion::vector2<bool, bool>
	> context_type;
	request_grammar() : request_grammar::base_type(root)
	{
		namespace karma = boost::spirit::karma;
		namespace phx = boost::phoenix;
		using karma::eps;
		using karma::lit;
		using karma::_pass;
		using karma::_val;
		using karma::_a;
		using karma::_b;

		version = "HTTP/" << karma::int_ << "." << karma::int_;
		crlf = lit("\r\n");
		query = eps[ _pass = !phx::empty(_val) ]
			<< lit('?') << query_format;
		fragment = eps [ _pass = !phx::empty(_val) ]
			<< karma::lit('#') << fragment_format;
		root = method << ' '
			<< ipath << -query << -fragment << ' '
			<< version << crlf
			<< headers;
	}
	boost::spirit::karma::rule< OutputIterator, request()> root;
	method_grammar method;
	iri::grammar::gen::ipath_grammar< OutputIterator > ipath;
	query_grammar< OutputIterator > query_format;
	boost::spirit::karma::rule< OutputIterator, request::query_type()> query;
	iri::grammar::gen::ifragment_grammar< OutputIterator > fragment_format;
	boost::spirit::karma::rule< OutputIterator, iri::fragment()> fragment;
	boost::spirit::karma::rule< OutputIterator, request::version_type()> version;
	boost::spirit::karma::rule< OutputIterator> crlf;
	headers_grammar< OutputIterator > headers;
};

}  // namespace gen
}  // namespace grammar
}  // namespace http
}  // namespace tip


#endif /* TIP_HTTP_COMMON_GRAMMAR_REQUEST_GENERATE_HPP_ */
