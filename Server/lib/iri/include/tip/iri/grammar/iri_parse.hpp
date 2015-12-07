/*
 * uri_grammar.hpp
 *
 *  Created on: Aug 12, 2015
 *      Author: zmij
 */

#ifndef TIP_IRI_GRAMMAR_IRI_PARSE_HPP_
#define TIP_IRI_GRAMMAR_IRI_PARSE_HPP_

#include <string>
#include <array>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <boost/lexical_cast.hpp>

#include <tip/iri.hpp>

namespace tip {
namespace iri {
namespace grammar {
namespace parse {

template < typename InputIterator >
struct sub_delims_grammar : boost::spirit::qi::grammar< InputIterator, char() > {
	sub_delims_grammar() : sub_delims_grammar::base_type(sub_delims)
	{
		using boost::spirit::qi::char_;
		sub_delims %= char_("!$&'()*+,;=");
	}
	boost::spirit::qi::rule< InputIterator, char() > sub_delims;
};

template < typename InputIterator >
struct gen_delims_grammar : boost::spirit::qi::grammar< InputIterator, char() > {
	gen_delims_grammar() : gen_delims_grammar::base_type(gen_delims)
	{
		using boost::spirit::qi::char_;
		gen_delims %= char_(":/?#[]@");
	}
	boost::spirit::qi::rule< InputIterator, char() > gen_delims;
};

template < typename InputIterator >
struct reserved_grammar : boost::spirit::qi::grammar< InputIterator, char() > {
	reserved_grammar() : reserved_grammar::base_type(reserved)
	{
		reserved = gen_delims | sub_delims;
	}
	boost::spirit::qi::rule< InputIterator, char() > reserved;
	gen_delims_grammar< InputIterator > gen_delims;
	sub_delims_grammar< InputIterator > sub_delims;
};

template < typename InputIterator >
struct unreserved_grammar : boost::spirit::qi::grammar< InputIterator, char() > {
	unreserved_grammar() : unreserved_grammar::base_type(unreserved)
	{
		using boost::spirit::qi::alnum;
		using boost::spirit::qi::char_;

		unreserved = alnum | char_("-._~");
	}

	boost::spirit::qi::rule< InputIterator, char() > unreserved;
};

template < typename InputIterator >
struct pct_encoded_grammar : boost::spirit::qi::grammar< InputIterator, char() > {
	pct_encoded_grammar() : pct_encoded_grammar::base_type(pct_encoded)
	{
		using boost::spirit::qi::_val;
		using boost::spirit::qi::_1;

		hex_octet = boost::spirit::qi::uint_parser< std::uint8_t, 16, 2, 2>();
		pct_encoded %= '%' >> hex_octet[ _val = _1 ];
	}
	boost::spirit::qi::rule< InputIterator, char() > pct_encoded,
			hex_octet;
};

struct is_iprivate {
	typedef boost::spirit::context<
			boost::fusion::cons< std::uint32_t&, boost::fusion::nil_>,
			boost::fusion::vector0<>
		> context_type;
	void
	operator()(std::uint32_t v, context_type& ctx, bool& pass) const
	{
		pass = ((0xe000 <= v) && (v <= 0xf8ff))
				|| ((0xf0000 <= v) && (v <= 0xffffd))
				|| ((0x100000 <= v) && (v <= 0x10fffd));
		if (pass) {
			boost::fusion::at_c<0>(ctx.attributes) = v;
		}
	}
};

struct is_ucschar {
	typedef boost::spirit::context<
			boost::fusion::cons< std::uint32_t&, boost::fusion::nil_>,
			boost::fusion::vector0<>
		> context_type;
	void
	operator()(std::uint32_t v, context_type& ctx, bool& pass) const
	{
		pass = (0xa0 <= v && v <= 0xd7ff) ||
			(0xf900 <= v && v <= 0xfdcf) ||
			(0xfdf0 <= v && v <= 0xffef) ||
			(0x10000 <= v && v <= 0x1fffd) ||
			(0x20000 <= v && v <= 0x2fffd) ||
			(0x30000 <= v && v <= 0x3fffd) ||
			(0x40000 <= v && v <= 0x4fffd) ||
			(0x50000 <= v && v <= 0x5fffd) ||
			(0x60000 <= v && v <= 0x6fffd) ||
			(0x70000 <= v && v <= 0x7fffd) ||
			(0x80000 <= v && v <= 0x8fffd) ||
			(0x90000 <= v && v <= 0x9fffd) ||
			(0xa0000 <= v && v <= 0xafffd) ||
			(0xb0000 <= v && v <= 0xbfffd) ||
			(0xc0000 <= v && v <= 0xcfffd) ||
			(0xd0000 <= v && v <= 0xdfffd) ||
			(0xe1000 <= v && v <= 0xefffd);
		if (pass) {
			boost::fusion::at_c<0>(ctx.attributes) = v;
		}
	}
};

template < typename InputIterator, typename ContextAction >
struct hex_range_grammar :
		boost::spirit::qi::grammar< InputIterator, std::uint32_t()> {

	typedef ContextAction context_action_type;

	hex_range_grammar() : hex_range_grammar::base_type(hex_number) {
		hex_number = boost::spirit::qi::uint_parser< std::uint32_t, 16, 2, 8>()[context_action_type()];
	}

	boost::spirit::qi::rule< InputIterator, std::uint32_t()> hex_number;
};

template < typename InputIterator, typename ContextAction >
struct hex_encoded_grammar :
		boost::spirit::qi::grammar< InputIterator, std::uint32_t()> {
	typedef ContextAction context_action_type;

	hex_encoded_grammar() : hex_encoded_grammar::base_type(hex_str) {
		using boost::spirit::qi::string;
		hex_str %= string("%x") >> hex_number;
	}

	boost::spirit::qi::rule< InputIterator, std::uint32_t()> hex_str;
	hex_range_grammar< InputIterator, ContextAction > hex_number;
};

template < typename InputIterator >
using iprivate_grammar = hex_encoded_grammar< InputIterator, is_iprivate >;
template < typename InputIterator >
using ucschar_grammar = hex_encoded_grammar< InputIterator, is_ucschar >;

template < typename InputIterator >
struct iunreserved_grammar :
		boost::spirit::qi::grammar< InputIterator, std::string() > {
	iunreserved_grammar() : iunreserved_grammar::base_type(iunreserved)
	{
		using boost::spirit::qi::alnum;
		using boost::spirit::qi::char_;
		iunreserved %= alnum | char_("-._~") | ucschar;
	}
	boost::spirit::qi::rule< InputIterator, std::string() > iunreserved;
	ucschar_grammar< InputIterator > ucschar;
};

template < typename InputIterator,
	typename SubDelims = sub_delims_grammar< InputIterator > >
struct ipchar_grammar_base :
		boost::spirit::qi::grammar< InputIterator, std::string() > {
	typedef SubDelims sub_delims_type;
	ipchar_grammar_base() : ipchar_grammar_base::base_type(ipchar) {
		using boost::spirit::qi::char_;
		ipchar %= iunreserved |
				pct_encoded |
				sub_delims |
				char_(":@");
	}
	boost::spirit::qi::rule< InputIterator, std::string() > ipchar;
	iunreserved_grammar< InputIterator > iunreserved;
	pct_encoded_grammar< InputIterator > pct_encoded;
	sub_delims_type sub_delims;
};

template < typename InputIterator >
using ipchar_grammar = ipchar_grammar_base< InputIterator, sub_delims_grammar< InputIterator > >;

template < typename InputIterator >
struct iquery_grammar :
		boost::spirit::qi::grammar< InputIterator, tip::iri::query() > {
	typedef tip::iri::query value_type;

	iquery_grammar() : iquery_grammar::base_type(iquery)
	{
		using boost::spirit::qi::char_;
		iquery %= *( ipchar | iprivate | char_("/?") );
	}
	boost::spirit::qi::rule< InputIterator, tip::iri::query() > iquery;
	ipchar_grammar< InputIterator > ipchar;
	iprivate_grammar< InputIterator > iprivate;
};

template < typename InputIterator >
struct ifragment_grammar :
		boost::spirit::qi::grammar< InputIterator, tip::iri::fragment() > {
	ifragment_grammar() : ifragment_grammar::base_type(ifragment)
	{
		using boost::spirit::qi::char_;
		ifragment %= *( ipchar | char_("/?") );
	}
	boost::spirit::qi::rule< InputIterator, tip::iri::fragment() > ifragment;
	ipchar_grammar< InputIterator > ipchar;
};

template < typename InputIterator,
	typename SubDelims = sub_delims_grammar<InputIterator> >
struct isegment_grammar_base :
		boost::spirit::qi::grammar< InputIterator, std::string() > {
	typedef SubDelims sub_delims_type;
	typedef ipchar_grammar_base< InputIterator, sub_delims_type> ipchar_type;

	isegment_grammar_base() : isegment_grammar_base::base_type(isegment) {
		isegment %= *ipchar;
	}

	boost::spirit::qi::rule< InputIterator, std::string() > isegment;
	ipchar_type ipchar;
};
template < typename InputIterator >
using isegment_grammar = isegment_grammar_base< InputIterator, sub_delims_grammar< InputIterator > >;

template < typename InputIterator,
	typename SubDelims = sub_delims_grammar<InputIterator> >
struct isegment_nz_grammar_base :
		boost::spirit::qi::grammar< InputIterator, std::string() > {
	typedef SubDelims sub_delims_type;
	typedef ipchar_grammar_base< InputIterator, sub_delims_type> ipchar_type;

	isegment_nz_grammar_base() : isegment_nz_grammar_base::base_type(isegment_nz) {
		isegment_nz %= +ipchar;
	}

	boost::spirit::qi::rule< InputIterator, std::string() > isegment_nz;
	ipchar_type ipchar;
};
template < typename InputIterator >
using isegment_nz_grammar = isegment_nz_grammar_base< InputIterator, sub_delims_grammar< InputIterator >>;

template < typename InputIterator,
	typename SubDelims = sub_delims_grammar<InputIterator> >
struct isegment_nz_nc_grammar_base :
		boost::spirit::qi::grammar< InputIterator, std::string() > {
	typedef SubDelims sub_delims_type;
	isegment_nz_nc_grammar_base() : isegment_nz_nc_grammar_base::base_type(isegment_nz_nc) {
		using boost::spirit::qi::char_;

		ipchar_nc %= iunreserved |
				pct_encoded |
				sub_delims |
				char_("@");
		isegment_nz_nc %= +ipchar_nc;
	}
	boost::spirit::qi::rule< InputIterator, std::string() > isegment_nz_nc,
			ipchar_nc;
	iunreserved_grammar< InputIterator > iunreserved;
	pct_encoded_grammar< InputIterator > pct_encoded;
	sub_delims_type sub_delims;
};
template < typename InputIterator >
using isegment_nz_nc_grammar = isegment_nz_nc_grammar_base< InputIterator,
		sub_delims_grammar< InputIterator > >;

template < typename InputIterator >
struct scheme_grammar :
		boost::spirit::qi::grammar< InputIterator, tip::iri::scheme() > {

	scheme_grammar() : scheme_grammar::base_type(scheme)
	{
		using namespace boost::spirit::qi;

		scheme = alpha >> *(alnum | char_("+-."));
	}

	boost::spirit::qi::rule< InputIterator, tip::iri::scheme() > scheme;
};

template < typename InputIterator >
struct port_grammar :
		boost::spirit::qi::grammar< InputIterator, tip::iri::port() > {

	port_grammar() : port_grammar::base_type(port)
	{
		using boost::spirit::qi::digit;
		port = *digit;
	}

	boost::spirit::qi::rule< InputIterator, tip::iri::port() > port;
};

template < typename InputIterator, typename AttributeType >
struct ipv4_grammar;

template < typename InputIterator >
struct ipv4_grammar< InputIterator, std::uint32_t() > :
		boost::spirit::qi::grammar<InputIterator, std::uint32_t()> {

	ipv4_grammar() : ipv4_grammar::base_type(ipv4address)
	{
		using boost::spirit::qi::eps;
		using boost::spirit::qi::_1;
		using boost::spirit::qi::_val;

		dec_octet = boost::spirit::qi::uint_parser< std::uint8_t, 10, 1, 3 >();

		ipv4address =
				eps[ _val = 0 ]
				>> dec_octet[ _val  = (_1 << 24) ] >> '.'
				>> dec_octet[ _val |= (_1 << 16) ] >> '.'
				>> dec_octet[ _val |= (_1 << 8)  ] >> '.'
				>> dec_octet[ _val |= _1 ];
	}

	boost::spirit::qi::rule< InputIterator, std::uint32_t()> ipv4address,
			dec_octet;
};

template < typename InputIterator >
struct ipv4_grammar< InputIterator, std::string() > :
		boost::spirit::qi::grammar< InputIterator, std::string() > {

	typedef std::string value_type;
	typedef boost::spirit::context<
			boost::fusion::cons< value_type&, boost::fusion::nil_ >,
			boost::fusion::vector0<>
		> context_type;

	struct print_octet {
		void
		operator()(std::uint8_t v, context_type& ctx, bool& pass) const
		{
			boost::fusion::at_c<0>(ctx.attributes) +=
					boost::lexical_cast< std::string >((unsigned int)v);
		}
	};
	ipv4_grammar() : ipv4_grammar::base_type(ipv4address)
	{
		using boost::spirit::qi::eps;
		using boost::spirit::qi::_1;
		using boost::spirit::qi::_val;
		using boost::spirit::qi::char_;

		dec_octet = boost::spirit::qi::uint_parser< std::uint8_t, 10, 1, 3 >()[ print_octet() ];

		ipv4address %=
				(  dec_octet >> char_('.')
				>> dec_octet >> char_('.')
				>> dec_octet >> char_('.')
				>> dec_octet);

		BOOST_SPIRIT_DEBUG_NODES((ipv4address));
	}
	boost::spirit::qi::rule< InputIterator, std::string() > ipv4address, dec_octet;

};

template < typename InputIterator, typename AttributeType >
struct ipv6_grammar;

template < typename InputIterator >
struct ipv6_grammar< InputIterator, std::array< std::uint16_t, 8 >() > :
		boost::spirit::qi::grammar<InputIterator,
				std::array< std::uint16_t, 8 >(),
				boost::spirit::qi::locals<
					std::size_t
				> > {

	typedef std::array< std::uint16_t, 8 > value_type;
	typedef boost::spirit::qi::locals<std::size_t> locals_type;
	typedef std::pair< std::uint16_t, std::uint16_t > ls32_value_type;
	typedef boost::spirit::context<
			boost::fusion::cons< value_type&, boost::fusion::nil_ >,
			boost::fusion::vector1<std::size_t>
		> context_type;

	struct assign_segment {
		void
		operator()(std::uint16_t s, context_type& ctx, bool& pass) const
		{
			boost::fusion::at_c<0>(ctx.attributes)[ boost::fusion::at_c<0>(ctx.locals)++ ] = s;
		}
		void
		operator()(ls32_value_type const& ls32, context_type& ctx, bool& pass) const
		{
			boost::fusion::at_c<0>(ctx.attributes)[6] = ls32.first;
			boost::fusion::at_c<0>(ctx.attributes)[7] = ls32.second;
		}
	};
	struct start_action {
		void
		operator()(boost::spirit::unused_type, context_type& ctx, bool& pass) const
		{
			boost::fusion::at_c<0>(ctx.attributes) 	= value_type();
			boost::fusion::at_c<0>(ctx.locals)		= 0;
		}
	};

	ipv6_grammar() : ipv6_grammar::base_type(ipv6address)
	{
		using boost::spirit::qi::eps;
		using boost::spirit::qi::lit;
		using boost::spirit::qi::repeat;
		using boost::spirit::qi::_1;
		using boost::spirit::qi::_a;
		using boost::spirit::qi::_val;
		using boost::phoenix::at_c;

		ls32 = (h16[at_c<0>(_val) = _1] >> ':' >> h16[at_c<1>(_val) = _1]) |
				ipv4address [ at_c<0>(_val) = (_1 >> 16),
							  at_c<1>(_val) = (0x0000ffff) & _1 ];

		assign_segment seg;
		start_action start;
		ipv6address =
			  (eps[start]																		>> repeat(6)[h16[seg] >> ':'] >> ls32[seg])
			| (eps[start]													>> lit("::")[_a=1]	>> repeat(5)[h16[seg] >> ':'] >> ls32[seg])
			| (eps[start]	>> 									-(h16[seg])	>> lit("::")[_a=2]	>> repeat(4)[h16[seg] >> ':'] >> ls32[seg])
			| (eps[start]	>> 									-(h16[seg])	>> lit("::")[_a=3]	>> repeat(3)[h16[seg] >> ':'] >> ls32[seg])
			| (eps[start]	>> 			 h16[seg] >> ':'	>>	  h16[seg]	>> lit("::")[_a=3]	>> repeat(3)[h16[seg] >> ':'] >> ls32[seg])

			| (eps[start]	>> 									-(h16[seg])	>> lit("::")[_a=4]	>> repeat(2)[h16[seg] >> ':'] >> ls32[seg])
			| (eps[start]	>> 			 h16[seg] >> ':'	>>	  h16[seg]	>> lit("::")[_a=4]	>> repeat(2)[h16[seg] >> ':'] >> ls32[seg])
			| (eps[start]	>> repeat(2)[h16[seg] >> ':']	>>	  h16[seg]	>> lit("::")[_a=4]	>> repeat(2)[h16[seg] >> ':'] >> ls32[seg])

			| (eps[start]	>> 									-(h16[seg])	>> lit("::")[_a=5]	>> 			 h16[seg] >> ':'  >> ls32[seg])
			| (eps[start]	>> 			 h16[seg] >> ':'	>>	  h16[seg]	>> lit("::")[_a=5]	>> 			 h16[seg] >> ':'  >> ls32[seg])
			| (eps[start]	>> repeat(2)[h16[seg] >> ':']	>>	  h16[seg]	>> lit("::")[_a=5]	>> 			 h16[seg] >> ':'  >> ls32[seg])
			| (eps[start]	>> repeat(3)[h16[seg] >> ':']	>>	  h16[seg]	>> lit("::")[_a=5]	>> 			 h16[seg] >> ':'  >> ls32[seg])

			| (eps[start]	>> 									-(h16[seg])	>> lit("::")[_a=6]	>> 							     ls32[seg])
			| (eps[start]	>> 			 h16[seg] >> ':'	>>	  h16[seg]	>> lit("::")[_a=6]	>> 							     ls32[seg])
			| (eps[start]	>> repeat(2)[h16[seg] >> ':']	>>	  h16[seg]	>> lit("::")[_a=6]	>> 							     ls32[seg])
			| (eps[start]	>> repeat(3)[h16[seg] >> ':']	>>	  h16[seg]	>> lit("::")[_a=6]	>> 							     ls32[seg])
			| (eps[start]	>> repeat(4)[h16[seg] >> ':']	>>	  h16[seg]	>> lit("::")[_a=6]	>> 							     ls32[seg])

			| (eps[start]	>> 									-(h16[seg])	>> lit("::")[_a=7]	>> 			 h16[seg])
			| (eps[start]	>> 			 h16[seg] >> ':'	>>	  h16[seg]	>> lit("::")[_a=7]	>> 			 h16[seg])
			| (eps[start]	>> repeat(2)[h16[seg] >> ':']	>>	  h16[seg]	>> lit("::")[_a=7]	>> 			 h16[seg])
			| (eps[start]	>> repeat(3)[h16[seg] >> ':']	>>	  h16[seg]	>> lit("::")[_a=7]	>> 			 h16[seg])
			| (eps[start]	>> repeat(4)[h16[seg] >> ':']	>>	  h16[seg]	>> lit("::")[_a=7]	>> 			 h16[seg])
			| (eps[start]	>> repeat(5)[h16[seg] >> ':']	>>	  h16[seg]	>> lit("::")[_a=7]	>> 			 h16[seg])

			| (eps[start]	>> 									-(h16[seg])	>> 	   "::")
			| (eps[start]	>> 			 h16[seg] >> ':' 	>>	  h16[seg]	>> 	   "::")
			| (eps[start]	>> repeat(2)[h16[seg] >> ':']	>>	  h16[seg]	>> 	   "::")
			| (eps[start]	>> repeat(3)[h16[seg] >> ':']	>>	  h16[seg]	>> 	   "::")
			| (eps[start]	>> repeat(4)[h16[seg] >> ':']	>>	  h16[seg]	>> 	   "::")
			| (eps[start]	>> repeat(5)[h16[seg] >> ':']	>>	  h16[seg]	>> 	   "::")
			| (eps[start]	>> repeat(6)[h16[seg] >> ':']	>>	  h16[seg]	>> 	   "::")
		;
	}

	boost::spirit::qi::rule< InputIterator, value_type(), locals_type > ipv6address;
	boost::spirit::qi::rule< InputIterator, ls32_value_type() > ls32;
	boost::spirit::qi::uint_parser< std::uint16_t, 16, 1, 4 > h16;
	ipv4_grammar< InputIterator, std::uint32_t() > ipv4address;
};

template < typename InputIterator >
struct ipv6_grammar< InputIterator, std::string() > :
		boost::spirit::qi::grammar< InputIterator, std::string(),
			boost::spirit::qi::locals<std::string> > {
	typedef std::string value_type;
	typedef boost::spirit::qi::locals<std::string> locals_type;
	typedef boost::spirit::context<
			boost::fusion::cons< value_type&, boost::fusion::nil_ >,
			boost::fusion::vector1<std::string>
		> context_type;

	struct start_action {
		void
		operator()(boost::spirit::unused_type, context_type& ctx, bool& pass) const
		{
			boost::fusion::at_c<0>(ctx.attributes) 	= value_type();
			boost::fusion::at_c<0>(ctx.locals) = value_type();
		}
	};

	ipv6_grammar() : ipv6_grammar::base_type(ipv6address)
	{
		using boost::spirit::qi::eps;
		using boost::spirit::qi::repeat;
		using boost::spirit::qi::xdigit;
		using boost::spirit::qi::char_;
		using boost::spirit::qi::string;
		using boost::spirit::qi::_1;
		using boost::spirit::qi::_a;
		using boost::spirit::qi::_val;

		h16 = repeat(1,4)[xdigit];
		ls32 = (ipv4address[ _a = _1 ] | (h16[_a = _1] >> ':' >>  h16[_a += ':' + _1]))[_val += _a];

		start_action start;
		ipv6address %=
				  (eps[start]																	>> repeat(6)[h16 >> char_(':')] >> ls32)
				| (eps[start]												>> string("::")		>> repeat(5)[h16 >> char_(':')] >> ls32)
				| (eps[start]	>> 									-(h16)	>> string("::")		>> repeat(4)[h16 >> char_(':')] >> ls32)
				| (eps[start]	>> 									-(h16)	>> string("::")		>> repeat(3)[h16 >> char_(':')] >> ls32)
				| (eps[start]	>> 			 h16 >> char_(':')	>>	  h16	>> string("::")		>> repeat(3)[h16 >> char_(':')] >> ls32)

				| (eps[start]	>> 									-(h16)	>> string("::")		>> repeat(2)[h16 >> char_(':')] >> ls32)
				| (eps[start]	>> 			 h16 >> char_(':')	>>	  h16	>> string("::")		>> repeat(2)[h16 >> char_(':')] >> ls32)
				| (eps[start]	>> repeat(2)[h16 >> char_(':')]	>>	  h16	>> string("::")		>> repeat(2)[h16 >> char_(':')] >> ls32)

				| (eps[start]	>> 									-(h16)	>> string("::")		>> 			 h16 >> char_(':')  >> ls32)
				| (eps[start]	>> 			 h16 >> char_(':')	>>	  h16	>> string("::")		>> 			 h16 >> char_(':')  >> ls32)
				| (eps[start]	>> repeat(2)[h16 >> char_(':')]	>>	  h16	>> string("::")		>> 			 h16 >> char_(':')  >> ls32)
				| (eps[start]	>> repeat(3)[h16 >> char_(':')]	>>	  h16	>> string("::")		>> 			 h16 >> char_(':')  >> ls32)

				| (eps[start]	>> 									-(h16)	>> string("::")		>> 								   ls32)
				| (eps[start]	>> 			 h16 >> char_(':')	>>	  h16	>> string("::")		>> 								   ls32)
				| (eps[start]	>> repeat(2)[h16 >> char_(':')]	>>	  h16	>> string("::")		>> 								   ls32)
				| (eps[start]	>> repeat(3)[h16 >> char_(':')]	>>	  h16	>> string("::")		>>		 						   ls32)
				| (eps[start]	>> repeat(4)[h16 >> char_(':')]	>>	  h16	>> string("::")		>> 								   ls32)

				| (eps[start]	>> 									-(h16)	>> string("::")		>> 			 h16)
				| (eps[start]	>> 			 h16 >> char_(':')	>>	  h16	>> string("::")		>> 			 h16)
				| (eps[start]	>> repeat(2)[h16 >> char_(':')]	>>	  h16	>> string("::")		>> 			 h16)
				| (eps[start]	>> repeat(3)[h16 >> char_(':')]	>>	  h16	>> string("::")		>> 			 h16)
				| (eps[start]	>> repeat(4)[h16 >> char_(':')]	>>	  h16	>> string("::")		>> 			 h16)
				| (eps[start]	>> repeat(5)[h16 >> char_(':')]	>>	  h16	>> string("::")		>> 			 h16)

				| (eps[start]	>> 									-(h16)	>> string("::"))
				| (eps[start]	>> 			 h16 >> char_(':') 	>>	  h16	>> string("::"))
				| (eps[start]	>> repeat(2)[h16 >> char_(':')]	>>	  h16	>> string("::"))
				| (eps[start]	>> repeat(3)[h16 >> char_(':')]	>>	  h16	>> string("::"))
				| (eps[start]	>> repeat(4)[h16 >> char_(':')]	>>	  h16	>> string("::"))
				| (eps[start]	>> repeat(5)[h16 >> char_(':')]	>>	  h16	>> string("::"))
				| (eps[start]	>> repeat(6)[h16 >> char_(':')]	>>	  h16	>> string("::"))
			;

	}

	boost::spirit::qi::rule< InputIterator, std::string(), locals_type > ipv6address, ls32;
	boost::spirit::qi::rule< InputIterator, std::string() > h16;
	ipv4_grammar< InputIterator, std::string() > ipv4address;
};

template < typename InputIterator >
struct ipvfuture_grammar :
		boost::spirit::qi::grammar< InputIterator, std::string() > {
	ipvfuture_grammar() : ipvfuture_grammar::base_type(ipvfuture)
	{
		using boost::spirit::qi::xdigit;
		using boost::spirit::qi::char_;
		ipvfuture %= char_('v') >> +xdigit >> char_('.') >>
				+(unreserved | sub_delims);
	}
	boost::spirit::qi::rule< InputIterator, std::string() > ipvfuture;
	unreserved_grammar< InputIterator > unreserved;
	sub_delims_grammar< InputIterator > sub_delims;
};

template < typename InputIterator >
struct ip_literal_grammar :
		boost::spirit::qi::grammar< InputIterator, std::string() > {
	ip_literal_grammar() : ip_literal_grammar::base_type(ip_literal)
	{
		using boost::spirit::qi::char_;
		using boost::spirit::qi::_val;
		using boost::spirit::qi::_1;

		ip_literal = char_('[')[ _val += _1 ] >>
				(ipv6_address[ _val += _1 ] | ipvfuture[ _val += _1 ]) >>
				char_(']')[ _val += _1 ];
	}
	boost::spirit::qi::rule< InputIterator, std::string() > ip_literal;
	ipv6_grammar< InputIterator, std::string() > ipv6_address;
	ipvfuture_grammar< InputIterator > ipvfuture;
};

struct path_semantic_actions {
	typedef tip::iri::path value_type;
	typedef boost::spirit::context<
			boost::fusion::cons< value_type&, boost::fusion::nil_ >,
			boost::fusion::vector0<>
	> context_type;

	// Start parsing
	void
	operator()(boost::spirit::unused_type, context_type& ctx, bool& pass) const
	{
		// Clear and assign rooted
		value_type(rooted_).swap(boost::fusion::at_c<0>(ctx.attributes));
	}
	void
	operator()(std::string const& segment, context_type& ctx, bool& pass) const
	{
		//if (!segment.empty())
			boost::fusion::at_c<0>(ctx.attributes).push_back(segment);
	}
	path_semantic_actions(bool rooted) : rooted_(rooted) {}

	bool rooted_;
};

template < typename InputIterator,
	typename SubDelims = sub_delims_grammar<InputIterator> >
struct ipath_abempty_grammar_base :
		boost::spirit::qi::grammar< InputIterator, tip::iri::path() > {
	typedef SubDelims sub_delims_type;
	typedef isegment_grammar_base< InputIterator, sub_delims_type > isegment_type;
	typedef isegment_nz_grammar_base< InputIterator, sub_delims_type > isegment_nz_type;

	ipath_abempty_grammar_base() : ipath_abempty_grammar_base::base_type(ipath_abempty) {
		using boost::spirit::qi::lit;
		using boost::spirit::qi::eps;
		path_semantic_actions pa(true);
		ipath_abempty = eps[pa] >> -(lit("/") >> -isegment_nz[pa])
				>> *(lit("/") >> isegment[pa]);
	}
	boost::spirit::qi::rule< InputIterator, tip::iri::path() > ipath_abempty;
	isegment_type isegment;
	isegment_nz_type isegment_nz;
};
template < typename InputIterator >
using ipath_abempty_grammar = ipath_abempty_grammar_base< InputIterator,
		sub_delims_grammar< InputIterator > >;

template < typename InputIterator,
	typename SubDelims = sub_delims_grammar<InputIterator> >
struct ipath_absolute_grammar_base :
		boost::spirit::qi::grammar< InputIterator, tip::iri::path() > {
	typedef SubDelims sub_delims_type;
	typedef isegment_grammar_base< InputIterator, sub_delims_type > isegment_type;
	typedef isegment_nz_grammar_base< InputIterator, sub_delims_type > isegment_nz_type;
	ipath_absolute_grammar_base() : ipath_absolute_grammar_base::base_type(ipath_absolute) {
		using boost::spirit::qi::lit;
		using boost::spirit::qi::eps;
		path_semantic_actions pa(true);
		ipath_absolute = eps[pa] >> lit("/")
				>> -(isegment_nz[pa] >> *(lit("/") >> isegment[pa]));
	}
	boost::spirit::qi::rule< InputIterator, tip::iri::path() > ipath_absolute;
	isegment_type isegment;
	isegment_nz_type isegment_nz;
};
template < typename InputIterator >
using ipath_absolute_grammar = ipath_absolute_grammar_base< InputIterator, sub_delims_grammar< InputIterator >>;

template < typename InputIterator >
struct ipath_noscheme_grammar :
		boost::spirit::qi::grammar< InputIterator, tip::iri::path() > {
	ipath_noscheme_grammar() : ipath_noscheme_grammar::base_type(ipath_noscheme) {
		using boost::spirit::qi::lit;
		using boost::spirit::qi::eps;
		path_semantic_actions pa(false);
		ipath_noscheme = eps[pa] >> isegment_nz_nc[pa] >> *(lit("/") >> isegment[pa]);
	}
	boost::spirit::qi::rule< InputIterator, tip::iri::path() > ipath_noscheme;
	isegment_grammar< InputIterator > isegment;
	isegment_nz_nc_grammar< InputIterator > isegment_nz_nc;
};

template < typename InputIterator >
struct ipath_rootless_grammar :
		boost::spirit::qi::grammar< InputIterator, tip::iri::path() > {
	ipath_rootless_grammar() : ipath_rootless_grammar::base_type(ipath_rootless) {
		using boost::spirit::qi::lit;
		using boost::spirit::qi::eps;
		path_semantic_actions pa(false);
		ipath_rootless = isegment_nz[pa] >> *(lit("/") >> isegment[pa]);
	}
	boost::spirit::qi::rule< InputIterator, tip::iri::path() > ipath_rootless;
	isegment_grammar< InputIterator > isegment;
	isegment_nz_grammar< InputIterator > isegment_nz;
};

template < typename InputIterator >
struct ipath_grammar :
		boost::spirit::qi::grammar< InputIterator, tip::iri::path() > {
	ipath_grammar() : ipath_grammar::base_type(ipath) {
		ipath %= ipath_rootless |
				ipath_noscheme |
				ipath_abempty |
				ipath_absolute
				;
	}
	boost::spirit::qi::rule< InputIterator, tip::iri::path() > ipath;
	ipath_abempty_grammar< InputIterator > ipath_abempty;
	ipath_absolute_grammar< InputIterator > ipath_absolute;
	ipath_noscheme_grammar< InputIterator > ipath_noscheme;
	ipath_rootless_grammar< InputIterator > ipath_rootless;
};

template < typename InputIterator,
	typename SubDelims = sub_delims_grammar<InputIterator> >
struct ireg_name_grammar_base :
		boost::spirit::qi::grammar< InputIterator, std::string() > {
	typedef SubDelims sub_delims_type;

	ireg_name_grammar_base() : ireg_name_grammar_base::base_type(ireg_name)
	{
		using boost::spirit::qi::char_;
		ireg_name %= *(
			iunreserved
			| pct_encoded
			| sub_delims
			//| char_(':')
		);
	}
	boost::spirit::qi::rule< InputIterator, std::string() > ireg_name;
	iunreserved_grammar< InputIterator > iunreserved;
	pct_encoded_grammar< InputIterator > pct_encoded;
	sub_delims_type sub_delims;
};
template < typename InputIterator >
using ireg_name_grammar = ireg_name_grammar_base< InputIterator, sub_delims_grammar< InputIterator > >;


template < typename InputIterator,
	typename SubDelims = sub_delims_grammar<InputIterator> >
struct ihost_grammar_base :
		boost::spirit::qi::grammar< InputIterator, tip::iri::host() > {
	typedef SubDelims sub_delims_type;
	typedef ireg_name_grammar_base< InputIterator, sub_delims_type > ireg_name_type;

	ihost_grammar_base() : ihost_grammar_base::base_type(ihost)
	{
		ihost %= ip_literal |
				ipv4_address |
				ireg_name;
	}

	boost::spirit::qi::rule< InputIterator, tip::iri::host() > ihost;
	ip_literal_grammar< InputIterator > ip_literal;
	ipv4_grammar< InputIterator, std::string() > ipv4_address;
	ireg_name_type ireg_name;
};
template < typename InputIterator >
using ihost_grammar = ihost_grammar_base< InputIterator, sub_delims_grammar< InputIterator > >;

template < typename InputIterator >
struct iuser_info_grammar :
		boost::spirit::qi::grammar< InputIterator, tip::iri::userinfo() > {

	iuser_info_grammar() : iuser_info_grammar::base_type(iuser_info)
	{
		using boost::spirit::qi::char_;
		iuser_info %= *(iunreserved |
				pct_encoded |
				sub_delims |
				char_(':'));
	}
	boost::spirit::qi::rule< InputIterator, tip::iri::userinfo() > iuser_info;
	iunreserved_grammar< InputIterator > iunreserved;
	pct_encoded_grammar< InputIterator > pct_encoded;
	sub_delims_grammar< InputIterator > sub_delims;
};

template < typename InputIterator >
struct iauthority_grammar :
		boost::spirit::qi::grammar< InputIterator, tip::iri::authority() > {

	typedef tip::iri::authority value_type;
	typedef boost::spirit::context<
			boost::fusion::cons< value_type&, boost::fusion::nil_ >,
			boost::fusion::vector0<>
	> context_type;

	struct components {
		void
		operator()(tip::iri::userinfo const& u, context_type& ctx, bool& pass) const
		{
			boost::fusion::at_c<0>(ctx.attributes).userinfo = u;
		}
		void
		operator()(tip::iri::host const& h, context_type& ctx, bool& pass) const
		{
			boost::fusion::at_c<0>(ctx.attributes).host = h;
		}
		void
		operator()(tip::iri::port const& p, context_type& ctx, bool& pass) const
		{
			boost::fusion::at_c<0>(ctx.attributes).port = p;
		}
	};
	struct start_action {
		void
		operator()(boost::spirit::unused_type, context_type& ctx, bool& pass) const
		{
			boost::fusion::at_c<0>(ctx.attributes) 	= value_type();
		}
	};

	iauthority_grammar() : iauthority_grammar::base_type(iauthority)
	{
		using boost::spirit::qi::eps;
		using boost::spirit::qi::char_;
		using boost::spirit::qi::_val;

		components cmp;
		start_action start;
		iauthority =
			  eps[start] >> iuser_info[cmp] >> char_('@') >> ihost[cmp] >> -(char_(':') >> port[cmp])
			| eps[start] >> ihost[ cmp ] >> -(char_(':') >> port[ cmp ]);
	}

	boost::spirit::qi::rule< InputIterator, tip::iri::authority() > iauthority;
	iuser_info_grammar< InputIterator > iuser_info;
	ihost_grammar< InputIterator > ihost;
	port_grammar< InputIterator > port;
};

template < typename InputIterator,
		typename QueryParser = iquery_grammar< InputIterator > >
struct irelative_part_grammar :
		boost::spirit::qi::grammar< InputIterator,
			tip::iri::basic_iri< typename QueryParser::value_type >() > {
	typedef QueryParser query_parser_type;
	typedef typename query_parser_type::value_type query_type;
	typedef tip::iri::basic_iri< query_type > value_type;
	typedef boost::spirit::context<
			boost::fusion::cons< value_type&, boost::fusion::nil_ >,
			boost::fusion::vector0<>
	> context_type;
	struct components {
		void
		operator()(tip::iri::authority const& a, context_type& ctx, bool& pass) const
		{
			boost::fusion::at_c<0>(ctx.attributes).authority = a;
		}
		void
		operator()(tip::iri::path const& p, context_type& ctx, bool& path) const
		{
			boost::fusion::at_c<0>(ctx.attributes).path = p;
		}
	};

	irelative_part_grammar() : irelative_part_grammar::base_type(irelative_part)
	{
		using boost::spirit::qi::string;
		components cmp;
		irelative_part =
				  (string("//") >> iauthority[cmp] >> ipath_abempty[cmp])
				| ipath_absolute[cmp]
				| ipath_noscheme[cmp];
	}

	boost::spirit::qi::rule< InputIterator, value_type() > irelative_part;
	iauthority_grammar< InputIterator >		iauthority;
	ipath_abempty_grammar< InputIterator >	ipath_abempty;
	ipath_absolute_grammar< InputIterator >	ipath_absolute;
	ipath_noscheme_grammar< InputIterator >	ipath_noscheme;
};

template < typename InputIterator,
		typename QueryParser = iquery_grammar< InputIterator > >
struct irelative_ref_grammar :
		boost::spirit::qi::grammar< InputIterator,
				tip::iri::basic_iri< typename QueryParser::value_type >() > {
	typedef QueryParser query_parser_type;
	typedef typename query_parser_type::value_type query_type;
	typedef tip::iri::basic_iri< query_type > value_type;
	typedef boost::spirit::context<
			boost::fusion::cons< value_type&, boost::fusion::nil_ >,
			boost::fusion::vector0<>
	> context_type;
	struct components {
		void
		operator()(query_type const& q, context_type& ctx, bool& path) const
		{
			boost::fusion::at_c<0>(ctx.attributes).query = q;
		}
		void
		operator()(tip::iri::fragment const& f, context_type& ctx, bool& path) const
		{
			boost::fusion::at_c<0>(ctx.attributes).fragment = f;
		}
	};
	irelative_ref_grammar() : irelative_ref_grammar::base_type(irelative_ref)
	{
		using boost::spirit::qi::char_;
		irelative_ref %= irelative_part
				>> -(char_('?') >> iquery)
				>> -(char_('#') >> ifragment);
	}

	boost::spirit::qi::rule< InputIterator, value_type() > irelative_ref;
	irelative_part_grammar< InputIterator > irelative_part;
	query_parser_type iquery;
	ifragment_grammar< InputIterator > ifragment;
};

template < typename InputIterator,
		typename QueryParser = iquery_grammar< InputIterator > >
struct ihier_part_grammar :
		boost::spirit::qi::grammar< InputIterator,
			tip::iri::basic_iri< typename QueryParser::value_type >() > {
	typedef QueryParser query_parser_type;
	typedef typename query_parser_type::value_type query_type;
	typedef tip::iri::basic_iri< query_type > value_type;
	typedef boost::spirit::context<
			boost::fusion::cons< value_type&, boost::fusion::nil_ >,
			boost::fusion::vector0<>
	> context_type;
	struct components {
		void
		operator()(tip::iri::authority const& a, context_type& ctx, bool& pass) const
		{
			boost::fusion::at_c<0>(ctx.attributes).authority = a;
		}
		void
		operator()(tip::iri::path const& p, context_type& ctx, bool& path) const
		{
			boost::fusion::at_c<0>(ctx.attributes).path = p;
		}
	};
	ihier_part_grammar() : ihier_part_grammar::base_type(ihier_part)
	{
		using boost::spirit::qi::string;
		components cmp;
		ihier_part =
				  (string("//") >> iauthority[cmp] >> ipath_abempty[cmp])
				| ipath_absolute[cmp]
				| ipath_rootless[cmp];
	}

	boost::spirit::qi::rule< InputIterator, value_type() >	ihier_part;
	iauthority_grammar< InputIterator >						iauthority;
	ipath_abempty_grammar< InputIterator >					ipath_abempty;
	ipath_absolute_grammar< InputIterator >					ipath_absolute;
	ipath_rootless_grammar< InputIterator >					ipath_rootless;
};

template < typename InputIterator,
		typename QueryParser = iquery_grammar< InputIterator > >
struct absolute_iri_grammar :
		boost::spirit::qi::grammar< InputIterator,
				tip::iri::basic_iri< typename QueryParser::value_type >() > {
	typedef QueryParser query_parser_type;
	typedef typename query_parser_type::value_type query_type;
	typedef tip::iri::basic_iri< query_type > value_type;
	typedef boost::spirit::context<
			boost::fusion::cons< value_type&, boost::fusion::nil_ >,
			boost::fusion::vector0<>
	> context_type;
	struct components {
		void
		operator()(tip::iri::scheme const& s, context_type& ctx, bool& pass) const
		{
			boost::fusion::at_c<0>(ctx.attributes).scheme = s;
		}
		void
		operator()(query_type const& q, context_type& ctx, bool& path) const
		{
			boost::fusion::at_c<0>(ctx.attributes).query = q;
		}
		void
		operator()(value_type const& i, context_type& ctx, bool& pass) const
		{
			value_type& val = boost::fusion::at_c<0>(ctx.attributes);
			val.authority = i.authority;
			val.path = i.path;
		}
	};

	absolute_iri_grammar() : absolute_iri_grammar::base_type(absolute_iri)
	{
		using boost::spirit::qi::char_;
		components cmp;
		absolute_iri = scheme[cmp] >> char_(':')
				>> ihier_part[cmp]
				>> -( char_('?') >> iquery[cmp] );
	}

	boost::spirit::qi::rule< InputIterator, value_type() >	absolute_iri;
	scheme_grammar< InputIterator >							scheme;
	ihier_part_grammar< InputIterator >						ihier_part;
	query_parser_type										iquery;
};

template < typename InputIterator,
		typename QueryParser = iquery_grammar< InputIterator > >
struct iri_grammar :
		boost::spirit::qi::grammar< InputIterator,
			tip::iri::basic_iri< typename QueryParser::value_type >() > {
	typedef QueryParser query_parser_type;
	typedef typename query_parser_type::value_type query_type;
	typedef tip::iri::basic_iri< query_type > value_type;
	typedef boost::spirit::context<
			boost::fusion::cons< value_type&, boost::fusion::nil_ >,
			boost::fusion::vector0<>
	> context_type;
	struct components {
		void
		operator()(tip::iri::scheme const& s, context_type& ctx, bool& pass) const
		{
			boost::fusion::at_c<0>(ctx.attributes).scheme = s;
		}
		void
		operator()(query_type const& q, context_type& ctx, bool& pass) const
		{
			boost::fusion::at_c<0>(ctx.attributes).query = q;
		}
		void
		operator()(tip::iri::fragment const& f, context_type& ctx, bool& pass) const
		{
			boost::fusion::at_c<0>(ctx.attributes).fragment = f;
		}
		void
		operator()(value_type const& i, context_type& ctx, bool& pass) const
		{
			value_type& val = boost::fusion::at_c<0>(ctx.attributes);
			val.authority = i.authority;
			val.path = i.path;
		}
	};
	iri_grammar() : iri_grammar::base_type(iri)
	{
		using boost::spirit::qi::char_;
		components cmp;
		iri = scheme[cmp] >> char_(':')
				>> ihier_part[cmp]
				>> -( char_('?') >> iquery[cmp] )
				>> -( char_('#') >> ifragment[cmp] )
		;
	}

	boost::spirit::qi::rule< InputIterator, value_type() >	iri;
	scheme_grammar< InputIterator >							scheme;
	ihier_part_grammar< InputIterator, query_parser_type >	ihier_part;
	query_parser_type										iquery;
	ifragment_grammar< InputIterator >						ifragment;
};

template < typename InputIterator,
		typename QueryParser = iquery_grammar< InputIterator > >
struct iri_reference_grammar :
		boost::spirit::qi::grammar< InputIterator,
				tip::iri::basic_iri< typename QueryParser::value_type >() > {
	typedef QueryParser query_parser_type;
	typedef typename query_parser_type::value_type query_type;
	typedef tip::iri::basic_iri< query_type > value_type;
	iri_reference_grammar() : iri_reference_grammar::base_type(iri_reference)
	{
		using boost::spirit::qi::char_;
		iri_reference = iri | irelative_ref;
	}

	boost::spirit::qi::rule< InputIterator, value_type() >		iri_reference;
	iri_grammar< InputIterator, query_parser_type >				iri;
	irelative_ref_grammar< InputIterator, query_parser_type >	irelative_ref;
};

}  // namespace parse
}  // namespace grammar
}  // namespace iri
}  // namespace tip



#endif /* TIP_IRI_GRAMMAR_IRI_PARSE_HPP_ */
