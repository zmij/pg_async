/*
 * iri.cpp
 *
 *  Created on: Aug 17, 2015
 *      Author: zmij
 */

#include <tip/iri/grammar/iri_parse.hpp>
#include <tip/iri.hpp>
#include <iostream>

namespace tip {
namespace iri {

path
path::parse(std::string const& s)
{
	namespace qi = boost::spirit::qi;
	typedef std::string::const_iterator string_iterator;
	typedef grammar::parse::ipath_grammar< string_iterator > ipath_grammar;

	string_iterator f = s.begin();
	string_iterator l = s.end();

	path p;
	if (!qi::parse(f, l, ipath_grammar(), p) || f != l) {
		throw std::runtime_error("Invalid path");
	}
	return p;
}

iri
parse_iri(std::string const& s)
{
	namespace qi = boost::spirit::qi;
	typedef std::string::const_iterator string_iterator;
	typedef grammar::parse::iri_grammar< string_iterator > iri_grammar;

	string_iterator f = s.begin();
	string_iterator l = s.end();

	iri res;
	if (!qi::parse(f, l, iri_grammar(), res) || f != l) {
		throw std::runtime_error("Invalid IRI");
	}

	return res;
}

std::ostream&
operator << (std::ostream& out, path const& val)
{
	std::ostream::sentry s(out);
	if (s) {
		if (val.is_rooted()) {
			out << "/";
		}
		path::const_iterator f = val.begin();
		path::const_iterator l = val.end();
		for (path::const_iterator p = f; p != l; ++p) {
			if (p != f)
				out << "/";
			out << *p;
		}
	}
	return out;
}

std::ostream&
operator << (std::ostream& out, host const& val)
{
	std::ostream::sentry s(out);
	if (s) {
		out << static_cast<std::string const&>(val);
	}
	return out;
}


std::ostream&
operator << (std::ostream& out, authority const& val)
{
	std::ostream::sentry s(out);
	if (s) {
		if (!val.userinfo.empty()) {
			out << val.userinfo << "@";
		}
		out << val.host;
		if (!val.port.empty()) {
			out << ":" << val.port;
		}
	}
	return out;
}

std::ostream&
operator << (std::ostream& out, basic_iri<query> const& val)
{
	std::ostream::sentry s(out);
	if (s) {
		if (!val.scheme.empty()) {
			out << val.scheme << ":";
		}
		if (!val.authority.empty()) {
			out << "//" << val.authority;
		}
		if (!val.path.empty()) {
			out << val.path;
		}
		if (!val.query.empty()) {
			out << '?' << val.query;
		}
		if (!val.fragment.empty()) {
			out << '#' << val.fragment;
		}
	}
	return out;
}


}  // namespace iri
}  // namespace tip
