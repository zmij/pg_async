/* $Id: pstream.h,v 1.9 2002/01/07 11:43:47 redi Exp $
PStreams - POSIX Process I/O for C++
Copyright (C) 2001 Jonathan Wakely

This file is part of PStreams.

PStreams is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

PStreams is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with PStreams; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
Provides ipstream, opstream (and for some systems, pstream).
Common implementation in abstract base class pstreambase.
Pstreambase has no public constructors, as it shouldn't be used directly.
Should always use the public interface of the derived concrete classes:

ipstream();
opstream();
pstream();
   Creates a new object with no associated process.

ipstream(const char* command, ios_base::openmode = ios_base::in);
opstream(const char* command, ios_base::openmode = ios_base::out);
pstream(const char* command, ios_base::openmode = ios_base::in|ios_base::out);
   Passes command to /bin/sh and opens a pipe to the new process.

void open(const char* command, ios_base::openmode);
   Passes command to /bin/sh and opens a pipe to the new process.

Writing to an open pstream writes to the standard input of the command;
the command's standard output is the same as that of the process that
created the pstream object, unless this is altered by the command itself.
Conversely, reading from a pstream reads the command's standard output,
and the command's standard input is the same as that of the process that
created the object.

The pstream class for both reading and writing to a process is only
available on some sytems, where popen(3) is implemented using a
bidirectional pipe. On other systems using any openmode except ios_base::in
or ios_base::out is an error.

N.B. The interface is slightly different for the non-ISO compliant pstreams.
*/

#ifndef REDI_PSTREAM_H
#define REDI_PSTREAM_H

#define PSTREAMS_VERSION 0x000b   // 0.11

// check whether to provide pstream
// popen() needs to use bidirectional pipe
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
# define REDI_PSTREAMS_POPEN_USES_BIDIRECTIONAL_PIPE 1
#elif defined(__NetBSD_Version__) && __NetBSD_Version__ >= 0x01040000
# define REDI_PSTREAMS_POPEN_USES_BIDIRECTIONAL_PIPE 1
#endif

#if ! ( BACK_COMPAT == 1 || GCC_BACK_COMPAT == 1 )

#include <ios>
#include <streambuf>
#include <string>
#include <cstdio>

namespace redi
{
// TODO comment code better

  template <typename CharT, typename Traits>
    class basic_pstreambuf : public std::basic_streambuf<CharT, Traits>
    {
      typedef std::string   string;
      typedef std::ios_base ios_base;

    public:
      basic_pstreambuf()
      : file_(0), take_from_buf_(false)
      { }

      basic_pstreambuf(const string& command, ios_base::openmode mode)
      : file_(0), take_from_buf_(false)
      { this->open(command, mode_); }

      ~basic_pstreambuf() { this->close(); }

      bool
      is_open() const { return bool(file_); }

      basic_pstreambuf*
      open(const string& command, ios_base::openmode mode)
      {
        file_ = ::popen(command.c_str(), openmode2str(mode).c_str());
        // TODO Document that there is no way to tell if command ran OK.
        // If command fails a non-NULL FILE* is still returned.
        return is_open() ? this : NULL;
      }

      basic_pstreambuf*
      close(void)
      {
        if (this->is_open())
        {
          ::pclose(file_);
          file_ = 0;
        }
        return this;
      }

    protected:
      int_type overflow(int_type c);

      int_type uflow();

      int_type underflow();

      int_type pbackfail(int_type c = traits_type::eof());

      bool read(int_type& c);

      bool write(int_type c);

      string
      openmode2str(ios_base::openmode);

    private:
      basic_pstreambuf(const basic_pstreambuf&);
      basic_pstreambuf& operator=(const basic_pstreambuf&);

      FILE* file_;
      char_type char_buf_;
      bool take_from_buf_;
    };

  template <typename CharT, typename Traits = std::char_traits<CharT> >
    class basic_ipstream : public std::basic_istream<CharT, Traits>
    {
      typedef std::basic_istream<CharT, Traits>     istream_type;
      typedef basic_pstreambuf<CharT, Traits>       streambuf_type;
      typedef streambuf_type::string                string;
      typedef streambuf_type::ios_base              ios_base;

    public:
      basic_ipstream()
      : istream_type(NULL), command_(), buf_()
      { this->init(&buf_); }

      basic_ipstream(const string& command, ios_base::openmode mode = ios_base::in)
      : istream_type(NULL), command_(command), buf_()
      {
        this->init(&buf_);
        this->open(command_, mode);
      }

      ~basic_ipstream() { }

      bool
      is_open() const { return buf_.is_open(); }

      void
      open(const string& command, ios_base::openmode mode = ios_base::in)
      { buf_.open((command_=command), mode); }

      void
      close() { buf_.close(); }

      const string&
      command() const { return command_; }

    protected:
      string command_;
      streambuf_type buf_;
    };

  template <typename CharT, typename Traits = std::char_traits<CharT> >
    class basic_opstream : public std::basic_ostream<CharT, Traits>
    {
      typedef std::basic_ostream<CharT, Traits>     ostream_type;
      typedef basic_pstreambuf<CharT, Traits>       streambuf_type;
      typedef streambuf_type::string                string;
      typedef streambuf_type::ios_base              ios_base;

    public:
      basic_opstream()
      : ostream_type(NULL), command_(), buf_()
      { this->init(&buf_); }

      basic_opstream(const string& command, ios_base::openmode mode = ios_base::out)
      : ostream_type(NULL), command_(command), buf_()
      {
        this->init(&buf_);
        this->open(command_, mode);
      }

      ~basic_opstream() { }

      bool
      is_open() const { return buf_.is_open(); }

      void
      open(const string& command, ios_base::openmode mode = ios_base::out)
      { buf_.open((command_=command), mode); }

      void
      close() { buf_.close(); }

      const string&
      command() const { return command_; }

    protected:
      string command_;
      streambuf_type buf_;
    };

  // typedefs for common template parameters
  typedef basic_ipstream<char> ipstream;
  typedef basic_opstream<char> opstream;

  // member definitions for pstreambuf

  template <typename C, typename T>
    typename basic_pstreambuf<C,T>::int_type
    basic_pstreambuf<C,T>::overflow(int_type c)
    {
      if (!traits_type::eq_int_type(c, traits_type::eof()))
      {
        if (!this->write(c))
          return traits_type::eof();
        else
          return c;
      }
      return traits_type::not_eof(c);
    }

  template <typename C, typename T>
    typename basic_pstreambuf<C,T>::int_type
    basic_pstreambuf<C,T>::uflow()
    {
      if (take_from_buf_)
      {
        take_from_buf_ = false;
        return traits_type::to_int_type(char_buf_);
      }
      else
      {
        int_type c;

        if (!this->read(c))
          return traits_type::eof();
        else
        {
          char_buf_ = traits_type::to_char_type(c);
          return traits_type::to_int_type(c);
        }
      }
    }

  template <typename C, typename T>
    typename basic_pstreambuf<C,T>::int_type
    basic_pstreambuf<C,T>::underflow()
    {
      if (take_from_buf_)
      {
        return traits_type::to_int_type(char_buf_);
      }
      else
      {
        int_type c;

        if (!this->read(c))
          return traits_type::eof();
        else
        {
          take_from_buf_ = true;
          char_buf_ = c;
          return traits_type::to_int_type(c);
        }
      }
    }

  template <typename C, typename T>
    typename basic_pstreambuf<C,T>::int_type
    basic_pstreambuf<C,T>::pbackfail(basic_pstreambuf<C,T>::int_type c)
    {
      if (!take_from_buf_)
      {
        if (traits_type::eq_int_type(c, traits_type::eof()))
          char_buf_ = traits_type::to_char_type(c); 
        take_from_buf_ = true;

        return traits_type::to_int_type(char_buf_);
      }
      else
      {
         return traits_type::eof();
      }
    }

  template <typename C, typename T>
    inline bool
    basic_pstreambuf<C,T>::read(basic_pstreambuf<C,T>::int_type& c)
    {
      char_type tmp = traits_type::to_char_type(c);
      if (file_ && (std::fread(&tmp, sizeof(char_type), 1, file_) == 1))
      {
        c = traits_type::to_int_type(tmp);
        return true;
      }
      else
      {
        return false;
      }
    }

  template <typename C, typename T>
    inline bool
    basic_pstreambuf<C,T>::write(basic_pstreambuf<C,T>::int_type c)
    {
      char_type tmp = traits_type::to_char_type(c);
      return (file_ && (std::fwrite(&tmp, sizeof(char_type), 1, file_) == 1));
    }

  // TODO extend openmode2str() to handle all modes and make non-member ?
  template <typename C, typename T>
    std::string
    basic_pstreambuf<C,T>::openmode2str(std::ios_base::openmode mode)
    {
      // restrict to the openmodes that pstreambuf honours
      mode &= (ios_base::in|ios_base::out);
      if (mode == ios_base::in)
        return "r";
      else if (mode == ios_base::out)
        return "w";
      else if (mode == (ios_base::in|ios_base::out))
        return "r+";
      else
        return "";
    }

#if REDI_PSTREAMS_POPEN_USES_BIDIRECTIONAL_PIPE
  template <typename CharT, typename Traits = std::char_traits<CharT> >
    class basic_pstream : public std::basic_iostream<CharT, Traits>
    {
      typedef std::basic_iostream<CharT, Traits>    iostream_type;
      typedef basic_pstreambuf<CharT, Traits>       streambuf_type;
      typedef streambuf_type::string                string;
      typedef streambuf_type::ios_base              ios_base;

    public:
      basic_ipstream()
      : iostream_type(NULL), command_(), buf_()
      { this->init(&buf_); }

      basic_ipstream(const string& command, ios_base::openmode mode = ios_base::in|ios_base::out)
      : iostream_type(NULL), command_(command), buf_()
      {
        this->init(&buf_);
        this->open(command_, mode);
      }

      ~basic_ipstream() { }

      bool
      is_open() const { return buf_.is_open(); }

      void
      open(const string& command, ios_base::openmode mode = ios_base::in|ios_base::out)
      { buf_.open((command_=command), mode); }

      void
      close() { buf_.close(); }

      const string&
      command() const { return command_; }

    protected:
      string command_;
      streambuf_type buf_;
    };

  typedef basic_pstream<char> pstream;

#endif

} // namespace redi

#elif GCC_BACK_COMPAT == 1
// gcc 2.7 / 2.8 / 2.9x / egcs
//#elif __GNUC__ == 2 && __GNUC_MINOR__ >= 7

#warning "PStreams needs an ISO C++ compliant compiler"

// turn on non-standard extensions
#define _STREAM_COMPAT 1

#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <string.h>

namespace redi
{
  class pstreambase : public fstreambase {
  public:
    void open(const char* command)
    { attach(fileno(popen(command, mode_))); errno=0; }

  protected:
    pstreambase(const char* mode) : fstreambase() { copy_mode(mode); }

    pstreambase(const char* command, const char* mode)
    : fstreambase(fileno(popen(command, mode))) { errno=0; copy_mode(mode); }

    virtual ~pstreambase()
    { if (FILE* fp = fdopen(filedesc(), mode_)) pclose(fp); }

  private:
    void copy_mode(const char* mode) { strncpy(mode_, mode, 2); }
    char mode_[3]; // "r", "w" or "r+"
  };
  // errno is set to zero because fstreambase tries to lseek()
  // to the end of the stream which is not allowed for pipes
  // and sets errno to ESPIPE, so we clear it.


  class ipstream : public pstreambase, public istream
  {
    static const char * const MODE = "r";
  public:
    ipstream() : pstreambase(MODE) { }
    ipstream(const char* name) : pstreambase(name, MODE) { }
  };

  class opstream : public pstreambase, public ostream
  {
    static const char * const MODE = "w";
  public:
    opstream() : pstreambase(MODE) { }
    opstream(const char* name) : pstreambase(MODE) { }
  };

#if REDI_PSTREAMS_POPEN_USES_BIDIRECTIONAL_PIPE
  class pstream :  protected pstreambase, public iostream {
    static const char * const MODE = "r+";
  public:
    pstream() : pstreambase() { }
    pstream(const char* command) : pstreambase(command, MODE) { }
  };
#endif

}  // namespace redi

#elif BACK_COMPAT == 1

#warning "PStreams needs an ISO C++ compliant compiler!"

#include <fstream>
#include <cstdio>
#include <cerrno>

  // very basic implementations
  // incomplete interface: no open(const char*)
  // use non-standard extensions
  // use at your own risk!

namespace redi
{

  class opstream : public std::ofstream {
    static const char MODE[] = "w";
  public:
    opstream(const char* command) : std::ofstream(fileno(popen(command, MODE)))
    { errno=0; }
    ~opstream() { pclose(fdopen(rdbuf()->fd(), MODE)); }
  };

  class ipstream : public std::ifstream {
    static const char MODE[] = "r";
  public:
    ipstream(const char* command) : std::ifstream(fileno(popen(command, MODE)))
    { errno=0; }
    ~ipstream() { pclose(fdopen(rdbuf()->fd(), MODE)); }
  };

#if REDI_PSTREAMS_POPEN_USES_BIDIRECTIONAL_PIPE
  class pstream : public std::fstream {
    static const char MODE[] = "r+";
  public:
    pstream(const char* command) : std::fstream(fileno(popen(command, MODE)))
    { errno=0; }
    ~pstream() { pclose(fdopen(rdbuf()->fd(), MODE)); }
  };
#endif

}  // namespace redi


#endif  // __GNUC__

#endif  // REDI_PSTREAM_H

