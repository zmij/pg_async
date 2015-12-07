/*
 * iri_generate.hpp
 *
 *  Created on: Aug 19, 2015
 *      Author: zmij
 */

#ifndef TIP_IRI_GRAMMAR_IRI_GENERATE_HPP_
#define TIP_IRI_GRAMMAR_IRI_GENERATE_HPP_

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/support_adapt_adt_attributes.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <tip/iri.hpp>

namespace tip {
namespace iri {
namespace grammar {
namespace gen {

template < typename OutputIterator >
struct sub_delims_grammar :
		boost::spirit::karma::grammar< OutputIterator, char()> {
	sub_delims_grammar() : sub_delims_grammar::base_type(sub_delims)
	{
		namespace karma = boost::spirit::karma;
		sub_delims = karma::char_("!$&'()*+,;=");
	}
	boost::spirit::karma::rule< OutputIterator, char()> sub_delims;
};

template < typename OutputIterator >
struct gen_delims_grammar :
		boost::spirit::karma::grammar< OutputIterator, char()> {
	gen_delims_grammar() : gen_delims_grammar::base_type(gen_delims)
	{
		namespace karma = boost::spirit::karma;
		gen_delims = karma::char_(":/?#[]@");
	}
	boost::spirit::karma::rule< OutputIterator, char()> gen_delims;
};

template < typename OutputIterator >
struct reserved_grammar :
		boost::spirit::karma::grammar< OutputIterator, char()> {
	reserved_grammar() : reserved_grammar::base_type(reserved)
	{
		namespace karma = boost::spirit::karma;
		reserved = gen_delims | sub_delims;
	}
	boost::spirit::karma::rule< OutputIterator, char()> reserved;
	gen_delims_grammar< OutputIterator > gen_delims;
	sub_delims_grammar< OutputIterator > sub_delims;
};

template < typename OutputIterator >
struct unreserved_grammar :
		boost::spirit::karma::grammar< OutputIterator, char()> {
	unreserved_grammar() : unreserved_grammar::base_type(unreserved)
	{
		namespace karma = boost::spirit::karma;
		unreserved = karma::alnum | karma::char_("-._~");
	}
	boost::spirit::karma::rule< OutputIterator, char()> unreserved;
};

template < typename OutputIterator >
struct pct_encoded_grammar :
		boost::spirit::karma::grammar< OutputIterator, char()> {
	pct_encoded_grammar() : pct_encoded_grammar::base_type(pct_encoded)
	{
		namespace karma = boost::spirit::karma;
		pct_encoded = '%' << karma::right_align(2, 0)[karma::hex];
	}
	boost::spirit::karma::rule< OutputIterator, char()> pct_encoded;
};

struct is_hex_escaped {
	typedef std::string value_type;
	typedef value_type::const_iterator value_iterator;
	typedef boost::spirit::context<
		boost::fusion::cons< value_type const&, boost::fusion::nil_ >,
		boost::fusion::vector1< value_iterator >
	> context_type;

	void
	operator()(std::string& v, context_type& ctx, bool& pass) const
	{
		value_iterator e = boost::fusion::at_c<0>(ctx.attributes).end();
		value_iterator p = boost::fusion::at_c<0>(ctx.locals);
		if (p != e && *p++ == '%' && p != e && *p++ == 'x') {
			int digits = 0;
			std::uint32_t char_code = 0;
			while (p != e && std::isxdigit((int)*p)) {
				char_code = (char_code << 8) | *p;
				v.push_back(*p++);
				++digits;
			}
			// TODO Check the char code (uscchar or iprivate)
			pass = digits > 1;
			if (pass)
				boost::fusion::at_c<0>(ctx.locals) = p;
		} else {
			pass = false;
		}
	}
};

template < typename OutputIterator >
struct hex_encoded_grammar :
		boost::spirit::karma::grammar< OutputIterator, std::string()> {
	typedef std::string value_type;
	typedef value_type::const_iterator value_iterator;

	hex_encoded_grammar() : hex_encoded_grammar::base_type(root)
	{
		namespace karma = boost::spirit::karma;
		is_hex_escaped ihe;
		hex_encoded = ("%x" << +karma::xdigit)[ihe];
		root = hex_encoded;
	}
	boost::spirit::karma::rule< OutputIterator, std::string(),
			boost::spirit::locals< value_iterator >> hex_encoded;
	boost::spirit::karma::rule< OutputIterator, value_type()> root;
};

template < typename OutputIterator >
struct iunreserved_grammar :
		boost::spirit::karma::grammar< OutputIterator, std::string()> {
	typedef std::string value_type;
	typedef value_type::const_iterator value_iterator;

	iunreserved_grammar() : iunreserved_grammar::base_type(root)
	{
		namespace karma = boost::spirit::karma;
		namespace phx = boost::phoenix;
		using karma::_val;
		using karma::_a;
		using karma::_1;

		is_hex_escaped ihe;
		unreserved = karma::alnum | karma::char_("-._~");
		ucschar = "%x" << +karma::xdigit;
		iunreserved = karma::eps[ karma::_a = phx::begin(_val) ] <<
				(ucschar[ihe] |
				unreserved[ karma::_pass = _a != phx::end(_val), _1 = *_a++ ]);
		root = iunreserved;
	}
	boost::spirit::karma::rule< OutputIterator, std::string()> root;
	boost::spirit::karma::rule< OutputIterator, std::string()> ucschar;
	boost::spirit::karma::rule< OutputIterator, char()> unreserved;
	boost::spirit::karma::rule< OutputIterator, std::string(),
			boost::spirit::karma::locals< value_iterator >> iunreserved;
};

template < typename OutputIterator >
struct ipath_grammar :
		boost::spirit::karma::grammar< OutputIterator, tip::iri::path()> {
	typedef tip::iri::path value_type;
	typedef boost::spirit::context<
			boost::fusion::cons< value_type const&, boost::fusion::nil_ >,
			boost::fusion::vector0<>
	> context_type;
	struct is_rooted {
		void
		operator()(boost::spirit::unused_type, context_type& ctx, bool& pass) const
		{
			pass = boost::fusion::at_c<0>(ctx.attributes).is_rooted();
		}
	};
	ipath_grammar() : ipath_grammar::base_type(root)
	{
		namespace karma = boost::spirit::karma;
		namespace phx = boost::phoenix;
		is_rooted ir;
		root %= -karma::lit('/')[ir]
			 << -(karma::string <<
				*(karma::lit('/') << karma::string));
	}
	boost::spirit::karma::rule< OutputIterator, tip::iri::path()> root;
};

template < typename OutputIterator >
struct ifragment_grammar :
		boost::spirit::karma::grammar< OutputIterator, tip::iri::fragment()> {
	ifragment_grammar() : ifragment_grammar::base_type(root)
	{
		namespace karma = boost::spirit::karma;
		root = karma::string;
	}
	boost::spirit::karma::rule< OutputIterator, tip::iri::fragment()> root;
};

}  // namespace gen
}  // namespace grammar
}  // namespace iri
}  // namespace tip



#endif /* TIP_IRI_GRAMMAR_IRI_GENERATE_HPP_ */
