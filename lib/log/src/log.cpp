/**
 * @file /tip-server/src/tip/log/log.cpp
 * @brief
 * @date Jul 8, 2015
 * @author: zmij
 */

#include <pushkin/log.hpp>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <ostream>
#include <fstream>
#include <istream>
#include <iterator>
#include <unistd.h>
#include <thread>
#include <queue>
#include <map>
#include <mutex>
#include <condition_variable>

#include <pushkin/log/stream_redirect.hpp>

#include <boost/thread/tss.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/date_facet.hpp>

#if defined __linux__ or defined MACOSX
#include <pthread.h>
#endif

namespace psst {
namespace log {

const std::string::size_type PROC_NAME_MAX_LEN = 12;
const std::string UNKNOW_CATEGORY = "<UNKNOWN>";
const std::string::size_type CATEGORY_MAX_LEN = UNKNOW_CATEGORY.size();

using timestamp_type = boost::posix_time::ptime;

namespace {

    std::string date_format = "%d.%m";
    timestamp_type
    timestamp()
    {
        return boost::posix_time::microsec_clock::universal_time();
    }

    std::string const&
    proc_name( std::string const& name = "")
    {
        static std::string name_("unknown");
        if (!name.empty()) {
            std::string::size_type pos = name.find_last_of('/');
            if (pos == std::string::npos)
                pos = 0;
            else
                ++pos;
            std::string::size_type len = name.size() - pos;
            if (len > PROC_NAME_MAX_LEN)
                len = PROC_NAME_MAX_LEN;
            name_ = name.substr(pos, len);
        }

        return name_;
    }

    logger::event_severity min_event_severity = logger::INFO;
    bool logger_use_colors = false;
    bool flush_stream_ = false;

    void
    lib_main(int, char* argv[], char* [])
    {
        proc_name(argv[0]);
    }
#ifdef GCC_5_3_1_FAIL
#ifdef __linux__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
    __attribute__((section(".init_array"))) void (* p_lib_main)(int,char*[],char*[]) = &lib_main;
#pragma GCC diagnostic pop
#endif /* __linux__ */
#endif
} // namespace

namespace {

std::map< std::string, logger::event_severity > name_to_severity {
    { "TRACE",         logger::TRACE    },
    { "DEBUG",         logger::DEBUG    },
    { "INFO",        logger::INFO    },
    { "WARNING",    logger::WARNING    },
    { "ERROR",        logger::ERROR }
};

std::map< logger::event_severity, util::ANSI_COLOR > severity_colors {
    { logger::TRACE,    (util::GREEN | util::DIM) },
    { logger::DEBUG,    (util::CYAN | util::DIM) },
    { logger::INFO,        (util::YELLOW | util::BRIGHT ) },
    { logger::WARNING,    (util::CYAN | util::BRIGHT ) },
    { logger::ERROR,    (util::RED | util::BRIGHT ) }
};

}  // namespace

std::ostream&
operator << (std::ostream& out, logger::event_severity es)
{
    std::ostream::sentry s(out);
    if (s) {
        switch (es) {
            case logger::TRACE:
                out << "TRACE";
                break;
            case logger::DEBUG:
                out << "DEBUG";
                break;
            case logger::INFO:
                out << "INFO";
                break;
            case logger::WARNING:
                out << "WARNING";
                break;
            case logger::ERROR:
                out << "ERROR";
                break;
            default:
                out << "UNKNOWN";
                break;
        }
    }
    return out;
}

std::istream&
operator >> (std::istream& in, logger::event_severity& es)
{
    std::istream::sentry s(in);
    if (s) {
        std::string name;
        if (in >> name) {
            std::transform(name.begin(), name.end(), name.begin(), ::toupper);
            auto f = name_to_severity.find(name);
            if (f != name_to_severity.end()) {
                es = f->second;
            } else {
                in.setstate(std::ios_base::failbit);
            }
        }
    }
    return in;
}

struct event_data {
    using buffer_type = boost::asio::streambuf;
    size_t                        thread_no_;
    timestamp_type                timestamp_;
    logger::event_severity        severity_;
    std::string                    category_;
    buffer_type                    buffer_;

    event_data(size_t t_no) :
        thread_no_(t_no), timestamp_(timestamp()), severity_(logger::TRACE),
        category_(UNKNOW_CATEGORY)
    {
    }
};

struct log_writer {
    using event_ptr     = ::std::shared_ptr<event_data>;
    using event_queue   = ::std::queue<event_ptr>;
    using mutex_type    = ::std::mutex;
    using lock_guard    = ::std::lock_guard<mutex_type>;
    using unique_lock   = ::std::unique_lock<mutex_type>;
    using redirect_ptr  = ::std::shared_ptr< stream_redirect >;

    ::std::ostream&                     out_;
    redirect_ptr                        redirect_;
    pid_t                               pid_;

    event_queue                         events_;
    std::condition_variable             cond_;
    mutex_type                          mtx_;

    bool                                finished_;


    log_writer(std::ostream& s)
            : out_(s), pid_(::getpid()), finished_(false)
    {
        using boost::gregorian::date_facet;
        date_facet* f = new date_facet(date_format.c_str());
        out_.imbue(std::locale(std::locale::classic(), f));
    }

    ~log_writer()
    {
        out_.flush();
    }

	void
	change_date_format()
	{
        using boost::gregorian::date_facet;
        date_facet* f = new date_facet(date_format.c_str());
        out_.imbue(std::locale(std::locale::classic(), f));
    }

	void
    redirect(::std::string const& file)
    {
        if (redirect_)
            throw ::std::runtime_error{ "Log is already redirected" };
        unique_lock lock{mtx_};
        redirect_ = ::std::make_shared< stream_redirect >( out_, file );
    }

    void
    reopen()
    {
        if (redirect_) {
            unique_lock lock{mtx_};
            redirect_->reopen();
        }
    }

    void
    run()
    {
        // TODO Wrap in if have posix
        #if defined __linux__
        pthread_setname_np(pthread_self(), "logger");
        #elif defined MACOSX
        pthread_setname_np("logger");
        #endif
        while (!finished_) {
            try {
                event_queue events;
                {
                    unique_lock lock{mtx_};
                    while (events_.empty() && !finished_) cond_.wait(lock);
                    events.swap(events_);
                }

                while (!events.empty()) {
                    while (!events.empty()) {
                        event_ptr e;
                        e = events.front();
                        events.pop();
                        if (!e) break;

                        event_data& evt = *e;
                        std::ostream::sentry s(out_);
                        if (s) {
                            bool use_colors = logger_use_colors;
                            if (use_colors)
                                out_ << severity_colors[ evt.severity_ ];
                            // process name
                            out_ << std::setw(PROC_NAME_MAX_LEN + 1) << std::left << proc_name() << ' ';
                            // pid
                            out_ << std::setw(6) << std::left << pid_ << ' ';
                            // thread no
                            out_ << std::setw(4) << std::left << evt.thread_no_ << ' ';
                            // category
                            out_ << std::setw(CATEGORY_MAX_LEN + 1) << evt.category_;
                            // timestamp
                            out_ << evt.timestamp_.date() << ' ' << evt.timestamp_.time_of_day() << ' ';
                            out_ << std::setw(8) << std::left << evt.severity_;

                            std::istreambuf_iterator<char> eob;
                            std::istreambuf_iterator<char> bi(&evt.buffer_);
                            std::ostream_iterator<char> out(out_);
                            std::copy(bi, eob, out);
                            if (use_colors)
                                out_ << util::CLEAR;
                            *out++ = '\n';
                            if (flush_stream_)
                                out_.flush();
                        }
                    }
                    unique_lock lock{mtx_};
                    events.swap(events_);
                }
                out_.flush();
            } catch (::std::exception const& e) {
                auto time = boost::posix_time::microsec_clock::universal_time();
                out_ << time.time_of_day() << " Exception in logging thread: " << e.what() << ::std::endl;
            } catch (...) {
                auto time = boost::posix_time::microsec_clock::universal_time();
                out_ << time.time_of_day() << " Unknown exception in logging thread"<< ::std::endl;
            }
        }
    }

    void
    push(event_ptr e)
    {
        unique_lock lock{mtx_};
        events_.push(e);
        cond_.notify_all();
    }

    void
    finish()
    {
        unique_lock lock{mtx_};
        finished_ = true;
        cond_.notify_all();
    }
};

struct logger::impl {
    struct thread_number {
        size_t no;
    };
    using thread_event_ptr = boost::thread_specific_ptr<event_data>;
    using thread_number_ptr = boost::thread_specific_ptr<thread_number>;
    using event_ptr = std::shared_ptr<event_data>;

    log_writer writer_;
    std::thread writer_thread_;

    thread_event_ptr event_;
    thread_number_ptr thread_no_;

    bool finished_;

    impl(std::ostream& out)
            : writer_(out), writer_thread_(), finished_(false)
    {
        writer_thread_ = std::thread([&]{
            writer_.run();
        });
    }

    ~impl()
    {
        try {
            flush();
            finished_ = true;
            writer_.finish();
            if (writer_thread_.joinable()) {
                writer_thread_.join();
            }
        } catch(...) {
        }
    }

    size_t
    thread_no()
    {
        static size_t t_no = 0;
        if (!thread_no_.get()) {
            thread_no_.reset(new thread_number{ t_no++ });
        }
        return thread_no_->no;
    }

    event_data&
    event()
    {
        if (!event_.get()) {
            event_.reset(new event_data( thread_no() ));
        }
        return *event_;
    }
    std::streambuf&
    buffer()
    {
        return event().buffer_;
    }

    size_t
    size()
    {
        return event().buffer_.size();
    }

    void
    category(std::string const& c)
    {
        event_data& evt = event();
        if (c.size() > CATEGORY_MAX_LEN) {
            evt.category_ = c.substr(0, CATEGORY_MAX_LEN);
        } else {
            evt.category_ = c;
        }
    }

    void
    flush()
    {
        if (!finished_ && event_.get()) {
            event_ptr e(event_.release());
            if (logger::min_severity() <= e->severity_
                && logger::OFF < e->severity_) {
                writer_.push(e);
            }
        }
    }

    void
    change_date_format()
    {
        writer_.change_date_format();
    }

    void
    redirect(::std::string const& file)
    {
        writer_.redirect(file);
    }

    void
    rotate()
    {
        writer_.reopen();
    }
};

logger&
logger::instance()
{
    static logger log_(std::clog);
    return log_;
}

void
logger::set_proc_name(std::string const& name)
{
    proc_name(name);
}

void
logger::set_stream(std::ostream& s)
{
    logger& l = instance();
    l.pimpl_.reset(new impl(s));
}

void
logger::set_target_file(::std::string const& file_name)
{
    logger& l = instance();
    l.pimpl_->redirect(file_name);
}

void
logger::set_date_format(std::string const& fmt)
{
    date_format = fmt;
    logger& l = instance();
    l.pimpl_->change_date_format();
}

void
logger::min_severity(event_severity s)
{
    min_event_severity = s;
}

logger::event_severity
logger::min_severity()
{
    return min_event_severity;
}

void
logger::use_colors(bool val)
{
    logger_use_colors = val;
}

bool
logger::use_colors()
{
    return logger_use_colors;
}

void
logger::flush_stream(bool v)
{
    flush_stream_ = v;
}

util::ANSI_COLOR
logger::severity_color()
{
    return severity_color( instance().severity() );
}

util::ANSI_COLOR
logger::severity_color(event_severity s)
{
    return severity_colors[s];
}


logger::logger(std::ostream& out)
        : pimpl_(new impl(out))
{
}

std::streambuf&
logger::buffer()
{
    return pimpl_->buffer();
}

logger&
logger::severity(event_severity s)
{
    pimpl_->event().severity_ = s;
    return *this;
}

logger::event_severity
logger::severity() const
{
    return pimpl_->event().severity_;
}

logger&
logger::category(std::string const& c)
{
    pimpl_->category(c);
    return *this;
}

logger&
logger::flush()
{
    pimpl_->flush();
    return *this;
}

void
logger::rotate()
{
    pimpl_->rotate();
}

}  // namespace log
namespace util {

log::logger&
operator << (log::logger& out, ANSI_COLOR col)
{
    if(log::logger::use_colors()) {
        std::ostream os(&out.buffer());
        os << col;
    }
    return out;
}

}  // namespace util
}  // namespace psst

