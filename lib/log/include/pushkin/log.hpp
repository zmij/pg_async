/**
 * @file pushkin/log.hpp
 * @brief
 * @date Jul 8, 2015
 * @author: zmij
 */

#ifndef PUSHKIN_LOG_LOG_HPP_
#define PUSHKIN_LOG_LOG_HPP_

#include <iostream>
#include <memory>

#include <pushkin/log/ansi_colors.hpp>

namespace psst {
namespace log {

/**
 * Logging facility
 */
class logger {
public:
    /**
     * Severity of event being logged
     */
    enum event_severity {
        OFF,    //!< OFF
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

    /**
     * @return Thread-specific buffer for writing a log message
     */
    std::streambuf&
    buffer();

    /**
     * Set severity of current logged event.
     * @param severity @e new event severity
     * @return Logger instance for call chaining
     */
    logger&
    severity(event_severity);

    /**
     * @return Severity of current event
     */
    event_severity
    severity() const;

    /**
     * Set category of current logged event
     * @param category New category for the event
     * @return Logger instance for call chaining
     */
    logger&
    category(std::string const&);

    /**
     * Flush the event
     * @return Logger instance for call chaining
     */
    logger&
    flush();

    /**
     * Reopen log file
     */
    void
    rotate();

    /**
     * Singleton instance of the logger
     * @return
     */
    static logger&
    instance();

    static void
    set_proc_name(std::string const&);
    /**
     * Set output stream of the logger.
     * @param stream
     */
    static void
    set_stream(std::ostream&);

    static void
    set_target_file(::std::string const& file_name);

    static void
    set_date_format(std::string const& fmt);

    /**
     * Set minimum event severity that will be written to the log.
     * @param severity
     */
    static void
    min_severity(event_severity);

    /**
     * Get minimum event severity that will be written to the log
     * @return
     */
    static event_severity
    min_severity();

    /**
     * Set true to use the ANSI color escape codes in the output
     * @param
     */
    static void
    use_colors(bool);

    /**
     * Check if the logger uses colored output
     * @return
     */
    static bool
    use_colors();

    /**
     * Get default color for current event severity
     * @return
     */
    static util::ANSI_COLOR
    severity_color();

    /**
     * Get ANSI color for a given event severity
     * @param
     * @return
     */
    static util::ANSI_COLOR
    severity_color(event_severity);

    /**
     * Set flush stream after every logged event
     * @param
     */
    static void
    flush_stream(bool);
private:
    logger(std::ostream&);

    struct impl;
    typedef std::shared_ptr<impl> pimpl;
    pimpl pimpl_;
};

/**
 * Auto flushing class for logging.
 * Convenience class for usage as follows:
 * @code{.cpp}
 * namespace {
 *
 * using namespace tip::log;
 *
 * const std::string LOG_CATEGORY = "LOCAL";
 * logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
 * local
 * local_log(logger::event_severity s = logger::DEBUG)
 * {
 *         return local(LOG_CATEGORY, s);
 * }
 * } // namespace
 *
 * local_log() << "Log a message with default setting for this file";
 * @endcode
 */
class local {
public:
    /**
     * Constructs a local logger with a category and severity
     * @param category
     * @param severity
     */
    local(std::string const& category, logger::event_severity severity =
                logger::TRACE)
            :
                do_flush_(true)
    {
        logger::instance().category(category).severity(severity);
    }
    ~local()
    {
        if (do_flush_)
            logger::instance().flush();
    }

    local(local const& rhs) : do_flush_(true)
    {
        rhs.do_flush_ = false;
    }

    local&
    operator = (local const& rhs)
    {
        do_flush_ = true;
        rhs.do_flush_ = false;
        return *this;
    }

    logger*
    operator->()
    {
        return &logger::instance();
    }
    logger const*
    operator->() const
    {
        return &logger::instance();
    }
    logger&
    operator*()
    {
        return logger::instance();
    }
    logger const&
    operator*() const
    {
        return logger::instance();
    }
private:
    mutable bool do_flush_;
};

inline logger&
endl(logger& out)
{
    return out.flush();
}


template < typename T >
logger&
operator << (logger& out, T const& v)
{
    std::ostream os(&out.buffer());
    os << v;
    return out;
}

template < typename T >
local
operator << (local out, T const& v)
{
    logger::instance() << v;
    return out;
}

inline logger&
operator << (logger& out, logger& (*fp)(logger&))
{
    return fp(out);
}

namespace detail {

/**
 * Helper struct to set severity for current event
 */
struct _set_severity {
    logger::event_severity s_;
};

/**
 * Helper struct to set category for current event
 */
struct _set_category {
    std::string c_;
};

}  // namespace detail

inline detail::_set_severity
severity(logger::event_severity s) { return { s }; }

inline detail::_set_category
category(std::string const& c) { return { c }; }

inline logger&
operator << (logger& out, detail::_set_severity const& s)
{
    return out.severity(s.s_);
}

inline logger&
operator << (logger& out, detail::_set_category const& s)
{
    return out.category(s.c_);
}

std::ostream&
operator << (std::ostream& out, logger::event_severity es);

std::istream&
operator >> (std::istream& in, logger::event_severity& es);

}  // namespace log
namespace util {

log::logger&
operator << (log::logger&, ANSI_COLOR);

}  // namespace util
}  // namespace psst

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

#define LOCAL_LOGGING_FACILITY(c, s) \
    namespace { \
        using namespace ::psst::log; \
        const std::string c##_LOG_CATEGORY = #c;    \
        const logger::event_severity c##_DEFAULT_SEVERITY = logger::s; \
        local \
        local_log(logger::event_severity sv = c##_DEFAULT_SEVERITY) \
        { return local(c##_LOG_CATEGORY, sv); }\
    } \
    using ::psst::log::logger

#define LOCAL_LOGGING_FACILITY_CFG(c, s) \
    namespace { \
        using namespace ::psst::log; \
        const std::string c##_LOG_CATEGORY = #c;    \
        const logger::event_severity c##_DEFAULT_SEVERITY = s; \
        local \
        local_log(logger::event_severity sv = c##_DEFAULT_SEVERITY) \
        { return local(c##_LOG_CATEGORY, sv); }\
    } \
    using ::psst::log::logger

#define LOCAL_LOGGING_FACILITY_FUNC(c, s, f) \
    namespace { \
        using namespace ::psst::log; \
        const std::string c##_LOG_CATEGORY = #c;    \
        const logger::event_severity c##_DEFAULT_SEVERITY = logger::s; \
        local \
        f(logger::event_severity sv = c##_DEFAULT_SEVERITY) \
        { return local(c##_LOG_CATEGORY, sv); }\
    } \
    using ::psst::log::logger

#define LOCAL_LOGGING_FACILITY_CFG_FUNC(c, s, f) \
    namespace { \
        using namespace ::psst::log; \
        const std::string c##_LOG_CATEGORY = #c;    \
        const logger::event_severity c##_DEFAULT_SEVERITY = s; \
        local \
        f(logger::event_severity sv = c##_DEFAULT_SEVERITY) \
        { return local(c##_LOG_CATEGORY, sv); }\
    } \
    using ::psst::log::logger

#pragma GCC diagnostic pop

#endif /* PUSHKIN_LOG_LOG_HPP_ */
