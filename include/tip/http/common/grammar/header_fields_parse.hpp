/*
 * header_fields_parse.hpp
 *
 *  Created on: Aug 28, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_GRAMMAR_HEADER_FIELDS_PARSE_HPP_
#define TIP_HTTP_COMMON_GRAMMAR_HEADER_FIELDS_PARSE_HPP_

#include <boost/spirit/include/qi.hpp>
#include <tip/http/common/header.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/fusion/include/std_pair.hpp>

namespace tip {
namespace http {
namespace grammar {
namespace parse {

template < typename InputIterator >
struct accept_language_grammar :
		boost::spirit::qi::grammar< InputIterator, accept_language()> {
	typedef accept_language value_type;
	accept_language_grammar() : accept_language_grammar::base_type(accept)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;
		namespace fsn = boost::fusion;
		using qi::_val;
		using qi::_1;

		language_range %= qi::repeat(1,8)[qi::alpha] >>
				-( qi::char_('-') >> qi::repeat(1,8)[qi::alpha]);
		qvalue = (qi::lit(";q=") >> qi::float_[ _val = _1 ]) | qi::eps[ _val = 1.0f ];
		accept %= language_range >> qvalue;
	}
	boost::spirit::qi::rule< InputIterator, value_type()> accept;
	boost::spirit::qi::rule< InputIterator, std::string()> language_range;
	boost::spirit::qi::rule< InputIterator, float() > qvalue;
};

template < typename InputIterator >
struct accept_languages_grammar :
		boost::spirit::qi::grammar< InputIterator, accept_languages() > {
	accept_languages_grammar() : accept_languages_grammar::base_type(root)
	{
		namespace qi = boost::spirit::qi;
		accept_repeat = "," >> *qi::space >> accept[ qi::_val = qi::_1 ];
		root %= accept >> *accept_repeat;
	}
	boost::spirit::qi::rule< InputIterator, accept_languages() > root;
	accept_language_grammar< InputIterator > accept;
	boost::spirit::qi::rule< InputIterator, accept_language() > accept_repeat;
};


}  // namespace parse
}  // namespace grammar
}  // namespace http
}  // namespace tip

#endif /* TIP_HTTP_COMMON_GRAMMAR_HEADER_FIELDS_PARSE_HPP_ */
