/*
 * header_generate.hpp
 *
 *  Created on: Aug 19, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_GRAMMAR_HEADER_GENERATE_HPP_
#define TIP_HTTP_COMMON_GRAMMAR_HEADER_GENERATE_HPP_

#include <tip/http/common/header.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/fusion/include/adapt_adt.hpp>
#include <boost/spirit/include/support_adapt_adt_attributes.hpp>
#include <boost/fusion/include/std_pair.hpp>

namespace tip {
namespace http {
namespace grammar {
namespace gen {

struct known_header_name_grammar :
	boost::spirit::karma::symbols<known_header_name, char const*> {
	known_header_name_grammar();
};

template < typename OutputIterator >
struct header_name_grammar :
		boost::spirit::karma::grammar< OutputIterator, header_name() > {
	header_name_grammar() : header_name_grammar::base_type(field_name)
	{
		namespace karma = boost::spirit::karma;
		field_name = (known_header | karma::string);
	}

	boost::spirit::karma::rule< OutputIterator, header_name() > field_name;
	known_header_name_grammar known_header;
};

template < typename OutputIterator >
struct header_grammar :
		boost::spirit::karma::grammar< OutputIterator, header() > {

	header_grammar() : header_grammar::base_type(header_)
	{
		using boost::spirit::karma::string;
		header_ = key << ": " << string << "\r\n";
	}
	boost::spirit::karma::rule< OutputIterator, header() > header_;
	header_name_grammar< OutputIterator > key;
};

template < typename OutputIterator >
struct headers_grammar :
		boost::spirit::karma::grammar< OutputIterator, headers() > {
	headers_grammar() : headers_grammar::base_type(headers_)
	{
		headers_ = *header_;
	}
	boost::spirit::karma::rule< OutputIterator, headers() > headers_;
	header_grammar< OutputIterator > header_;
};

}  // namespace gen
}  // namespace grammar
}  // namespace http
}  // namespace tip

#endif /* TIP_HTTP_COMMON_GRAMMAR_HEADER_GENERATE_HPP_ */
