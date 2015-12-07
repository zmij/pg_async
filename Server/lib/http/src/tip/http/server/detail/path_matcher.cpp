/*
 * path_matcher.cpp
 *
 *  Created on: Aug 29, 2015
 *      Author: zmij
 */

#include <tip/http/server/detail/path_matcher.hpp>
#include <tip/http/server/detail/pathmatcher_parse.hpp>
#include <algorithm>

namespace tip {
namespace http {
namespace server {
namespace detail {

match_visitor::match_visitor(path_iterator f, path_iterator l, match_function match) :
		first_(f), last_(l), match_(match)
{
}

boost::tribool
match_visitor::operator ()(std::string const& l) const
{
	if (first_ == last_)
		return false;
	if (*first_ == l) {
		++first_;
		return true;
	}
	return false;
}

boost::tribool
match_visitor::operator()(util::wildcard< std::string > const& w) const
{
	if (first_ == last_)
		return false;
	if (!w.kleene()) {
		if (!w.name().empty() && match_) {
			match_(w.name(), *first_);
		}
		++first_;
		return true;
	}
	++first_;
	return boost::indeterminate;
}

match_visitor::path_iterator
match_visitor::current() const
{
	return first_;
}

bool
match_visitor::finished() const
{
	return first_ == last_;
}

path_matcher::path_matcher() : match_sequence_()
{
}

path_matcher::path_matcher(std::string const& s) : match_sequence_()
{
	namespace qi = boost::spirit::qi;
	typedef std::string::const_iterator string_iterator;
	typedef grammar::parse::match_path_sequence_grammar< string_iterator >
		match_sequence_grammar;

	string_iterator f = s.begin();
	string_iterator l = s.end();
	if (!qi::parse(f, l, match_sequence_grammar(), match_sequence_) || f != l) {
		throw std::logic_error("Failed to parse path pattern");
	}
}

bool
path_matcher::operator == (path_matcher const& rhs) const
{
	return match_sequence_ == rhs.match_sequence_;
}
bool
path_matcher::operator < (path_matcher const& rhs) const
{
	typedef path_match_sequence::const_iterator iterator;
	if (empty() && !rhs.empty()) {
		return false;
	}
	if (rhs.empty()) {
		return true;
	}

	iterator lc = match_sequence_.begin();
	iterator le = match_sequence_.end();
	iterator rc = rhs.match_sequence_.begin();
	iterator re = rhs.match_sequence_.end();

	for (; lc != le && rc != re; ++lc, ++rc) {
		if (*lc < *rc)
			return true;
	}
	return lc != le;
}

bool
path_matcher::matches(iri::path const& p) const
{
	typedef path_match_sequence::const_iterator iterator;
	iterator matcher = match_sequence_.begin();
	iterator match_end = match_sequence_.end();
	if (matcher == match_end)
		return p.empty();

	match_visitor vis(p.begin(), p.end());
	for (; matcher != match_end;) {
		boost::tribool res = boost::apply_visitor(vis, *matcher);
		if (res) {
			++matcher;
		} else if (!res) {
			return matcher + 1 == match_end && vis.finished();
		}
	}
	return true;
}

std::ostream&
operator << (std::ostream& out, path_matcher const& val)
{
	std::ostream::sentry s(out);
	if (s) {
		for (auto& m : val.match_sequence_) {
			out << "/" << m;
		}
	}
	return out;
}


} /* namespace detail */
} /* namespace server */
} /* namespace http */
} /* namespace tip */
