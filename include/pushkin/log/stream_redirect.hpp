/*
 * stream_redirect.hpp
 *
 *  Created on: Jan 15, 2016
 *      Author: zmij
 */

#ifndef PUSHKIN_LOG_STREAM_REDIRECT_HPP_
#define PUSHKIN_LOG_STREAM_REDIRECT_HPP_

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>

namespace psst {
namespace log {

class stream_redirect {
public:
    stream_redirect(::std::ostream& stream,
            ::std::string const& file_name,
            ::std::ios_base::openmode open_mode
                 = ::std::ios_base::out | ::std::ios_base::app) :
        stream_(stream), old_(stream.rdbuf()),
        mode_{open_mode}, file_name_{file_name}
    {
        file_.open(file_name.c_str(), open_mode);
        if (!file_.good()) {
            std::ostringstream msg;
            msg << "Failed to open file " << file_name
                    << ": " << strerror(errno) << "\n";
            throw std::runtime_error(msg.str());
        }
        stream_.rdbuf(file_.rdbuf());
    }
    ~stream_redirect()
    {
        stream_.flush();
        stream_.rdbuf(old_);
    }

    ::std::string const&
    file_name() const
    { return file_name_; }

    void
    reopen()
    {
        stream_.rdbuf(old_);
        file_.close();
        file_.clear();
        file_.open(file_name_, mode_);
        stream_.rdbuf(file_.rdbuf());
    }
private:
    ::std::ostream&             stream_;
    ::std::streambuf*           old_;
    ::std::ofstream             file_;
    ::std::ios_base::openmode   mode_;
    ::std::string               file_name_;
};

}  // namespace log
}  // namespace psst


#endif /* PUSHKIN_LOG_STREAM_REDIRECT_HPP_ */
