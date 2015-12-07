/*
 * iri.hpp
 *
 *  Created on: Aug 17, 2015
 *      Author: zmij
 */

#ifndef TIP_IRI_IRI_HPP_
#define TIP_IRI_IRI_HPP_

#include <string>
#include <vector>
#include <iosfwd>

namespace tip {
namespace iri {

class scheme : public std::string {
public:
	scheme() : std::string() {}
	explicit
	scheme(std::string const& s) : std::string(s) {}
};

class userinfo : public std::string {
public:
	userinfo() : std::string() {}
	explicit
	userinfo(std::string const& s) : std::string(s) {}
};

class host : public std::string {
public:
	host() : std::string() {}
	explicit
	host(std::string const& s) : std::string(s) {}
};

class port : public std::string {
public:
	port() : std::string() {}
	explicit
	port(std::string const& s) : std::string(s) {}
};

struct authority {
	class host host;
	class port port;
	class userinfo userinfo;

	bool
	operator == (authority const & rhs) const
	{ return host == rhs.host && port == rhs.port && userinfo == rhs.userinfo; }

	bool
	empty() const
	{ return host.empty(); }
};

class path : public std::vector< std::string > {
public:
	typedef std::vector< std::string > base_type;

	path() : base_type(), rooted_(false) {}
	path(bool rooted) : base_type(), rooted_(rooted) {}
	path(bool rooted, std::initializer_list<std::string> args) :
		base_type(args), rooted_(rooted) {}

	void
	swap(path& rhs)
	{
		std::swap(rooted_, rhs.rooted_);
		base_type::swap(rhs);
	}

	bool
	operator == (path const& rhs) const
	{
		return rooted_ == rhs.rooted_ &&
				static_cast<base_type const&>(*this) == static_cast<base_type const&>(rhs);
	}
	bool
	is_rooted() const
	{ return rooted_; }
	bool&
	is_rooted()
	{ return rooted_; }

	static path
	parse(std::string const&);
private:
	bool rooted_;
};

class query : public std::string {
public:
	query() : std::string() {}
	explicit
	query(std::string const& s) : std::string(s) {}
};

class fragment : public std::string {
public:
	fragment() : std::string() {}
	explicit
	fragment(std::string const& s) : std::string(s) {}
};

template < typename QueryType = query >
struct basic_iri {
	typedef QueryType query_type;
	class scheme	scheme;
	class authority	authority;
	class path		path;
	query_type		query;
	class fragment	fragment;

	bool
	operator == (basic_iri const& rhs) const
	{ return scheme == rhs.scheme && authority == rhs.authority &&
			path == rhs.path && query == rhs.query && fragment == rhs.fragment;
	}
	bool
	empty() const
	{ return authority.empty() && path.empty(); }
};

using iri = basic_iri<>;

iri
parse_iri(std::string const&);

std::ostream&
operator << (std::ostream&, path const&);
std::ostream&
operator << (std::ostream&, authority const&);
std::ostream&
operator << (std::ostream&, basic_iri< query > const&);
}  // namespace iri
}  // namespace tip

#endif /* TIP_IRI_IRI_HPP_ */
