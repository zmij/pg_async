/*
 * cookie_generate.hpp
 *
 *  Created on: Aug 27, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_GRAMMAR_COOKIE_GENERATE_HPP_
#define TIP_HTTP_COMMON_GRAMMAR_COOKIE_GENERATE_HPP_

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <tip/http/common/cookie.hpp>
#include <tip/http/common/grammar/datetime_generate.hpp>
#include <tip/iri/grammar/iri_generate.hpp>

namespace tip {
namespace http {
namespace grammar {
namespace gen {

template < typename OutputIterator >
struct cookie_grammar :
		boost::spirit::karma::grammar< OutputIterator, cookie()> {
	typedef cookie value_type;
	cookie_grammar() : cookie_grammar::base_type(root)
	{
		namespace karma = boost::spirit::karma;
		namespace phx = boost::phoenix;
		using karma::_val;
		using karma::_1;
		using karma::_2;
		root = karma::string[ _1 = phx::bind(&cookie::name, _val) ] << '='
			<< karma::string[ _1 = phx::bind(&cookie::value, _val) ];
	}
	boost::spirit::karma::rule< OutputIterator, value_type()> root;
};

template < typename OutputIterator >
struct cookies_grammar :
		boost::spirit::karma::grammar< OutputIterator, request_cookies()> {
	typedef request_cookies value_type;
	cookies_grammar() : cookies_grammar::base_type(cookies)
	{
		namespace karma = boost::spirit::karma;
		cookies %= cookie_ << *( "; " << cookie_ );
	}
	boost::spirit::karma::rule< OutputIterator, value_type()> cookies;
	boost::spirit::karma::rule< OutputIterator, cookie()> cookie_;
};

template < typename OutputIterator >
struct set_cookie_grammar :
		boost::spirit::karma::grammar< OutputIterator, cookie()> {
	typedef cookie value_type;
	set_cookie_grammar() : set_cookie_grammar::base_type(set_cookie)
	{
		namespace karma = boost::spirit::karma;
		namespace phx = boost::phoenix;
		using karma::_pass;
		using karma::_val;
		using karma::_1;

		expires_av = ("; Expires=" << datetime)
			[
				_pass = phx::static_cast_<bool>(phx::bind(&cookie::expires, _val)),
				phx::if_(_pass) [
					_1 = *phx::bind(&cookie::expires, _val)
				]
			];
		max_age_av = ("; Max-Age=" << karma::int_)
			[
				_pass = phx::static_cast_<bool>(phx::bind(&cookie::max_age, _val)),
				phx::if_(_pass) [
					_1 = *phx::bind(&cookie::max_age, _val)
				]
			];
		domain_av = ("; Domain=" << karma::string)
			[
				_pass = phx::static_cast_<bool>(phx::bind(&cookie::domain, _val)),
				phx::if_(_pass) [
					_1 = *phx::bind(&cookie::domain, _val)
				]
			];
		path_av = ("; Path=" << path)
			[
				_pass = phx::static_cast_<bool>(phx::bind(&cookie::path, _val)),
				phx::if_(_pass) [
					_1 = *phx::bind(&cookie::path, _val)
				]
			];
		secure_av = ("; Secure" << karma::omit[karma::bool_])
			[
				_pass = phx::bind(&cookie::secure, _val),
				_1 = _pass
			];
		http_only_av = ("; HttpOnly" << karma::omit[karma::bool_])
			[
				_pass = phx::bind(&cookie::http_only, _val),
				_1 = _pass
			];

		set_cookie = cookie_pair
			<< -expires_av
			<< -max_age_av
			<< -domain_av
			<< -path_av
			<< -secure_av
			<< -http_only_av;
	}
	boost::spirit::karma::rule< OutputIterator, value_type()> set_cookie,
			expires_av, max_age_av, domain_av, path_av, secure_av, http_only_av;
	cookie_grammar< OutputIterator > cookie_pair;
	http_datetime_grammar< OutputIterator > datetime;
	iri::grammar::gen::ipath_grammar< OutputIterator > path;
};

}  // namespace gen
}  // namespace grammar
}  // namespace http
}  // namespace tip


#endif /* TIP_HTTP_COMMON_GRAMMAR_COOKIE_GENERATE_HPP_ */

