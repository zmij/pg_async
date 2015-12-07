/**
 * locale_name.cpp
 *
 *  Created on: 10 окт. 2015 г.
 *      Author: zmij
 */

#include <tip/http/server/locale_name.hpp>
#include <tip/http/server/grammar/locale_name.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>
#include <boost/algorithm/string.hpp>
#include <tip/util/string_predicate.hpp>
#include <iostream>

namespace tip {
namespace http {
namespace server {

locale_name::locale_name() {
}

locale_name::locale_name(std::string const& lcname)
{
	typedef std::string::const_iterator input_iterator;
	typedef grammar::parse::locale_name_grammar< input_iterator > grammar_type;
	static grammar_type locale_name_grammar;

	std::string::const_iterator f = lcname.begin();
	std::string::const_iterator l = lcname.end();
	if (!boost::spirit::qi::parse(f, l, locale_name_grammar, *this) || f != l)
		throw std::runtime_error("Failed to parse locale name");
}

locale_name::locale_name(std::string const& lang, std::string const& cult,
		std::string const& enc) : language(lang), culture(cult), encoding(enc)
{
}

locale_name::operator std::string() const
{
	typedef std::back_insert_iterator< std::string > output_iterator;
	typedef grammar::gen::locale_name_grammar< output_iterator > grammar_type;
	static grammar_type locale_name_grammar;
	std::string res;
	boost::spirit::karma::generate(std::back_inserter(res), locale_name_grammar, *this);
	return res;
}

int
locale_name::compare(locale_name const& rhs, compare_options opts) const
{
	int res = util::icompare(language, rhs.language);
	if (!res) {
		if (!(opts & ignore_culture))
			res = util::icompare(culture, rhs.culture);
		if (!res && !(opts & ignore_encoding)) {
			res = util::icompare(encoding, rhs.encoding);
		}
	}
	return res;
}

std::ostream&
operator << (std::ostream& os, locale_name const& val)
{
	typedef std::ostream_iterator<char> output_iterator;
	typedef grammar::gen::locale_name_grammar< output_iterator > grammar_type;
	static grammar_type locale_name_grammar;
	std::ostream::sentry s(os);
	if (s) {
		output_iterator it(os);
		boost::spirit::karma::generate(it, locale_name_grammar, val);
	}
	return os;
}

std::istream&
operator >> (std::istream& is, locale_name& val)
{
	typedef std::istreambuf_iterator<char> istreambuf_iterator;
	typedef boost::spirit::multi_pass< istreambuf_iterator > stream_iterator;
	typedef grammar::parse::locale_name_grammar< stream_iterator > grammar_type;
	static grammar_type locale_name_grammar;

	std::istream::sentry s(is);
	if (s) {
		stream_iterator f{istreambuf_iterator(is)};
		stream_iterator l{istreambuf_iterator()};
		boost::spirit::qi::parse(f, l, locale_name_grammar, val);
	}
	return is;
}

} /* namespace server */
} /* namespace http */
} /* namespace tip */
