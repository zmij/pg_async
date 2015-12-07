/**
 * string_predicate.hpp
 *
 *  Created on: 10 окт. 2015 г.
 *      Author: zmij
 */

#ifndef TIP_UTIL_STRING_PREDICATE_HPP_
#define TIP_UTIL_STRING_PREDICATE_HPP_

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator.hpp>
#include <boost/range/const_iterator.hpp>
#include <boost/range/as_literal.hpp>
#include <boost/range/iterator_range_core.hpp>

namespace tip {
namespace util {

namespace detail {

struct compare_pred {
	template< typename T1, typename T2 >
	int
	operator() (T1 const& lhs, T2 const& rhs) const
	{
		if (lhs < rhs)
			return -1;
		if (lhs > rhs)
			return 1;
		return 0;
	}
};

struct icompare_pred {
	icompare_pred(std::locale const& loc = std::locale()) : loc_(loc) {}
	template < typename T1, typename T2 >
	int
	operator() (T1 const& lhs, T2 const& rhs) const
	{
		return cmp_(std::toupper<T1>(lhs, loc_), std::toupper<T2>(rhs, loc_));
	}
private:
	std::locale loc_;
	compare_pred cmp_;
};

template < typename InputRange, typename TestRange, typename Cmp >
inline int
compare(InputRange const& input, TestRange const& test, Cmp cmp)
{
	typedef typename boost::range_const_iterator< InputRange >::type input_iterator;
	typedef typename boost::range_const_iterator< TestRange >::type test_iterator;

	boost::iterator_range< input_iterator > lit_input(boost::as_literal(input));
	boost::iterator_range< test_iterator > lit_test(boost::as_literal(test));

	input_iterator it = boost::begin(lit_input);
	input_iterator iend = boost::end(lit_input);
	test_iterator tt = boost::begin(lit_test);
	test_iterator tend = boost::end(lit_test);

	for (; it != iend && tt != tend; ++it, ++tt) {
		int r = cmp(*it, *tt);
		if (r != 0)
			return r;
	}
	if (it == iend && tt != tend)
		return -1;
	if (it != iend && tt == tend)
		return 1;
	return 0;
}

}  // namespace detail

template < typename InputRange, typename TestRange >
inline int
icompare(InputRange const& input, TestRange const& test, std::locale const& loc = std::locale())
{
	return detail::compare(input, test, detail::icompare_pred(loc));
}

}  // namespace util
}  // namespace tip


#endif /* TIP_UTIL_STRING_PREDICATE_HPP_ */
