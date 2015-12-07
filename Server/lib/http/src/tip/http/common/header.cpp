/*
 * header.cpp
 *
 *  Created on: Aug 24, 2015
 *      Author: zmij
 */

#include <tip/http/common/header.hpp>
#include <boost/spirit/include/qi.hpp>
#include <tip/http/common/grammar/header_generate.hpp>

namespace tip {
namespace http {

std::ostream&
operator << (std::ostream& out, header const& val)
{
	namespace karma = boost::spirit::karma;
	typedef std::ostream_iterator<char> output_iterator;
	typedef grammar::gen::header_grammar<output_iterator> header_grammar;
	std::ostream::sentry s(out);
	if (s) {
		output_iterator oi(out);
		karma::generate(oi, header_grammar(), val);
	}
	return out;
}


std::ostream&
operator << (std::ostream& out, headers const& val)
{
	namespace karma = boost::spirit::karma;
	typedef std::ostream_iterator<char> output_iterator;
	typedef grammar::gen::headers_grammar<output_iterator> headers_grammar;
	std::ostream::sentry s(out);
	if (s) {
		output_iterator oi(out);
		karma::generate(oi, headers_grammar(), val);
	}
	return out;
}


size_t
content_length(headers const& hdrs)
{
	namespace qi = boost::spirit::qi;
	typedef std::string::const_iterator string_iterator;
	auto p = hdrs.find(ContentLength);
	if (p != hdrs.end()) {
		string_iterator f = p->second.begin();
		string_iterator l = p->second.end();
		size_t res;
		if (qi::parse(f, l, qi::int_parser<size_t, 10>(), res) && f == l)
			return res;
		else
			throw std::runtime_error("Invalid Content-Length header");
	}
	return 0;
}

bool
chunked_transfer_encoding(headers const& hdrs)
{
	typedef std::string::const_iterator string_iterator;
	auto p = hdrs.find(TransferEncoding);
	if (p != hdrs.end()) {
		return p->second == "chunked";
	}
	return false;
}
}  // namespace http
}  // namespace tip
