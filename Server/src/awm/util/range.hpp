/*
 * range.hpp
 *
 *  Created on: Dec 16, 2015
 *      Author: zmij
 */

#ifndef AWM_UTIL_RANGE_HPP_
#define AWM_UTIL_RANGE_HPP_

#include <iostream>
#include <cstdint>

namespace awm {
namespace util {

enum class range_ends : uint8_t {
	exclusive		= 0,
	min_inclusive	= 1,
	max_inclusive	= 2,
	inclusive		= min_inclusive | max_inclusive
};

inline range_ends
operator | (range_ends a, range_ends b)
{
	return static_cast<range_ends>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline range_ends
operator & (range_ends a, range_ends b)
{
	return static_cast<range_ends>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

template < typename T >
struct range {
	typedef T value_type;

	value_type	min;
	value_type	max;
	range_ends	ends;
};

template < typename T >
bool
operator == (range<T> const& lhs, range<T> const& rhs)
{
	return lhs.min == rhs.min && lhs.max == rhs.max && lhs.ends == rhs.ends;
}

template < typename T >
std::ostream&
operator << (std::ostream& os, range< T > const& val)
{
	std::ostream::sentry s(os);
	if (s) {
		os	<< (((val.ends & range_ends::min_inclusive) != range_ends::exclusive) ? '[' : '(')
			<< val.min << "," << val.max
			<< (((val.ends & range_ends::max_inclusive) != range_ends::exclusive) ? ']' : ')');
	}
	return os;
}

template < typename T >
std::istream&
operator >> (std::istream& is, range< T >& val)
{
	std::istream::sentry s(is);
	if (s) {
		range< T > tmp { T(), T(), range_ends::exclusive };
		std::istream::char_type c;
		bool ok = false;
		if (is.get(c) && (c == '[' || c == '(')) {
			if (c == '[')
				tmp.ends = range_ends::min_inclusive;
			if ((bool)(is >> tmp.min) && is.get(c) && c == ','
					&& ((bool)(is >> tmp.max))
					&& is.get(c) && (c == ']' || c == ')')) {
				if (c == ']')
					tmp.ends = tmp.ends | range_ends::max_inclusive;
				val = tmp;
				ok = true;
			}
		}
		if (!ok)
			is.setstate(std::ios_base::badbit);
	}
	return is;
}


}  // namespace util
}  // namespace awm


#endif /* AWM_UTIL_RANGE_HPP_ */
