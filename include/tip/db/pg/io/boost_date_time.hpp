/*
 * boost_date_time.hpp
 *
 *  Created on: Sep 2, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_BOOST_DATE_TIME_HPP_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_BOOST_DATE_TIME_HPP_

#include <tip/db/pg/protocol_io_traits.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/date_time.hpp>

namespace tip {
namespace db {
namespace pg {
namespace io {

namespace grammar {
namespace parse {

template < typename InputIterator >
struct time_grammar :
		boost::spirit::qi::grammar< InputIterator, boost::posix_time::time_duration()> {
	typedef boost::posix_time::time_duration value_type;
	time_grammar() : time_grammar::base_type(time)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;
		using qi::_pass;
		using qi::_val;
		using qi::_1;
		using qi::_2;
		using qi::_3;
		using qi::_4;
		_2digits = qi::uint_parser< std::uint32_t, 10, 2, 2 >();
		fractional_part = -('.' >> qi::long_);

		time = (_2digits >> ':' >> _2digits >> ':' >> _2digits >> fractional_part )
			[ _pass = (_1 < 24) && (_2 < 60) && (_3 < 60),
			  _val = phx::construct< value_type >(_1, _2, _3, _4)];
	}
	boost::spirit::qi::rule< InputIterator, value_type()> time;
	boost::spirit::qi::rule< InputIterator, std::uint32_t() > _2digits;
	boost::spirit::qi::rule< InputIterator, std::uint64_t() > fractional_part;
};

template < typename InputIterator >
struct date_grammar :
		boost::spirit::qi::grammar< InputIterator, boost::gregorian::date()> {
	typedef boost::gregorian::date value_type;
	date_grammar() : date_grammar::base_type(date)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;
		using qi::_pass;
		using qi::_val;
		using qi::_1;
		using qi::_2;
		using qi::_3;
		_2digits = qi::uint_parser< std::uint32_t, 10, 2, 2 >();
		_4digits = qi::uint_parser< std::uint32_t, 10, 4, 4 >();
		date = (_4digits >> "-" >> _2digits >> "-" >> _2digits)
			[
			 	 _pass = _3 < 32,
				 phx::try_[
					_val = phx::construct< value_type > (_1, _2, _3)
				].catch_all[
					_pass = false
				]
			];
	}
	boost::spirit::qi::rule< InputIterator, value_type()> date;
	boost::spirit::qi::rule< InputIterator, std::int32_t() > _2digits;
	boost::spirit::qi::rule< InputIterator, std::int32_t() > _4digits;
};

template < typename InputIterator, typename DateTime >
struct datetime_grammar;

template < typename InputIterator >
struct datetime_grammar< InputIterator, boost::posix_time::ptime > :
		boost::spirit::qi::grammar< InputIterator, boost::posix_time::ptime()> {
	typedef boost::posix_time::ptime value_type;
	datetime_grammar() : datetime_grammar::base_type(datetime)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;
		using qi::_val;
		using qi::_1;
		using qi::_2;

		datetime = (date >> ' ' >> time)
			[ _val = phx::construct< value_type >( _1, _2) ];
	}
	boost::spirit::qi::rule< InputIterator, value_type()> datetime;
	date_grammar< InputIterator > date;
	time_grammar< InputIterator > time;
};

}  // namespace parse
}  // namespace grammar

template < >
struct protocol_parser< boost::posix_time::ptime, TEXT_DATA_FORMAT > :
		detail::parser_base< boost::posix_time::ptime > {
	typedef detail::parser_base< boost::posix_time::ptime > base_type;
	typedef typename base_type::value_type value_type;
	protocol_parser(value_type& v) : base_type(v) {}

	size_t
	size() const;

	template < typename InputIterator >
    InputIterator
    operator()(InputIterator begin, InputIterator end)
	{
		typedef std::iterator_traits< InputIterator > iter_traits;
		typedef typename iter_traits::value_type iter_value_type;
		static_assert(std::is_same< iter_value_type, byte >::type::value,
				"Input iterator must be over a char container");
		namespace qi = boost::spirit::qi;
		namespace parse = grammar::parse;

		parse::datetime_grammar< InputIterator, value_type > grammar;
		value_type tmp;
		if (qi::parse(begin, end, grammar, tmp)) {
			std::swap(tmp, base_type::value);
		}
		return begin;
	}
};

namespace traits {

//@{
template < >
struct pgcpp_data_mapping< oids::type::timestamp > :
		detail::data_mapping_base< oids::type::timestamp, boost::posix_time::ptime > {};
template < >
struct cpppg_data_mapping< boost::posix_time::ptime > :
		detail::data_mapping_base< oids::type::timestamp, boost::posix_time::ptime > {};

template <>
struct is_nullable< boost::posix_time::ptime > : ::std::true_type {};

template <>
struct nullable_traits< boost::posix_time::ptime > {
	inline static bool
	is_null(boost::posix_time::ptime const& val)
	{
		return val.is_not_a_date_time();
	}
	inline static void
	set_null(boost::posix_time::ptime& val)
	{
		val = boost::posix_time::ptime{};
	}
};
//@}

}  // namespace traits

}  // namespace io
}  // namespace pg
}  // namespace db
}  // namespace tip


#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_BOOST_DATE_TIME_HPP_ */
