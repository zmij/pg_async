/*
 * cookie_parse.hpp
 *
 *  Created on: Aug 26, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_GRAMMAR_COOKIE_PARSE_HPP_
#define TIP_HTTP_COMMON_GRAMMAR_COOKIE_PARSE_HPP_

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
//#include <boost/spirit/include/su

#include <tip/http/common/cookie.hpp>

#include <tip/http/common/grammar/datetime_parse.hpp>
#include <tip/iri/grammar/iri_parse.hpp>

namespace tip {
namespace http {
namespace grammar {
namespace parse {

template < typename InputIterator >
struct ctl_grammar :
		boost::spirit::qi::grammar< InputIterator, char()> {
	typedef char value_type;
	ctl_grammar() : ctl_grammar::base_type(ctl)
	{
		namespace qi = boost::spirit::qi;
		ctl = qi::char_((char)0, (char)31) | qi::char_(127);
	}
	boost::spirit::qi::rule< InputIterator, value_type()> ctl;
};

template < typename InputIterator >
struct separators_grammar :
		boost::spirit::qi::grammar< InputIterator, char()> {
	typedef char value_type;
	separators_grammar() : separators_grammar::base_type(separator)
	{
		namespace qi = boost::spirit::qi;
		separator = qi::char_("()<>@,:;\\\"/[]?={} \t");
	}
	boost::spirit::qi::rule< InputIterator, value_type()> separator;
};

/**
 * Sub-delims without ';'
 */
template < typename InputIterator >
struct sub_delims_nsc_grammar : boost::spirit::qi::grammar< InputIterator, char() > {
	sub_delims_nsc_grammar() : sub_delims_nsc_grammar::base_type(sub_delims)
	{
		using boost::spirit::qi::char_;
		sub_delims %= char_("!$&'()*+,=");
	}
	boost::spirit::qi::rule< InputIterator, char() > sub_delims;
};


template < typename InputIterator >
struct cookie_grammar :
		boost::spirit::qi::grammar< InputIterator, cookie()> {
	typedef cookie value_type;
	cookie_grammar() : cookie_grammar::base_type(root)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;
		token %= +(qi::char_ - ctl - separators);
		value %= *(~qi::char_(';'));

		root = token[phx::bind(&cookie::name, qi::_val) = qi::_1] >> '=' >>
				value[phx::bind(&cookie::value, qi::_val) = qi::_1];
	}
	boost::spirit::qi::rule< InputIterator, value_type()> root;
	boost::spirit::qi::rule< InputIterator, std::string()> token;
	ctl_grammar< InputIterator > ctl;
	separators_grammar< InputIterator > separators;
	boost::spirit::qi::rule< InputIterator, std::string()> value;
};

template < typename InputIterator >
struct cookies_grammar :
		boost::spirit::qi::grammar< InputIterator, request_cookies()> {
	typedef request_cookies value_type;
	cookies_grammar() : cookies_grammar::base_type(cookies)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;

		cookies = _cookie[ phx::push_back(qi::_val, qi::_1) ] >>
				*(qi::lit(';') >> *qi::space >> _cookie[ phx::push_back(qi::_val, qi::_1) ]);
	}
	boost::spirit::qi::rule< InputIterator, value_type()> cookies;
	cookie_grammar< InputIterator > _cookie;
};

template < typename InputIterator >
struct set_cookie_grammar :
		boost::spirit::qi::grammar< InputIterator, cookie()> {

	typedef cookie value_type;
	typedef sub_delims_nsc_grammar< InputIterator > sub_delims_type;
	typedef tip::iri::grammar::parse::ipath_absolute_grammar_base<
			InputIterator, sub_delims_type> ipath_type;
	typedef tip::iri::grammar::parse::ihost_grammar_base<
			InputIterator, sub_delims_type> ihost_type;
	set_cookie_grammar() : set_cookie_grammar::base_type(root)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;

		expires_av	= qi::lit("Expires=")	>> datetime;
		max_age_av	= qi::lit("Max-Age=")	>> qi::int_;
		domain_av	= qi::lit("Domain=")	>> host;
		path_av		= qi::lit("Path=")		>> path;
		secure_av	= qi::string("Secure")[ qi::_val = true ];
		http_only_av= qi::string("HttpOnly")[ qi::_val = true ];

		cookie_av =
				expires_av	[ phx::bind(&cookie::expires, qi::_val) = qi::_1 ] |
				max_age_av	[ phx::bind(&cookie::max_age, qi::_val) = qi::_1 ] |
				domain_av	[ phx::bind(&cookie::domain,  qi::_val) = qi::_1 ] |
				path_av 	[ phx::bind(&cookie::path,    qi::_val) = qi::_1 ] |
				secure_av	[ phx::bind(&cookie::secure,  qi::_val) = qi::_1 ] |
				http_only_av[ phx::bind(&cookie::http_only,qi::_val) = qi::_1 ];

		root = cookie_pair >> *( ';' >> +qi::space >> cookie_av );
	}
	boost::spirit::qi::rule< InputIterator, value_type()> root;
	boost::spirit::qi::rule< InputIterator, value_type()> cookie_av;

	http_datetime_grammar< InputIterator > datetime;
	cookie_grammar< InputIterator > cookie_pair;

	ihost_type host;
	ipath_type path;

	boost::spirit::qi::rule< InputIterator, cookie::datetime_type()> expires_av;
	boost::spirit::qi::rule< InputIterator, std::int32_t()> max_age_av;
	boost::spirit::qi::rule< InputIterator, iri::host()> domain_av;
	boost::spirit::qi::rule< InputIterator, iri::path()> path_av;
	boost::spirit::qi::rule< InputIterator, bool()> secure_av;
	boost::spirit::qi::rule< InputIterator, bool()> http_only_av;
};

}  // namespace parse
}  // namespace grammar
}  // namespace http
}  // namespace tip

#endif /* TIP_HTTP_COMMON_GRAMMAR_COOKIE_PARSE_HPP_ */
