/*
 * null_logger.hpp
 *
 *  Created on: Jul 19, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_NULL_LOGGER_HPP_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_NULL_LOGGER_HPP_

#include <iosfwd>

namespace tip {
namespace util {
enum ANSI_COLOR {
	// clear the color
	CLEAR		= 0x00,
	// attribs
	BRIGHT		= 0x01,
	DIM			= BRIGHT * 2,
	UNDERLINE	= DIM * 2,

	FOREGROUND	= UNDERLINE * 2,
	BACKGROUND	= FOREGROUND * 2,
	// normal colors
	BLACK		= BACKGROUND * 2,
	RED			= BLACK * 2,
	GREEN		= RED * 2,
	YELLOW		= GREEN * 2,
	BLUE		= YELLOW * 2,
	MAGENTA		= BLUE * 2,
	CYAN		= MAGENTA * 2,
	WHITE		= CYAN * 2,

	// Color mask
	COLORS		= BLACK | RED | GREEN | YELLOW |
					BLUE | MAGENTA | CYAN | WHITE
};
}  // namespace util
namespace log {

class logger {
public:
	/**
	 * Severity of event being logged
	 */
	enum event_severity {
		OFF,	//!< OFF
		TRACE,  //!< TRACE
		DEBUG,  //!< DEBUG
		INFO,   //!< INFO
		WARNING,//!< WARNING
		ERROR   //!< ERROR
	};
public:
	logger(logger const&) = delete;
	logger&
	operator = (logger const&) = delete;

	static void
	set_proc_name(std::string const&) {}
	/**
	 * Set output stream of the logger.
	 * @param stream
	 */
	static void
	set_stream(std::ostream&) {}

	/**
	 * Set minimum event severity that will be written to the log.
	 * @param severity
	 */
	static void
	min_severity(event_severity) {}

	/**
	 * Get minimum event severity that will be written to the log
	 * @return
	 */
	static event_severity
	min_severity() { return OFF; }

	/**
	 * Set true to use the ANSI color escape codes in the output
	 * @param
	 */
	static void
	use_colors(bool) {}

	/**
	 * Check if the logger uses colored output
	 * @return
	 */
	static bool
	use_colors() { return false; }

	/**
	 * Get default color for current event severity
	 * @return
	 */
	static util::ANSI_COLOR
	severity_color() { return util::CLEAR; }

	/**
	 * Get ANSI color for a given event severity
	 * @param
	 * @return
	 */
	static util::ANSI_COLOR
	severity_color(event_severity) { return util::CLEAR; }
};

class local {
public:
	local(std::string const& category,
			logger::event_severity severity = logger::OFF)
	{}
};

template < typename T >
logger&
operator << (logger& out, T const& v)
{
	return out;
}

template < typename T >
local
operator << (local out, T const& v)
{
	return out;
}

}  // namespace log
}  // namespace tip


#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_NULL_LOGGER_HPP_ */
