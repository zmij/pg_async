/* $Id: pstream.h,v 1.10 2002/01/07 20:28:38 redi Exp $
PStreams - POSIX Process I/O for C++
Copyright (C) 2001-2002 Jonathan Wakely

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

/**
 * @file pstream.h
 * @brief Declares all PStreams classes.
 * @author Jonathan Wakely
 *
 * Defines classes redi::ipstream, redi::opstream, and, conditionally,
 * redi::pstream.
 * Also defines utility function redi::openmode2str() to convert a
 * @c std::ios_base::openmode to a @c std::string.
 */

#ifndef REDI_PSTREAM_H
#define REDI_PSTREAM_H

/**
 * The value is @c ((major*0xff)+minor), where @c major is the number to
 * the left of the decimal point and @c minor is the number to the right.
 * (This gives a maximum of 256 minor revisions between major versions).
 * @brief The library version.
 */
#define PSTREAMS_VERSION 0x000c   // 0.12

// check whether to provide pstream
// popen() needs to use bidirectional pipe
#if ! defined(REDI_PSTREAMS_POPEN_USES_BIDIRECTIONAL_PIPE)
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
# define REDI_PSTREAMS_POPEN_USES_BIDIRECTIONAL_PIPE 1
#elif defined(__NetBSD_Version__) && __NetBSD_Version__ >= 0x01040000
# define REDI_PSTREAMS_POPEN_USES_BIDIRECTIONAL_PIPE 1
#endif
#endif

#if ! ( BACK_COMPAT == 1 || GCC_BACK_COMPAT == 1 )

#include <ios>
#include <streambuf>
#include <string>
#include <cstdio>

/// All PStreams classes are declared in namespace redi.
namespace redi
{
  /// Convert an ios_base::openmode to a string.
  std::string
  openmode2str(std::ios_base::openmode mode);
  // TODO extend openmode2str() to handle all openmode values

  // TODO move function definitions out of class body (and most doxygen stuff)

  /// Class template for stream buffer.
  /** Provides underlying streambuf functionality for the PStreams classes. */
  template <typename CharT, typename Traits>
    class basic_pstreambuf : public std::basic_streambuf<CharT, Traits>
    {
    public:
      /// Default constructor.
      /** Creates an uninitialised stream buffer. */
      basic_pstreambuf()
      : file_(0), take_from_buf_(false)
      { }

      /// Constructor that initialises the buffer.
      /**
       * Initialises the stream buffer by calling open() with the supplied
       * arguments.
       *
       * @param command a string containing a shell command.
       * @param mode the I/O mode to use when opening the pipe.
       * @see open()
       */
      basic_pstreambuf(const std::string& command, std::ios_base::openmode mode)
      : file_(0), take_from_buf_(false)
      { this->open(command, mode_); }

      /// Destructor.
      /**
       * Closes the stream by calling close().
       * @see close()
       */
      ~basic_pstreambuf() { this->close(); }

      /// Initialise the stream buffer.
      /**
       * Starts a new process by passing @a command to the shell
       * and opens a pipe to the process with the specified @a mode.
       * There is no way to tell whether the shell command succeeded, this
       * function will always succeed unless resource limits (such as
       * memory usage, or number of processes or open files) are exceeded.
       *
       * @param command a string containing a shell command.
       * @param mode the I/O mode to use when opening the pipe.
       * @return NULL if the shell could not be started or the
       * pipe could not be opened, pointer to self otherwise.
       */
      basic_pstreambuf*
      open(const std::string& command, std::ios_base::openmode mode)
      {
        file_ = ::popen(command.c_str(), openmode2str(mode).c_str());
        return is_open() ? this : NULL;
      }

      /// Close the stream buffer.
      /**
       * Waits for the associated process to finish and closes the pipe.
       * @return pointer to self.
       */
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

      /// Report whether the stream buffer has been initialised.
      /**
       * @return true if open() has been successfully called, false otherwise.
       * @warning Can't determine whether the command used to
       * initialise the buffer was successfully executed or not. If the
       * shell command failed this function will still return true.
       * @see open()
       */
      bool
      is_open() const { return bool(file_); }

    protected:
      /// Transfer characters to the pipe when character buffer overflows.
      int_type overflow(int_type c);

      /// Transfer characters from the pipe when the character buffer is empty.
      int_type uflow();

      /// Transfer characters from the pipe when the character buffer is empty.
      int_type underflow();

      /// Make a character available to be returned by the next extraction.
      int_type pbackfail(int_type c = traits_type::eof());

      /// Extract a character from the pipe.
      bool read(int_type& c);

      /// Insert a character into the pipe.
      bool write(int_type c);

    private:
      basic_pstreambuf(const basic_pstreambuf&);
      basic_pstreambuf& operator=(const basic_pstreambuf&);

      FILE* file_;
      char_type char_buf_;
      bool take_from_buf_;
    };

  /// Class template for Input PStreams.
  /**
   * Reading from an ipstream reads the command's standard output,
   * and the command's standard input is the same as that of the process
   * that created the object.
   */
  template <typename CharT, typename Traits = std::char_traits<CharT> >
    class basic_ipstream : public std::basic_istream<CharT, Traits>
    {
      typedef std::basic_istream<CharT, Traits>     istream_type;
      typedef basic_pstreambuf<CharT, Traits>       streambuf_type;

    public:
      /// Default constructor.
      /** Creates an uninitialised stream. */
      basic_ipstream()
      : istream_type(NULL), command_(), buf_()
      { this->init(&buf_); }

      /// Constructor that initialises the stream by starting a process.
      /**
       * Initialises the stream buffer by calling open() with the supplied
       * arguments.
       * @param command a string containing a shell command.
       * @param mode the I/O mode to use when opening the pipe.
       * @see open()
       */
      basic_ipstream(const std::string& command, std::ios_base::openmode mode = ios_base::in)
      : istream_type(NULL), command_(command), buf_()
      {
        this->init(&buf_);
        this->open(command_, mode);
      }

      /// Destructor
      ~basic_ipstream() { }

      /// Start a process.
      /**
       * Starts a new process by passing <em>command</em> to the shell
       * and opens a pipe to the process with the specified mode.
       * @param command a string containing a shell command.
       * @param mode the I/O mode to use when opening the pipe.
       * @see basic_pstreambuf::open()
       */
      void
      open(const std::string& command, std::ios_base::openmode mode = std::ios_base::in)
      { buf_.open((command_=command), mode); }

      /// Close the pipe.
      /** Waits for the associated process to finish and closes the pipe. */
      void
      close() { buf_.close(); }

      /// Report whether the stream's buffer has been initialised.
      /**
       * @return true if open() has been successfully called, false otherwise.
       * @see basic_pstreambuf::open()
       */
      bool
      is_open() const { return buf_.is_open(); }

      /// Return the command used to initialise the stream.
      /**
       * @return a string containing the command used to initialise the stream.
       */
      const std::string&
      command() const { return command_; }

    private:
      std::string command_;
      streambuf_type buf_;
    };

  /// Class template for Output PStreams.
  /**
   * Writing to an open opstream writes to the standard input of the command;
   * the command's standard output is the same as that of the process that
   * created the pstream object, unless this is altered by the command itself.
   */
  template <typename CharT, typename Traits = std::char_traits<CharT> >
    class basic_opstream : public std::basic_ostream<CharT, Traits>
    {
      typedef std::basic_ostream<CharT, Traits>     ostream_type;
      typedef basic_pstreambuf<CharT, Traits>       streambuf_type;

    public:
      /**
       * @brief Default constructor.
       * Creates an uninitialised stream.
       */
      basic_opstream()
      : ostream_type(NULL), command_(), buf_()
      { this->init(&buf_); }

      /**
       * @brief Constructor that initialises the stream by starting a process.
       * @param command a string containing a shell command.
       * @param mode the I/O mode to use when opening the pipe.
       * @see open()
       *
       * Initialises the stream buffer by calling open() with the supplied
       * arguments.
       */
      basic_opstream(const std::string& command, std::ios_base::openmode mode = std::ios_base::out)
      : ostream_type(NULL), command_(command), buf_()
      {
        this->init(&buf_);
        this->open(command_, mode);
      }

      /// Destructor
      ~basic_opstream() { }

      /**
       * @brief Start a process.
       * @param command a string containing a shell command.
       * @param mode the I/O mode to use when opening the pipe.
       * @see basic_pstreambuf::open()
       *
       * Starts a new process by passing @a command to the shell
       * and opens a pipe to the process with the specified @a mode.
       */
      void
      open(const std::string& command, std::ios_base::openmode mode = std::ios_base::out)
      { buf_.open((command_=command), mode); }

      /**
       * @brief Close the pipe.
       * Waits for the associated process to finish and closes the pipe.
       */
      void
      close() { buf_.close(); }

      /**
       * @brief Report whether the stream's buffer has been initialised.
       * @see basic_pstreambuf::open()
       * @return true if open() has been successfully called, false otherwise.
       */
      bool
      is_open() const { return buf_.is_open(); }

      /**
       * @brief Return the command used to initialise the stream.
       * @return a string containing the command used to initialise the stream.
       */
      const std::string&
      command() const { return command_; }

    private:
      std::string command_;
      streambuf_type buf_;
    };

  /// Type definition for the most common template specialisation.
  typedef basic_ipstream<char> ipstream;
  /// Type definition for the most common template specialisation.
  typedef basic_opstream<char> opstream;

  // member definitions for pstreambuf

  /**
   * Called when the internal character buffer is not present or is full,
   * to transfer the buffer contents to the pipe. For unbuffered streams
   * this is called for every insertion.
   *
   * @param c a character to be written to the pipe
   * @return @c traits_type::not_eof(c) if @a c is equal to @c
   * traits_type::eof(). Otherwise returns @a c if @a c can be written
   * to the pipe, or @c traits_type::eof() if not.
   */
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

  /**
   * Called when the internal character buffer is not present or is empty,
   * to re-fill the buffer from the pipe. Behaves like underflow() but also
   * increments the next pointer to the get area, so repeated calls will
   * return a different character each time.
   *
   * @return The next available character in the buffer,
   * or @c traits_type::eof() in case of failure.
   * @see underflow()
   */
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

  /**
   * Called when the internal character buffer is not present or is empty,
   * to re-fill the buffer from the pipe. For unbuffered streams this is
   * called for every extraction.
   *
   * @return The first available character in the buffer,
   * or @c traits_type::eof() in case of failure.
   * @see uflow()
   */
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

  /**
   * Attempts to make @a c available as the next character to be read by
   * @c this->sgetc(), or if @a c is equal to @c traits_type::eof() then
   * the previous character in the sequence is made the next available ???
   *
   * @param c a character to make available for extraction.
   * @return @a c if the character can be made available, @c traits_type::eof()
   * otherwise.
   */
  template <typename C, typename T>
    typename basic_pstreambuf<C,T>::int_type
    basic_pstreambuf<C,T>::pbackfail(basic_pstreambuf<C,T>::int_type c)
    {
      if (!take_from_buf_)
      {
        if (!traits_type::eq_int_type(c, traits_type::eof()))
          char_buf_ = traits_type::to_char_type(c); 
        take_from_buf_ = true;
        return traits_type::to_int_type(char_buf_);
      }
      else
      {
         return traits_type::eof();
      }
    }

  /**
   * Attempts to extract a character from the pipe and store it in @a c.
   * Used by underflow().
   *
   * @param c a reference to hold the extracted character.
   * @return true if a character could be extracted, false otherwise.
   */
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

  /**
   * Attempts to insert @a c into the pipe. Used by overflow().
   *
   * @param c a character to insert.
   * @return true if the character could be inserted, false otherwise.
   */
  template <typename C, typename T>
    inline bool
    basic_pstreambuf<C,T>::write(basic_pstreambuf<C,T>::int_type c)
    {
      char_type tmp = traits_type::to_char_type(c);
      return (file_ && (std::fwrite(&tmp, sizeof(char_type), 1, file_) == 1));
    }

  /**
   * @a mode is masked with @c (ios_base::in|ios_base::out)
   * and converted to a string containing a stdio-style mode.
   *
   * @param mode an I/O mode to convert.
   * @return one of "r", "w", "r+" or "" as a string.
   */
  std::string
  openmode2str(std::ios_base::openmode mode)
  {
    using std::ios_base;
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

  /// Class template for Bidirectional PStreams.
  /**
   * The pstream class for both reading and writing to a process is only
   * available on some sytems, where popen(3) is implemented using a
   * bidirectional pipe.
   */
  template <typename CharT, typename Traits = std::char_traits<CharT> >
    class basic_pstream : public std::basic_iostream<CharT, Traits>
    {
      typedef std::basic_iostream<CharT, Traits>    iostream_type;
      typedef basic_pstreambuf<CharT, Traits>       streambuf_type;
      typedef streambuf_type::string                string;
      typedef streambuf_type::ios_base              ios_base;

    public:
      /**
       * @brief Default constructor.
       * Creates an uninitialised stream.
       */
      basic_pstream()
      : iostream_type(NULL), command_(), buf_()
      { this->init(&buf_); }

      /**
       * @brief Constructor that initialises the stream by starting a process.
       * Initialises the stream buffer by calling open() with the supplied
       * arguments.
       * @param command a string containing a shell command.
       * @param mode the I/O mode to use when opening the pipe.
       * @see open()
       */
      basic_pstream(const string& command, ios_base::openmode mode = ios_base::in|ios_base::out)
      : iostream_type(NULL), command_(command), buf_()
      {
        this->init(&buf_);
        this->open(command_, mode);
      }

      /// Destructor
      ~basic_pstream() { }

      /**
       * @brief Start a process.
       * Starts a new process by passing @a command to the shell
       * and opens a pipe to the process with the specified @a mode.
       * @param command a string containing a shell command.
       * @param mode the I/O mode to use when opening the pipe.
       * @see basic_pstreambuf::open()
       */
      void
      open(const string& command, ios_base::openmode mode = ios_base::in|ios_base::out)
      { buf_.open((command_=command), mode); }

      /**
       * @brief Close the pipe.
       * Waits for the associated process to finish and closes the pipe.
       */
      void
      close() { buf_.close(); }

      /**
       * @brief Report whether the stream's buffer has been initialised.
       * @see basic_pstreambuf::open()
       * @return true if open() has been successfully called, false otherwise.
       */
      bool
      is_open() const { return buf_.is_open(); }

      /**
       * @brief Return the command used to initialise the stream.
       * @return a string containing the command used to initialise the stream.
       */
      const string&
      command() const { return command_; }

    private:
      string command_;
      streambuf_type buf_;
    };

  /// Type definition for the most common template specialisation.
  typedef basic_pstream<char> pstream;

#endif

} // namespace redi


/**
 * @mainpage
 * @htmlinclude pstreams.html
 */
// TODO don't use @htmlinclude here, 

#elif GCC_BACK_COMPAT == 1
// gcc 2.7 / 2.8 / 2.9x / egcs
//#elif __GNUC__ == 2 && __GNUC_MINOR__ >= 7

#warning "PStreams needs an ISO C++ compliant compiler."
#warning "These classes are unsupported and largely untested."

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
#warning "These classes are unsupported and untested."

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

