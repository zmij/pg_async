/*
 * datetime_parse.hpp
 *
 *  Created on: Aug 26, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_GRAMMAR_DATETIME_PARSE_HPP_
#define TIP_HTTP_COMMON_GRAMMAR_DATETIME_PARSE_HPP_

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/date_time.hpp>
#include <exception>

namespace tip {
namespace http {
namespace grammar {
namespace parse {

struct month_grammar :
		boost::spirit::qi::symbols< char, boost::date_time::months_of_year > {
	month_grammar()
	{
		using namespace boost::date_time;
		add
			("Jan", Jan)
			("Feb", Feb)
			("Mar", Mar)
			("Apr", Apr)
			("May", May)
			("Jun", Jun)
			("Jul", Jul)
			("Aug", Aug)
			("Sep", Sep)
			("Oct", Oct)
			("Nov", Nov)
			("Dec", Dec)
		;
	}
};

struct weekday_grammar :
		boost::spirit::qi::symbols< char, boost::date_time::weekdays > {
	weekday_grammar()
	{
		using namespace boost::date_time;
		add
			("Mon",			Monday)
			("Tue",			Tuesday)
			("Wed",			Wednesday)
			("Thu",			Thursday)
			("Fri",			Friday)
			("Sat",			Saturday)
			("Sun",			Sunday)
			("Monday",		Monday)
			("Tuesday",		Tuesday)
			("Wednesday",	Wednesday)
			("Thursday",	Thursday)
			("Friday",		Friday)
			("Saturday",	Saturday)
			("Sunday",		Sunday)
		;
	}
};

template < typename InputIterator >
struct http_time_grammar :
		boost::spirit::qi::grammar< InputIterator, boost::posix_time::time_duration()> {
	typedef boost::posix_time::time_duration value_type;
	http_time_grammar() : http_time_grammar::base_type(time)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;
		using qi::_pass;
		using qi::_val;
		using qi::_1;
		using qi::_2;
		using qi::_3;

		_2digits = qi::uint_parser< std::uint32_t, 10, 2, 2 >();

		time = (_2digits >> ':' >> _2digits >> ':' >> _2digits )
			[ _pass = (_1 < 24) && (_2 < 60) && (_3 < 60),
			  _val = phx::construct< value_type >(_1, _2, _3)];
	}
	boost::spirit::qi::rule< InputIterator, value_type()> time;
	boost::spirit::qi::rule< InputIterator, std::uint32_t() > _2digits;
};

template < typename InputIterator >
struct date_rfc1123_grammar :
		boost::spirit::qi::grammar< InputIterator, boost::gregorian::date()> {
	typedef boost::gregorian::date value_type;
	date_rfc1123_grammar() : date_rfc1123_grammar::base_type(date)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;
		using qi::_pass;
		using qi::_val;
		using qi::_2;
		using qi::_3;
		using qi::_4;

		_2digits = qi::uint_parser< std::uint32_t, 10, 2, 2 >();
		_4digits = qi::uint_parser< std::uint32_t, 10, 4, 4 >();
		date = (weekday >> qi::lit(", ") >> _2digits >> ' ' >> month >> ' ' >> _4digits)
			[
			 _pass = _2 < 32,
			 phx::try_[
				_val = phx::construct< value_type >( _4, _3, _2 )
			  ].catch_all[
				_pass = false
			  ]
			];
	}
	boost::spirit::qi::rule< InputIterator, value_type()> date;
	weekday_grammar weekday;
	month_grammar month;
	boost::spirit::qi::rule< InputIterator, std::int32_t() > _2digits;
	boost::spirit::qi::rule< InputIterator, std::int32_t() > _4digits;
};

template < typename InputIterator >
struct datetime_rfc1123_grammar :
		boost::spirit::qi::grammar< InputIterator, boost::posix_time::ptime()> {
	typedef boost::posix_time::ptime value_type;
	datetime_rfc1123_grammar() : datetime_rfc1123_grammar::base_type(datetime)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;
		using qi::_val;
		using qi::_1;
		using qi::_2;

		datetime = (date >> ' ' >> time >> qi::lit(" GMT"))
			[ _val = phx::construct<value_type>( _1, _2 ) ];
	}
	boost::spirit::qi::rule< InputIterator, value_type()> datetime;
	http_time_grammar< InputIterator > time;
	date_rfc1123_grammar< InputIterator > date;
};

template < typename InputIterator >
struct date_rfc850_grammar :
		boost::spirit::qi::grammar< InputIterator, boost::gregorian::date()> {
	typedef boost::gregorian::date value_type;
	date_rfc850_grammar() : date_rfc850_grammar::base_type(root)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;
		using qi::_pass;
		using qi::_val;
		using qi::_2;
		using qi::_3;
		using qi::_4;
		using qi::_a;

		_2digits = qi::uint_parser< std::uint32_t, 10, 2, 2 >();
		date = (weekday >> qi::lit(", ") >> _2digits >> '-' >> month >> '-' >> _2digits)
			[
			 _pass = _2 < 32,
			 phx::try_[
				phx::if_(_4 > 70)[
					_a = _4 + 1900
				].else_[
					_a = _4 + 2000
				],
				_val = phx::construct< value_type >( _a, _3, _2 )
			  ].catch_all[
				_pass = false
			  ]
			];
		root = date;
	}
	boost::spirit::qi::rule< InputIterator, value_type()> root;
	boost::spirit::qi::rule< InputIterator, value_type(),
			boost::spirit::locals< std::int32_t >> date;
	weekday_grammar weekday;
	month_grammar month;
	boost::spirit::qi::rule< InputIterator, std::int32_t() > _2digits;
};

template < typename InputIterator >
struct datetime_rfc850_grammar :
		boost::spirit::qi::grammar< InputIterator, boost::posix_time::ptime()> {
	typedef boost::posix_time::ptime value_type;
	datetime_rfc850_grammar() : datetime_rfc850_grammar::base_type(datetime)
	{
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;
		using qi::_val;
		using qi::_1;
		using qi::_2;

		datetime = (date >> ' ' >> time >> qi::lit(" GMT"))
			[ _val = phx::construct<value_type>( _1, _2 ) ];
	}
	boost::spirit::qi::rule< InputIterator, value_type()> datetime;
	http_time_grammar< InputIterator > time;
	date_rfc850_grammar< InputIterator > date;
};

template < typename InputIterator >
struct datetime_asctime_grammar :
		boost::spirit::qi::grammar< InputIterator, boost::posix_time::ptime()> {
	typedef boost::posix_time::ptime value_type;
	typedef value_type::date_type date_type;
	typedef value_type::time_duration_type time_type;

	datetime_asctime_grammar() : datetime_asctime_grammar::base_type(datetime)
	{
		namespace qi = boost::spirit::qi;
		namespace qi = boost::spirit::qi;
		namespace phx = boost::phoenix;
		using qi::_pass;
		using qi::_val;
		using qi::_2;
		using qi::_3;
		using qi::_4;
		using qi::_5;

		digit = qi::uint_parser< std::uint32_t, 10, 1, 1 >();
		_2digits = qi::uint_parser< std::uint32_t, 10, 2, 2 >();
		_4digits = qi::uint_parser< std::uint32_t, 10, 4, 4 >();
		datetime = (weekday >> ' ' >> month >>  ' '
				>> (_2digits | ' ' >> digit)
				>> ' ' >> time >> ' ' >> _4digits)
			[
			  _pass = _3 < 32,
			  phx::try_[
				_val = phx::construct< value_type >(
					phx::construct< date_type >( _5, _2, _3 ),
					_4
				)
			  ].catch_all[
				_pass = false
			  ]
			];
	}
	boost::spirit::qi::rule< InputIterator, value_type()> datetime;
	weekday_grammar weekday;
	month_grammar month;
	boost::spirit::qi::rule< InputIterator, std::int32_t() > digit;
	boost::spirit::qi::rule< InputIterator, std::int32_t() > _2digits;
	boost::spirit::qi::rule< InputIterator, std::int32_t() > _4digits;
	http_time_grammar< InputIterator > time;
};

template < typename InputIterator >
struct http_datetime_grammar :
		boost::spirit::qi::grammar< InputIterator, boost::posix_time::ptime()> {
	typedef boost::posix_time::ptime value_type;
	http_datetime_grammar() : http_datetime_grammar::base_type(datetime)
	{
		namespace qi = boost::spirit::qi;
		datetime = rfc1123 | rfc850 | ansi;
	}
	boost::spirit::qi::rule< InputIterator, value_type()> datetime;
	datetime_rfc1123_grammar< InputIterator > rfc1123;
	datetime_rfc850_grammar< InputIterator > rfc850;
	datetime_asctime_grammar< InputIterator > ansi;
};

}  // namespace parse
}  // namespace grammar
}  // namespace http
}  // namespace tip


#endif /* TIP_HTTP_COMMON_GRAMMAR_DATETIME_PARSE_HPP_ */
