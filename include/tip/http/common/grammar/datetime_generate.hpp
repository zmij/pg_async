/*
 * datetime_generate.hpp
 *
 *  Created on: Aug 27, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_GRAMMAR_DATETIME_GENERATE_HPP_
#define TIP_HTTP_COMMON_GRAMMAR_DATETIME_GENERATE_HPP_


#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#define MAKE_LAZY_FUNCTION(resulttype, name, arg_type, body) \
struct name##_impl { \
	typedef resulttype result_type; \
	result_type \
	operator()(arg_type val) const \
	{ \
		return body; \
	} \
}; \
boost::phoenix::function< name##_impl > name

namespace tip {
namespace http {
namespace grammar {
namespace gen {

struct month_grammar :
		boost::spirit::karma::symbols< boost::date_time::months_of_year, char const * > {
	month_grammar()
	{
		using namespace boost::date_time;
		add
			(Jan, "Jan")
			(Feb, "Feb")
			(Mar, "Mar")
			(Apr, "Apr")
			(May, "May")
			(Jun, "Jun")
			(Jul, "Jul")
			(Aug, "Aug")
			(Sep, "Sep")
			(Oct, "Oct")
			(Nov, "Nov")
			(Dec, "Dec")
		;
	}
};

struct weekday_grammar :
		boost::spirit::karma::symbols< boost::date_time::weekdays, char const * > {
	weekday_grammar()
	{
		using namespace boost::date_time;
		add
			(Monday,	"Mon")
			(Tuesday,	"Tue")
			(Wednesday,	"Wed")
			(Thursday,	"Thu")
			(Friday,	"Fri")
			(Saturday,	"Sat")
			(Sunday,	"Sun")
		;
	}
};

template < typename OutputIterator >
struct http_datetime_grammar :
		boost::spirit::karma::grammar< OutputIterator, boost::posix_time::ptime()> {
	typedef boost::posix_time::ptime value_type;
	http_datetime_grammar() : http_datetime_grammar::base_type(datetime)
	{
		namespace karma = boost::spirit::karma;
		using karma::_val;
		using karma::lit;
		using karma::int_;
		using karma::right_align;
		using karma::_1;
		using karma::_2;
		using karma::_3;
		using karma::_4;
		using karma::_5;
		using karma::_6;
		using karma::_7;

		zero_padded = right_align(2,0)[int_];
		datetime = (weekday << ", " << zero_padded << " "
				<< month << " " << int_ << " "
				<< zero_padded << ':' << zero_padded << ':' << zero_padded
				<< " GMT")
				[
				   _1 = day_of_week(_val),
				   _2 = day_of_month(_val),
				   _3 = month_(_val),
				   _4 = year(_val),
				   _5 = hours(_val),
				   _6 = minutes(_val),
				   _7 = seconds(_val)
				];
	}
	boost::spirit::karma::rule< OutputIterator, value_type()> datetime;
	boost::spirit::karma::rule< OutputIterator, std::int32_t()> zero_padded;
	weekday_grammar weekday;
	month_grammar month;

	MAKE_LAZY_FUNCTION(boost::date_time::weekdays, day_of_week,
			boost::posix_time::ptime const&, val.date().day_of_week().as_enum());
	MAKE_LAZY_FUNCTION(std::int32_t, day_of_month,
			boost::posix_time::ptime const&, val.date().day());
	MAKE_LAZY_FUNCTION( boost::date_time::months_of_year, month_,
			boost::posix_time::ptime const&, val.date().month().as_enum() );
	MAKE_LAZY_FUNCTION(std::int32_t, year,
			boost::posix_time::ptime const&, val.date().year());
	MAKE_LAZY_FUNCTION(std::int32_t, hours,
			boost::posix_time::ptime const&, val.time_of_day().hours());
	MAKE_LAZY_FUNCTION(std::int32_t, minutes,
			boost::posix_time::ptime const&, val.time_of_day().minutes());
	MAKE_LAZY_FUNCTION(std::int32_t, seconds,
			boost::posix_time::ptime const&, val.time_of_day().seconds());

};

}  // namespace gen
}  // namespace grammar
}  // namespace http
}  // namespace tip


#endif /* TIP_HTTP_COMMON_GRAMMAR_DATETIME_GENERATE_HPP_ */
