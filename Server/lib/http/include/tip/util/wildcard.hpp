/*
 * wildcard.hpp
 *
 *  Created on: Aug 29, 2015
 *      Author: zmij
 */

#ifndef TIP_UTIL_WILDCARD_HPP_
#define TIP_UTIL_WILDCARD_HPP_

#include <string>
#include <iostream>

namespace tip {
namespace util {

/**
 * Wildcard precedence named -> single -> kleene
 */
template < typename T >
class wildcard {
public:
	wildcard() : name_(), kleene_(false) {}
	explicit
	wildcard(std::string const& name) : name_(name), kleene_(false) {}
	explicit
	wildcard(bool kleene) : name_(), kleene_(kleene) {}

	inline std::string const&
	name() const
	{
		return name_;
	}
	inline bool
	unnamed() const
	{
		return name_.empty();
	}
	inline bool
	kleene() const
	{
		return kleene_;
	}
private:
	std::string name_;
	bool kleene_;
};

template < typename T >
inline bool
operator == (wildcard<T> const& lhs, wildcard<T> const& rhs)
{
	if (lhs.kleene()) {
		return rhs.kleene();
	} else if (!rhs.kleene()){
		return lhs.unnamed() == rhs.unnamed();
	}
	return false;
}
template < typename T >
inline bool
operator != (wildcard<T> const& lhs, wildcard<T> const& rhs)
{
	return !(lhs == rhs);
}
template < typename T >
inline bool
operator < (wildcard<T> const& lhs, wildcard<T> const& rhs)
{
	if (lhs.kleene() != rhs.kleene()) {
		return !lhs.kleene();
	}
	if (!lhs.kleene()) {
		if (lhs.unnamed() != rhs.unnamed()) {
			return rhs.unnamed();
		}
	}
	return false;
}
template < typename T >
inline bool
operator > (wildcard<T> const& lhs, wildcard<T> const& rhs)
{
	return rhs < lhs;
}

template < typename T >
inline bool
operator == (wildcard<T> const& lhs, T const& rhs)
{
	return true;
}
template < typename T >
inline bool
operator == (T const& lhs, wildcard<T> const& rhs)
{
	return true;
}

template < typename T >
inline bool
operator != (wildcard<T> const& lhs, T const& rhs)
{
	return false;
}
template < typename T >
inline bool
operator != (T const& lhs, wildcard<T> const& rhs)
{
	return false;
}

template < typename T >
inline bool
operator < (wildcard<T> const& lhs, T const& rhs)
{
	return false;
}
template < typename T >
inline bool
operator < (T const& lhs, wildcard<T> const& rhs)
{
	return false;
}

template < typename T >
std::ostream&
operator << (std::ostream& out, wildcard<T> const& val)
{
	std::ostream::sentry s(out);
	if (s) {
		if (val.kleene()) {
			out << '*';
		} else if (!val.name().empty()) {
			out << ':' << val.name() << ':';
		} else {
			out << '?';
		}
	}
	return out;
}


}  // namespace util
}  // namespace tip



#endif /* TIP_UTIL_WILDCARD_HPP_ */
