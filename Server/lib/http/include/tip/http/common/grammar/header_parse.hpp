/*
 * header_grammar.hpp
 *
 *  Created on: Aug 18, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_GRAMMAR_HEADER_PARSE_HPP_
#define TIP_HTTP_COMMON_GRAMMAR_HEADER_PARSE_HPP_

#include <tip/http/common/header.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/std_pair.hpp>

namespace tip {
namespace http {
namespace grammar {
namespace parse {

struct known_header_name_grammar :
		boost::spirit::qi::symbols<char, known_header_name> {

	known_header_name_grammar();
};

template < typename InputIterator >
struct header_name_grammar :
		boost::spirit::qi::grammar< InputIterator, header_name() > {

	header_name_grammar() : header_name_grammar::base_type(field_name)
	{
		using boost::spirit::qi::alnum;
		using boost::spirit::qi::char_;

		field_name %= known_header | +(alnum | char_("-"));
	}

	boost::spirit::qi::rule< InputIterator, header_name() > field_name;
	known_header_name_grammar known_header;
};

template < typename InputIterator >
struct header_grammar :
		boost::spirit::qi::grammar< InputIterator, header() > {
	typedef header value_type;
	typedef boost::spirit::context<
			boost::fusion::cons< value_type&, boost::fusion::nil_ >,
			boost::fusion::vector0<>
		> context_type;

	struct value_ {
		void
		operator()(std::string const& v, context_type& ctx, bool& pass) const
		{
			boost::fusion::at_c<0>(ctx.attributes).second = v;
		}
	};
	header_grammar() : header_grammar::base_type(key_value)
	{
		using boost::spirit::qi::space;
		using boost::spirit::qi::char_;
		using boost::spirit::qi::lit;

		value %= +(~char_("\r"));
		key_value %= key >> *space >> lit(':')
				>> *space >> value[value_()] >> lit("\r\n");
	}
	boost::spirit::qi::rule< InputIterator, header() > key_value;
	header_name_grammar< InputIterator > key;
	boost::spirit::qi::rule< InputIterator, std::string() > value;
};

template < typename InputIterator >
struct headers_grammar :
		boost::spirit::qi::grammar< InputIterator, headers() > {
	headers_grammar() : headers_grammar::base_type(_headers)
	{
		_headers %= +_header;
	}

	boost::spirit::qi::rule< InputIterator, headers() > _headers;
	header_grammar< InputIterator > _header;
};

}  // namespace parse
}  // namespace grammar
}  // namespace http
}  // namespace tip


#endif /* TIP_HTTP_COMMON_GRAMMAR_HEADER_PARSE_HPP_ */
