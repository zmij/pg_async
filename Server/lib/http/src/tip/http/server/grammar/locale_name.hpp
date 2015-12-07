/**
 * locale_name.hpp
 *
 *  Created on: 10 окт. 2015 г.
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_GRAMMAR_LOCALE_NAME_HPP_
#define TIP_HTTP_SERVER_GRAMMAR_LOCALE_NAME_HPP_

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include <tip/http/server/locale_name.hpp>

namespace tip {
namespace http {
namespace server {
namespace grammar {
namespace parse {
template < typename InputIterator >
struct locale_name_grammar :
		boost::spirit::qi::grammar< InputIterator, locale_name() > {
	typedef locale_name value_type;
	locale_name_grammar() : locale_name_grammar::base_type(root)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;
		using qi::lit;
		using qi::char_;
		using qi::_val;
		using qi::_1;

		segment %= qi::alpha >> +qi::alnum;
		encoding %= qi::alpha >> +(qi::alnum | qi::punct);

		root = segment[ phx::bind(&value_type::language, _val) = _1 ]
				>> -((lit("_") | lit("-")) >> segment[ phx::bind(&value_type::culture, _val) = _1 ])
				>> -(lit(".") >> encoding[ phx::bind(&value_type::encoding, _val) = _1 ])
						;
	}
	boost::spirit::qi::rule< InputIterator, value_type() > root;
	boost::spirit::qi::rule< InputIterator, std::string() > segment;
	boost::spirit::qi::rule< InputIterator, std::string() > encoding;
};

template < typename InputIterator, typename Container >
struct locale_names_grammar :
		boost::spirit::qi::grammar< InputIterator, Container() > {
	locale_names_grammar() : locale_names_grammar::base_type(root)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;
		using qi::_val;
		using qi::_1;
		using qi::_2;

		root %= qi::skip(qi::punct | qi::space)[name_grammar >> *name_grammar];
	}

	boost::spirit::qi::rule< InputIterator, Container() > root;
	locale_name_grammar< InputIterator > name_grammar;
};

}  // namespace parse
namespace gen {

template < typename OutputIterator >
struct locale_name_grammar :
			boost::spirit::karma::grammar< OutputIterator, locale_name() > {
	locale_name_grammar() : locale_name_grammar::base_type(root)
	{
		namespace karma = boost::spirit::karma;
		namespace phx = boost::phoenix;
		using karma::_val;
		using karma::_pass;
		using karma::_1;

		root = karma::string[ _1 = phx::bind( &locale_name::language, _val ) ]
			<< -('_' << karma::string[ _1 = phx::bind( &locale_name::culture, _val) ])
				[ _pass = !phx::empty(phx::bind( &locale_name::culture, _val)) ]
			<< -('.' << karma::string[ _1 = phx::bind( &locale_name::encoding, _val) ])
				[ _pass = !phx::empty(phx::bind( &locale_name::encoding, _val)) ]
		;
	}
	boost::spirit::karma::rule< OutputIterator, locale_name() > root;
};

}  // namespace gen
}  // namespace grammar
}  // namespace server
}  // namespace http
}  // namespace tip

#endif /* TIP_HTTP_SERVER_GRAMMAR_LOCALE_NAME_HPP_ */
