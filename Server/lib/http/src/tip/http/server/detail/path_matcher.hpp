/*
 * path_matcher.hpp
 *
 *  Created on: Aug 29, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_DETAIL_PATH_MATCHER_HPP_
#define TIP_HTTP_SERVER_DETAIL_PATH_MATCHER_HPP_

#include <tip/iri.hpp>
#include <tip/util/wildcard.hpp>
#include <boost/variant.hpp>
#include <boost/logic/tribool.hpp>

namespace tip {
namespace http {
namespace server {
namespace detail {

typedef boost::variant< std::string, util::wildcard< std::string > > path_segment;
typedef std::vector< path_segment > path_match_sequence;

/**
 * A class to match a tip::iri::path
 * Concept
 * Must be created from a string in a form
 * match_string = / 1*path_segment
 * path_segment = segment_literal | wildcard
 * wildcard = ( '?' | '*' | named_capture)
 * named_capture = ':' token ':'
 * token = (alpha | '_') *(alnum | '_')
 */
class path_matcher {
public:
	path_matcher();
	explicit
	path_matcher(std::string const&);

	bool
	matches( tip::iri::path const& ) const;
	bool
	empty() const
	{
		return match_sequence_.empty();
	}

	bool
	operator == (path_matcher const& rhs) const;
	bool
	operator != (path_matcher const& rhs) const
	{ return !(*this == rhs); }
	bool
	operator < (path_matcher const& rhs) const;
	bool
	operator > (path_matcher const& rhs) const
	{ return rhs < *this; }
private:
	friend std::ostream&
	operator << (std::ostream&, path_matcher const&);
	path_match_sequence match_sequence_;
};

std::ostream&
operator << (std::ostream&, path_matcher const&);

/**
 * Path match visitor.
 * Operators return false if matching failed, true if matching finished,
 * indeterminate if the segment can consume more elements
 */
class match_visitor : public boost::static_visitor<boost::tribool> {
public:
	typedef iri::path::const_iterator path_iterator;
	typedef std::function< void( std::string const&, std::string const& ) > match_function;
public:
	match_visitor(path_iterator f, path_iterator l,
			match_function = match_function());

	boost::tribool
	operator()(std::string const& l) const;
	boost::tribool
	operator()(util::wildcard< std::string > const& w) const;

	path_iterator
	current() const;

	bool
	finished() const;
private:
	mutable path_iterator first_;
	path_iterator last_;
	match_function match_;
};

} /* namespace detail */
} /* namespace server */
} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_SERVER_DETAIL_PATH_MATCHER_HPP_ */
