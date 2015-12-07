/*
 * base64_parse.hpp
 *
 *  Created on: Aug 20, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_GRAMMAR_BASE64_PARSE_HPP_
#define TIP_HTTP_COMMON_GRAMMAR_BASE64_PARSE_HPP_

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <string>
#include <vector>

namespace tip {
namespace http {
namespace grammar {
namespace parse {

template < typename InputIterator, typename ValueType >
struct base64_grammar_base :
		boost::spirit::qi::grammar< InputIterator, ValueType()> {
	typedef ValueType value_type;
	typedef typename value_type::const_iterator element_iterator;
	typedef typename element_iterator::value_type element_type;

	base64_grammar_base() : base64_grammar_base::base_type(root)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;

		base64alpha.add
			( "A",  0 )( "B",  1 )( "C",  2 )( "D",  3 )
			( "E",  4 )( "F",  5 )( "G",  6 )( "H",  7 )
			( "I",  8 )( "J",  9 )( "K", 10 )( "L", 11 )
			( "M", 12 )( "N", 13 )( "O", 14 )( "P", 15 )
			( "Q", 16 )( "R", 17 )( "S", 18 )( "T", 19 )
			( "U", 20 )( "V", 21 )( "W", 22 )( "X", 23 )
			( "Y", 24 )( "Z", 25 )( "a", 26 )( "b", 27 )
			( "c", 28 )( "d", 29 )( "e", 30 )( "f", 31 )
			( "g", 32 )( "h", 33 )( "i", 34 )( "j", 35 )
			( "k", 36 )( "l", 37 )( "m", 38 )( "n", 39 )
			( "o", 40 )( "p", 41 )( "q", 42 )( "r", 43 )
			( "s", 44 )( "t", 45 )( "u", 46 )( "v", 47 )
			( "w", 48 )( "x", 49 )( "y", 50 )( "z", 51 )
			( "0", 52 )( "1", 53 )( "2", 54 )( "3", 55 )
			( "4", 56 )( "5", 57 )( "6", 58 )( "7", 59 )
			( "8", 60 )( "9", 61 )( "+", 62 )( "/", 63 );
		block = ( (base64alpha >> base64alpha >> base64alpha >> base64alpha)[
			qi::_a = (qi::_1 << 18) | (qi::_2 << 12) | (qi::_3 << 6) | qi::_4,
			phx::push_back(qi::_val, qi::_a >> 16),
			phx::push_back(qi::_val, (qi::_a >> 8) & 0xff),
			phx::push_back(qi::_val, qi::_a & 0xff)
		] );
		partial_block =
			( (base64alpha >> base64alpha >> base64alpha >> '=')[
				qi::_a = (qi::_1 << 18) | (qi::_2 << 12) | (qi::_3 << 6),
				phx::push_back(qi::_val, qi::_a >> 16),
				phx::push_back(qi::_val, (qi::_a >> 8) & 0xff)
			] ) |
			( (base64alpha >> base64alpha >> '=' >> '=')[
				qi::_a = (qi::_1 << 18) | (qi::_2 << 12),
				phx::push_back(qi::_val, qi::_a >> 16)
			] );
		root = qi::skip( qi::byte_ - base64alpha)[
			*block[ phx::insert(qi::_val, phx::end(qi::_val), phx::begin(qi::_1), phx::end(qi::_1)) ]
			>> -partial_block[ phx::insert(qi::_val, phx::end(qi::_val), phx::begin(qi::_1), phx::end(qi::_1)) ]
		];
	}
	boost::spirit::qi::rule< InputIterator, value_type()> root;
	boost::spirit::qi::rule< InputIterator, value_type(),
			boost::spirit::qi::locals< std::uint32_t >> block;
	boost::spirit::qi::rule< InputIterator, value_type(),
			boost::spirit::qi::locals< std::uint32_t >> partial_block;
	boost::spirit::qi::symbols< char, uint32_t > base64alpha;
};

template < typename InputIterator >
using base64_grammar = base64_grammar_base< InputIterator, std::string >;
template < typename InputIterator >
using base64_grammar_v = base64_grammar_base< InputIterator, std::vector< std::uint8_t > >;

template < typename InputIterator, typename Output >
bool
base64_decode( InputIterator first, InputIterator last, Output& v)
{
	base64_grammar_base< InputIterator, Output > decoder;
	return boost::spirit::qi::parse(first, last, decoder, v);
}

}  // namespace parse
}  // namespace grammar
}  // namespace http
}  // namespace tip

#endif /* TIP_HTTP_COMMON_GRAMMAR_BASE64_PARSE_HPP_ */
