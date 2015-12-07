/*
 * base64_generate.hpp
 *
 *  Created on: Aug 20, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_GRAMMAR_BASE64_GENERATE_HPP_
#define TIP_HTTP_COMMON_GRAMMAR_BASE64_GENERATE_HPP_

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <string>
#include <vector>

namespace tip {
namespace http {
namespace grammar {
namespace gen {

template < typename OutputIterator, typename ValueType >
struct base64_grammar_base :
		boost::spirit::karma::grammar< OutputIterator, ValueType()> {
	typedef ValueType value_type;
	typedef typename value_type::const_iterator element_iterator;
	typedef typename element_iterator::value_type element_type;

	base64_grammar_base(bool line_breaks = false) : base64_grammar_base::base_type(root)
	{
		namespace karma = boost::spirit::karma;
		namespace phx = boost::phoenix;
		using karma::_a;
		using karma::_b;
		using karma::_val;
		using karma::_pass;
		using karma::_1;
		using karma::_2;
		using karma::_3;
		using karma::_4;


		base64alpha.add
			( 0,  'A' )( 1,  'B' )( 2,  'C' )( 3,  'D' )
			( 4,  'E' )( 5,  'F' )( 6,  'G' )( 7,  'H' )
			( 8,  'I' )( 9,  'J' )( 10, 'K' )( 11, 'L' )
			( 12, 'M' )( 13, 'N' )( 14, 'O' )( 15, 'P' )
			( 16, 'Q' )( 17, 'R' )( 18, 'S' )( 19, 'T' )
			( 20, 'U' )( 21, 'V' )( 22, 'W' )( 23, 'X' )
			( 24, 'Y' )( 25, 'Z' )( 26, 'a' )( 27, 'b' )
			( 28, 'c' )( 29, 'd' )( 30, 'e' )( 31, 'f' )
			( 32, 'g' )( 33, 'h' )( 34, 'i' )( 35, 'j' )
			( 36, 'k' )( 37, 'l' )( 38, 'm' )( 39, 'n' )
			( 40, 'o' )( 41, 'p' )( 42, 'q' )( 43, 'r' )
			( 44, 's' )( 45, 't' )( 46, 'u' )( 47, 'v' )
			( 48, 'w' )( 49, 'x' )( 50, 'y' )( 51, 'z' )
			( 52, '0' )( 53, '1' )( 54, '2' )( 55, '3' )
			( 56, '4' )( 57, '5' )( 58, '6' )( 59, '7' )
			( 60, '8' )( 61, '9' )( 62, '+' )( 63, '/' );

		block_3 = ( (base64alpha << base64alpha << base64alpha << base64alpha)[
			phx::if_( phx::size( _val ) == 3ul ) [
				_pass = true,
				_a =
					( phx::static_cast_< std::uint32_t >( phx::at( _val, 0) ) << 16 ) |
					( phx::static_cast_< std::uint32_t >( phx::at( _val, 1) ) << 8 ) |
					  phx::static_cast_< std::uint32_t >( phx::at( _val, 2) ),
				_1 = _a >> 18,
				_2 = (_a >> 12) & 0x3f,
				_3 = (_a >> 6) & 0x3f,
				_4 = _a & 0x3f
			].else_[
				_pass = false
			]
		]);
		block_2 = ( (base64alpha << base64alpha << base64alpha << '=')[
			phx::if_( phx::size( _val ) == 2ul ) [
				_pass = true,
				_a =
					( phx::static_cast_< std::uint32_t >( phx::at( _val, 0) ) << 16 ) |
					( phx::static_cast_< std::uint32_t >( phx::at( _val, 1) ) << 8 ),
				_1 = _a >> 18,
				_2 = (_a >> 12) & 0x3f,
				_3 = (_a >> 6) & 0x3f
			].else_[
				_pass = false
			]
		]);
		block_1 = ( (base64alpha << base64alpha << '=' << '=')[
			phx::if_( phx::size( _val ) == 1ul ) [
				_pass = true,
				_a =
					( phx::static_cast_< std::uint32_t >( phx::at( _val, 0) ) << 16 ),
				_1 = _a >> 18,
				_2 = (_a >> 12) & 0x3f
			].else_[
				_pass = false
			]
		]);
		block = block_3 | block_2 | block_1;

		line = karma::eps[ _a = phx::begin(_val), _b = 1ul ] <<
			*( block[
				_pass = _a != phx::end(_val),
				phx::clear(_1),
				phx::push_back(_1, *(_a++)),
				phx::if_(_a != phx::end(_val)) [
					phx::push_back(_1, *(_a++)),
					phx::if_(_a != phx::end(_val)) [
						phx::push_back(_1, *(_a++))
					]
				]
			] << -karma::eol[
				_pass = line_breaks && _b % 19ul == 0ul,
				++_b
			] );
		root = line;
	}
	boost::spirit::karma::rule< OutputIterator, value_type()> root;
	boost::spirit::karma::rule< OutputIterator, value_type(),
			boost::spirit::karma::locals< std::uint32_t >> block_3;
	boost::spirit::karma::rule< OutputIterator, value_type(),
			boost::spirit::karma::locals< std::uint32_t >> block_2;
	boost::spirit::karma::rule< OutputIterator, value_type(),
			boost::spirit::karma::locals< std::uint32_t >> block_1;
	boost::spirit::karma::rule< OutputIterator, value_type()> block;
	boost::spirit::karma::rule< OutputIterator, value_type(),
			boost::spirit::karma::locals< element_iterator, size_t >> line;

	boost::spirit::karma::symbols< element_type, char > base64alpha;
};

template < typename OutputIterator >
using base64_grammar = base64_grammar_base< OutputIterator, std::string >;
template < typename OutputIterator >
using base64_grammar_v = base64_grammar_base < OutputIterator, std::vector<std::uint8_t> >;

template < typename OutputIterator, typename Input >
bool
base64_encode( OutputIterator sink, Input const& v, bool line_breaks = false)
{
	base64_grammar_base< OutputIterator, Input > encoder(line_breaks);
	return boost::spirit::karma::generate(sink, encoder, v);
}

}  // namespace gen
}  // namespace grammar
}  // namespace http
}  // namespace tip



#endif /* TIP_HTTP_COMMON_GRAMMAR_BASE64_GENERATE_HPP_ */
