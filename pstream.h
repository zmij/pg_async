/* $Id: pstream.h,v 1.31 2002/04/29 23:44:52 redi Exp $
PStreams - POSIX Process I/O for C++
Copyright (C) 2001,2002 Jonathan Wakely

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
 * Defines classes redi::ipstream, redi::opstream, redi::pstream
 * and, conditionally, redi::rpstream.
 */

#ifndef REDI_PSTREAM_H
#define REDI_PSTREAM_H

#include <ios>
#include <streambuf>
#include <istream>
#include <ostream>
#include <iostream>
#include <string>
#include <vector>
#include <cerrno>
#include <cstring>
#include <sys/types.h>  // for pid_t
#include <sys/wait.h>   // for waitpid()
#include <unistd.h>     // for pipe() fork() exec() and filedes functions


// TODO add buffering to pstreambuf


/// The library version.
#define PSTREAMS_VERSION 0x0023   // 0.35

/**
 *  @namespace redi
 *  @brief  All PStreams classes are declared in namespace redi.
 *
 *  Like the standard IOStreams, PStreams is a set of class templates,
 *  taking a character type and traits type, but as with the standard
 *  streams they are most likely to be used with @c char and the default
 *  traits type, so typedefs for this most common case are provided.
 *
 *  The @c pstream_base class template is not intended to be used directly,
 *  it is used internally to provide the common functionality for the
 *  other stream classes.
 */
namespace redi
{
  /// Class template for stream buffer.
  template <typename CharT, typename Traits>
    class basic_pstreambuf : public std::basic_streambuf<CharT, Traits>
    {
    public:
      // Type definitions for dependent types
      typedef CharT                             char_type;
      typedef Traits                            traits_type;
      typedef typename traits_type::int_type    int_type;
      typedef typename traits_type::off_type    off_type;
      typedef typename traits_type::pos_type    pos_type;
      /// Type used for file descriptors
      typedef int                               fd_t;
      /// Type used to specify how to connect to the process
      typedef std::ios_base::openmode           pmode;

      static const pmode pstdin  = std::ios_base::out; ///< Write to stdin
      static const pmode pstdout = std::ios_base::in;  ///< Read from stdout
      static const pmode pstderr = std::ios_base::app; ///< Read from stderr

      /// Default constructor.
      basic_pstreambuf();

      /// Constructor that initialises the buffer with @a command.
      basic_pstreambuf(const std::string& command, pmode mode);

      /// Constructor that initialises the buffer with @a file and @a argv..
      basic_pstreambuf(const std::string& file, const std::vector<std::string>& argv, pmode mode);

      /// Destructor.
      ~basic_pstreambuf();

      /// Initialise the stream buffer with @a command.
      basic_pstreambuf*
      open(const std::string& command, pmode mode);

      /// Initialise the stream buffer with @a file and @a argv.
      basic_pstreambuf*
      open(const std::string& file, const std::vector<std::string>& argv, pmode mode);

      /// Close the stream buffer.
      basic_pstreambuf*
      close();

      /// Change active input source.
      bool
      read_err(bool readerr = true);
      
      /// Report whether the stream buffer has been initialised.
      bool
      is_open() const;

#if REDI_EVISCERATE_PSTREAMS
      /// Obtain FILE pointers for each of the process' standard streams.
      size_t
      fopen(FILE*& in, FILE*& out, FILE*& err);
#endif

    protected:
      /// Transfer characters to the pipe when character buffer overflows.
      int_type
      overflow(int_type c);

      /// Transfer characters from the pipe when the character buffer is empty.
      int_type
      uflow();

      /// Transfer characters from the pipe when the character buffer is empty.
      int_type
      underflow();

      /// Make a character available to be returned by the next extraction.
      int_type
      pbackfail(int_type c = traits_type::eof());

      /// Insert a character into the pipe.
      bool
      write(char_type c);

      /// Extract a character from the pipe.
      bool
      read(char_type& c);

#if 0
      /// Insert a sequence of characters into the pipe.
      std::streamsize
      write(char_type* s, std::streamsize n);

      /// Extract a sequence of characters from the pipe.
      std::streamsize
      read(char_type* s, std::streamsize n);
#endif

    protected:
      /// Enumerated type to indicate whether stdout or stderr is to be read.
      enum buf_read_src { rsrc_out = 0, rsrc_err = 1 };

      /// Initialise pipes and fork process.
      pid_t
      fork(pmode mode);

      /// Return the file descriptor for the output pipe;
      fd_t&
      wpipe();

      /// Return the file descriptor for the active input pipe
      fd_t&
      rpipe();

      /// Return the file descriptor for the specified input pipe.
      fd_t&
      rpipe(enum buf_read_src which);

      /// Return the state of the active input buffer.
      bool&
      take_from_buf();

      /// Return the character buffer for the active input pipe.
      char_type&
      char_buf();

      /// Close an array of file descriptors.
      static void
      close_fd_array(fd_t* filedes, size_t count);

    private:
      basic_pstreambuf(const basic_pstreambuf&);
      basic_pstreambuf& operator=(const basic_pstreambuf&);

      pid_t         ppid_;        // pid of process
      fd_t          wpipe_;       // pipe used to write to process' stdin
      fd_t          rpipe_[2];    // two pipes to read from, stdout and stderr
      char_type     char_buf_[2];
      bool          take_from_buf_[2];
      /// Index into rpipe_[] to indicate active source for read operations
      buf_read_src   rsrc_;
    };

  /// Class template for common base class.
  template <typename CharT, typename Traits = std::char_traits<CharT> >
    class pstream_base : virtual public std::basic_ios<CharT, Traits>
    {
      typedef basic_pstreambuf<CharT, Traits>       streambuf_type;

    public:
      /// Type used to specify how to connect to the process
      typedef typename streambuf_type::pmode        pmode;

      /// Default constructor.
      pstream_base();

      /// Constructor that initialises the stream by starting a process.
      pstream_base(const std::string& command, pmode mode);

      /// Constructor that initialises the stream by starting a process.
      pstream_base(const std::string& file, const std::vector<std::string>& argv, pmode mode);

      /// Pure virtual destructor
      virtual
      ~pstream_base() = 0;

      /// Start a process.
      virtual void
      open(const std::string& command, pmode mode);

      /// Start a process.
      virtual void
      open(const std::string& file, const std::vector<std::string>& argv, pmode mode);

      /// Close the pipe.
      void
      close();

      /// Report whether the stream's buffer has been initialised.
      bool
      is_open() const;

      /// Return the command used to initialise the stream.
      const std::string&
      command() const;

#if REDI_EVISCERATE_PSTREAMS
      /// Obtain FILE pointers for each of the process' standard streams.
      size_t
      fopen(FILE*& in, FILE*& out, FILE*& err);
#endif

    protected:
      std::string       command_; ///< The command used to start the process.
      streambuf_type    buf_;     ///< The stream buffer.
    };


  /**
   * @class basic_ipstream
   * @brief Class template for Input PStreams.
   *
   * Reading from an ipstream reads the command's standard output and/or
   * standard error (depending on how the ipstream is opened)
   * and the command's standard input is the same as that of the process
   * that created the object, unless altered by the command itself.
   */

  template <typename CharT, typename Traits = std::char_traits<CharT> >
    class basic_ipstream
    : public std::basic_istream<CharT, Traits>
    , public pstream_base<CharT, Traits>
    {
      typedef std::basic_istream<CharT, Traits>     istream_type;
      typedef pstream_base<CharT, Traits>           pbase_type;
      typedef typename pbase_type::streambuf_type   streambuf_type;

    public:
      /// Type used to specify how to connect to the process
      typedef typename pbase_type::pmode            pmode;

      /// Default constructor, creates an uninitialised stream.
      basic_ipstream()
      : istream_type(NULL), pbase_type()
      {}

      /**
       * @brief Constructor that initialises the stream by starting a process.
       *
       * Initialises the stream buffer by calling open() with the supplied
       * arguments.
       *
       * @param command a string containing a shell command.
       * @param mode the I/O mode to use when opening the pipe.
       * @see open()
       */
      basic_ipstream(const std::string& command, pmode mode = std::ios_base::in)
      : istream_type(NULL), pbase_type(command, mode)
      {}

      /**
       * @brief Constructor that initialises the stream by starting a process.
       *
       * Initialises the stream buffer by calling open() with the supplied
       * arguments.
       *
       * @param file a string containing the pathname of a program to execute.
       * @param argv a vector of argument strings passed to the new program.
       * @param mode the I/O mode to use when opening the pipe.
       * @see open()
       */
      basic_ipstream(const std::string& file, const std::vector<std::string>& argv, pmode mode = std::ios_base::in)
      : istream_type(NULL), pbase_type(file, argv, mode)
      {}

      /**
       * @brief Destructor
       *
       * Closes the stream and waits for the child to exit.
       */
      ~basic_ipstream()
      {}

      /**
       * @brief Start a process.
       *
       * Starts a new process by passing @a command to the shell
       * and opens a pipe to the process with the specified @a mode.
       *
       * @param command a string containing a shell command.
       * @param mode the I/O mode to use when opening the pipe.
       * @see pstream_base::open()
       */
      void
      open(const std::string& command, pmode mode = std::ios_base::in)
      { pbase_type::open(command, mode); }

      /**
       * @brief Start a process.
       *
       * Starts a new process by executing @a file with the arguments in
       * @a argv and opens pipes to the process as given by @a mode.
       *
       * @param file a string containing the pathname of a program to execute.
       * @param argv a vector of argument strings passed to the new program.
       * @param mode the I/O mode to use when opening the pipe.
       * @see pstream_base::open()
       */
      void
      open(const std::string& file, const std::vector<std::string>& argv, pmode mode = std::ios_base::in)
      { pbase_type::open(file, argv, mode); }
    };


  /**
   * @class basic_opstream
   * @brief Class template for Output PStreams.
   * 
   * Writing to an open opstream writes to the standard input of the command;
   * the command's standard output is the same as that of the process that
   * created the pstream object, unless altered by the command itself.
   */

  template <typename CharT, typename Traits = std::char_traits<CharT> >
    class basic_opstream
    : public std::basic_ostream<CharT, Traits>
    , public pstream_base<CharT, Traits>
    {
      typedef std::basic_ostream<CharT, Traits>     ostream_type;
      typedef pstream_base<CharT, Traits>           pbase_type;
      typedef typename pbase_type::streambuf_type   streambuf_type;

    public:
      /// Type used to specify how to connect to the process
      typedef typename pbase_type::pmode            pmode;

      /// Default constructor, creates an uninitialised stream.
      basic_opstream()
      : ostream_type(NULL), pbase_type()
      {}

      /**
       * @brief Constructor that initialises the stream by starting a process.
       *
       * Initialises the stream buffer by calling open() with the supplied
       * arguments.
       *
       * @param command a string containing a shell command.
       * @param mode the I/O mode to use when opening the pipe.
       * @see open()
       */
      basic_opstream(const std::string& command, pmode mode = std::ios_base::out)
      : ostream_type(NULL), pbase_type(command, mode)
      {}

      /**
       * @brief Constructor that initialises the stream by starting a process.
       *
       * Initialises the stream buffer by calling open() with the supplied
       * arguments.
       *
       * @param file a string containing the pathname of a program to execute.
       * @param argv a vector of argument strings passed to the new program.
       * @param mode the I/O mode to use when opening the pipe.
       * @see open()
       */
      basic_opstream(const std::string& file, const std::vector<std::string>& argv, pmode mode = std::ios_base::out)
      : ostream_type(NULL), pbase_type(file, argv, mode)
      {}

      /**
       * @brief Destructor
       *
       * Closes the stream and waits for the child to exit.
       */
      ~basic_opstream() { }

      /**
       * @brief Start a process.
       * @param command a string containing a shell command.
       * @param mode the I/O mode to use when opening the pipe.
       * @see pstream_base::open(const std::string&, pmode)
       */
      void
      open(const std::string& command, pmode mode = std::ios_base::out)
      { pbase_type::open(command, mode); }

      /**
       * @brief Start a process.
       * @param file a string containing the pathname of a program to execute.
       * @param argv a vector of argument strings passed to the new program.
       * @param mode the I/O mode to use when opening the pipe.
       * @see pstream_base::open(const std::string&, const std::vector<std::string>&, pmode)
       */
      void
      open(const std::string& file, const std::vector<std::string>& argv, pmode mode = std::ios_base::out)
      { pbase_type::open(file, argv, mode); }

      /**
       * @brief Set streambuf to read from process' @c stdout.
       * @return @c *this
       */
      basic_opstream&
      out()
      {
        buf_.read_err(false);
        return *this;
      }

      /**
       * @brief Set streambuf to read from process' @c stderr.
       * @return @c *this
       */
      basic_opstream&
      err()
      {
        buf_.read_err(true);
        return *this;
      }
    };


  /**
   * @class basic_pstream
   * @brief Class template for Bidirectional PStreams.
   *
   * Writing to a pstream opened with @c pmode @c pstdin writes to the
   * standard input of the command.
   * Reading from a pstream opened with @c pmode @c pstdout and/or @c pstderr
   * reads the command's standard output and/or standard error.
   * Any of the process' @c stdin, @c stdout or @c stderr that is not
   * connected to the pstream (as specified by the @c pmode)
   * will be the same as the process that created the pstream object,
   * unless altered by the command itself.
   */
  template <typename CharT, typename Traits = std::char_traits<CharT> >
    class basic_pstream
    : public std::basic_iostream<CharT, Traits>
    , public pstream_base<CharT, Traits>
    {
      typedef std::basic_iostream<CharT, Traits>    iostream_type;
      typedef pstream_base<CharT, Traits>           pbase_type;
      typedef typename pbase_type::streambuf_type   streambuf_type;

    public:
      /// Type used to specify how to connect to the process
      typedef typename pbase_type::pmode            pmode;

      /// Default constructor, creates an uninitialised stream.
      basic_pstream()
      : iostream_type(NULL), pbase_type()
      {}

      /**
       * @brief Constructor that initialises the stream by starting a process.
       *
       * Initialises the stream buffer by calling open() with the supplied
       * arguments.
       *
       * @param command a string containing a shell command.
       * @param mode the I/O mode to use when opening the pipe.
       * @see open(const std::string&, pmode)
       */
      basic_pstream(const std::string& command, pmode mode = std::ios_base::in|std::ios_base::out)
      : iostream_type(NULL), pbase_type(command, mode)
      {}

      /**
       * @brief Constructor that initialises the stream by starting a process.
       *
       * Initialises the stream buffer by calling open() with the supplied
       * arguments.
       *
       * @param file a string containing the pathname of a program to execute.
       * @param argv a vector of argument strings passed to the new program.
       * @param mode the I/O mode to use when opening the pipe.
       * @see open(const std::string&, const std::vector<std::string>&, pmode)
       */
      basic_pstream(const std::string& file, const std::vector<std::string>& argv, pmode mode = std::ios_base::in|std::ios_base::out)
      : iostream_type(NULL), pbase_type(command, argv, mode)
      {}

      /**
       * @brief Destructor
       *
       * Closes the stream and waits for the child to exit.
       */
      ~basic_pstream() { }

      /**
       * @brief Start a process.
       * @param command a string containing a shell command.
       * @param mode the I/O mode to use when opening the pipe.
       * @see pstream_base::open(const std::string&, pmode)
       */
      void
      open(const std::string& command, pmode mode = std::ios_base::in|std::ios_base::out)
      { pbase_type::open(command, mode); }

      /**
       * @brief Start a process.
       * @param file a string containing the pathname of a program to execute.
       * @param argv a vector of argument strings passed to the new program.
       * @param mode the I/O mode to use when opening the pipe.
       * @see pstream_base::open(const std::string&, const std::vector<std::string>&, pmode)
       */
      void
      open(const std::string& file, const std::vector<std::string>& argv, pmode mode = std::ios_base::in|std::ios_base::out)
      { pbase_type::open(file, argv, mode); }

      /**
       * @brief Set streambuf to read from process' @c stdout.
       * @return @c *this
       */
      basic_pstream&
      out()
      {
        buf_.read_err(false);
        return *this;
      }

      /**
       * @brief Set streambuf to read from process' @c stderr.
       * @return @c *this
       */
      basic_pstream&
      err()
      {
        buf_.read_err(true);
        return *this;
      }
    };


  /// Type definition for common template specialisation.
  typedef basic_pstreambuf<char, std::char_traits<char> > pstreambuf;
  /// Type definition for common template specialisation.
  typedef basic_ipstream<char> ipstream;
  /// Type definition for common template specialisation.
  typedef basic_opstream<char> opstream;
  /// Type definition for common template specialisation.
  typedef basic_pstream<char> pstream;


  /*
   * member definitions for pstreambuf
   */

  /**
   * @class basic_pstreambuf
   * Provides underlying streambuf functionality for the PStreams classes.
   */

  /** Creates an uninitialised stream buffer. */
  template <typename C, typename T>
    inline
    basic_pstreambuf<C,T>::basic_pstreambuf()
    : ppid_(0)
    , wpipe_(-1)
    , rsrc_(rsrc_out)
    {
      rpipe_[rsrc_out] = rpipe_[rsrc_err] = -1;
      take_from_buf_[rsrc_out] = take_from_buf_[rsrc_err] = false;
    }

  /**
   * Initialises the stream buffer by calling open() with the supplied
   * arguments.
   *
   * @param command a string containing a shell command.
   * @param mode the I/O mode to use when opening the pipe.
   * @see open()
   */
  template <typename C, typename T>
    inline
    basic_pstreambuf<C,T>::basic_pstreambuf(const std::string& command, pmode mode)
    : ppid_(0)
    , wpipe_(-1)
    , rsrc_(rsrc_out)
    {
      rpipe_[rsrc_out] = rpipe_[rsrc_err] = -1;
      take_from_buf_[rsrc_out] = take_from_buf_[rsrc_err] = false;
      this->open(command, mode);
    }

  /**
   * Initialises the stream buffer by calling open() with the supplied
   * arguments.
   *
   * @param file a string containing the name of a program to execute.
   * @param argv a vector of argument strings passsed to the new program.
   * @param mode the I/O mode to use when opening the pipe.
   * @see open()
   */
  template <typename C, typename T>
    inline
    basic_pstreambuf<C,T>::basic_pstreambuf(const std::string& file, const std::vector<std::string>& argv, pmode mode)
    : ppid_(0)
    , wpipe_(-1)
    , rsrc_(rsrc_out)
    {
      rpipe_[rsrc_out] = rpipe_[rsrc_err] = -1;
      take_from_buf_[rsrc_out] = take_from_buf_[rsrc_err] = false;
      this->open(file, argv, mode);
    }

  /**
   * Closes the stream by calling close().
   * @see close()
   */
  template <typename C, typename T>
    inline
    basic_pstreambuf<C,T>::~basic_pstreambuf()
    {
      this->close();
    }

  /**
   * Starts a new process by passing @a command to the shell
   * and opens pipes to the process with the specified @a mode.
   * There is no way to tell whether the shell command succeeded, this
   * function will always succeed unless resource limits (such as
   * memory usage, or number of processes or open files) are exceeded.
   *
   * @param command a string containing a shell command.
   * @param mode a bitwise OR of one or more of @c out, @c in and @c err.
   * @return NULL if the shell could not be started or the
   * pipes could not be opened, pointer to self otherwise.
   */
  template <typename C, typename T>
    inline basic_pstreambuf<C,T>*
    basic_pstreambuf<C,T>::open(const std::string& command, pmode mode)
    {
      basic_pstreambuf<C,T>* ret = NULL;

      if (!this->is_open())
      {
        switch(this->fork(mode))
        {
          case 0 :
          {
            // this is the new process, exec command
            ::execlp("sh", "sh", "-c", command.c_str(), 0);

            // can only reach this point if exec() failed
            int error = errno;
            // TODO use exceptions not cerr, don't #include iostream & cstring
            std::cerr << "sh: " << std::strerror(error) << '\n';

            // parent can get exit code from waitpid()
            std::exit(error);
          }
          case -1 :
          {
            // couldn't fork, error already handled
            break;
          }
          default :
          {
            // this is the parent process
            if (this->is_open()) // should be true now
              ret = this;
          }
        }
      }
      return ret;
    }


  /**
   * Starts a new process by executing @a file with the arguments in
   * @a argv and opens pipes to the process with the specified @a mode.
   * By convention argv[0] should be the file name of the file being executed.
   * Will duplicate the actions of  the  shell  in searching for an
   * executable file if the specified file name does not contain a slash (/)
   * character.
   *
   * @param file a string containing the pathname of a program to execute.
   * @param argv a vector of argument strings passed to the new program.
   * @param mode a bitwise OR of one or more of @c out, @c in and @c err.
   * @return NULL if the program could not be executed or the
   * pipes could not be opened, pointer to self otherwise.
   * @see execvp()
   */
  template <typename C, typename T>
    inline basic_pstreambuf<C,T>*
    basic_pstreambuf<C,T>::open(const std::string& file, const std::vector<std::string>& argv, pmode mode)
    {
      basic_pstreambuf<C,T>* ret = NULL;

      if (!this->is_open())
      {
        switch(this->fork(mode))
        {
          case 0 :
          {
            // this is the new process, exec command

            char** arg_v = new char*[argv.size()+1];
            for (size_t i = 0; i < argv.size(); ++i)
            {
#if 0
              arg_v[i] = new char[argv[i].size()+1];
              argv[i].copy(arg_v[i], std::string::npos);
              arg_v[i][argv[i].size()] = 0;
#else
              arg_v[i] = const_cast<char*>(argv[i].c_str());
#endif
            }
            arg_v[argv.size()] = 0;

            ::execvp(file.c_str(), arg_v);

            // can only reach this point if exec() failed
            int error = errno;
            // TODO use exceptions not cerr, don't #include iostream & cstring
            std::cerr << file << ": " << std::strerror(error) << '\n';

            // parent can get exit code from waitpid()
            std::exit(error);
          }
          case -1 :
          {
            // couldn't fork, error already handled
            break;
          }
          default :
          {
            // this is the parent process
            if (this->is_open()) // should be true now
              ret = this;
          }
        }
      }
      return ret;
    }

  /**
   * Creates pipes as specified by @a mode and calls @c fork() to create
   * a new process. If the fork is successful the parent process stores
   * the child's PID and the opened pipes and the child process replaces
   * its standard streams with the opeoened pipes.
   *
   * @param mode a pmode specifying which of the child's standard streams
   * to connect to.
   * @return -1 on failure, otherwise the PID of the child in the parent's
   * context, 0 in the child's context.
   */
  template <typename C, typename T>
    pid_t
    basic_pstreambuf<C,T>::fork(pmode mode)
    {
      pid_t pid = -1;

      // three pairs of file descriptors, for pipes connected to the
      // process' stdin, stdout and stderr
      // (stored in a single array so close_fd_array() can close all at once)
      fd_t fd[6] =  {-1, -1, -1, -1, -1, -1};
      fd_t* pin = fd;
      fd_t* pout = fd+2;
      fd_t* perr = fd+4;

      // constants for read/write ends of pipe
      const int RD = 0;
      const int WR = 1;

      // N.B.
      // For the pstreambuf pin is an output stream and
      // pout and perr are input streams.

      if ( (mode&pstdin && ::pipe(pin)==0)
          || (mode&pstdout && ::pipe(pout)==0)
          || (mode&pstderr && ::pipe(perr)==0) )
      {
        pid = ::fork();
        int error = errno;
        switch (pid)
        {
          case 0 :
          {
            // this is the new process

            // for each open pipe close one end and redirect the
            // respective standard stream to the other end

            if (*pin >= 0)
            {
              ::close(pin[WR]);
              ::dup2(pin[RD], STDIN_FILENO)>=0;
            }
            if (*pout >= 0)
            {
              ::close(pout[RD]);
              ::dup2(pout[WR], STDOUT_FILENO);
            }
            if (*perr >= 0)
            {
              ::close(perr[RD]);
              ::dup2(perr[WR], STDERR_FILENO);
            }
            break;
          }
          case -1 :
          {
            // TODO use exceptions not cerr, don't #include iostream & cstring
            std::cerr << "Cannot fork: " << std::strerror(error) << '\n';
            // couldn't fork for some reason, close any open pipes
            basic_pstreambuf<C,T>::close_fd_array(fd, 6);
            break;
          }
          default :
          {
            // this is the parent process, store process' pid
            ppid_ = pid;

            // store one end of open pipes and close other end
            if (*pin >= 0)
            {
              wpipe_ = pin[WR];
              ::close(pin[RD]);
            }
            if (*pout >= 0)
            {
              rpipe_[rsrc_out] = pout[RD];
              ::close(pout[WR]);
            }
            if (*perr >= 0)
            {
              rpipe_[rsrc_err] = perr[RD];
              ::close(perr[WR]);
            }

            if (rpipe_[rsrc_out] == -1 && rpipe_[rsrc_err] >= 0)
            {
              // reading stderr but not stdout, so use stderr for all reads
              this->read_err(true);
            }
          }
        }
      }
      else
      {
        //int error = errno;
        // TODO report error to cerr ?

        // close any pipes we opened before failure
        basic_pstreambuf<C,T>::close_fd_array(fd, 6);
      }
      return pid;
    }

  /**
   * Waits for the associated process to finish and closes the pipe.
   * @return pointer to self or @c NULL if @c pclose() fails.
   */
  template <typename C, typename T>
    inline basic_pstreambuf<C,T>*
    basic_pstreambuf<C,T>::close()
    {
      basic_pstreambuf<C,T>* ret = NULL;
      if (this->is_open())
      {
        basic_pstreambuf<C,T>::close_fd_array(&wpipe_, 1);
        basic_pstreambuf<C,T>::close_fd_array(rpipe_, 2);
        wpipe_ = rpipe_[rsrc_out] = rpipe_[rsrc_err] = -1;

        int status;
        if (::waitpid(ppid_, &status, WNOHANG) == ppid_)
        {
          ppid_ = 0;
          ret = this;
        }
        // TODO handle errors from waitpid()
        // int exit_status = WEXITSTATUS(status);
      }
      return ret;
    }

  /**
   * @return true if open() has been successfully called, false otherwise.
   * @warning Can't determine whether the command used to
   * initialise the buffer was successfully executed or not. If the
   * shell command failed this function will still return true.
   * @see open()
   */
  template <typename C, typename T>
    inline bool
    basic_pstreambuf<C,T>::is_open() const
    {
      return bool(ppid_>0);
      //return (wpipe_>=0 || rpipe_[rsrc_out]>=0 || rpipe_[rsrc_err]>=0);
    }

  /**
   * Toggle the stream used for reading. If @a readerr is @c true then the
   * process' @c stderr output will be used for subsequent extractions, if
   * @a readerr is false the the process' stdout will be used.
   * @param readerr @c true to read @c stderr, @c false to read @c stdout.
   * @return @c true if the requested stream is open and will be used for
   * subsequent extractions, @c false otherwise.
   */
  template <typename C, typename T>
    inline bool
    basic_pstreambuf<C,T>::read_err(bool readerr)
    {
      bool ret = false;
      if (readerr)
      {
        if (rpipe_[rsrc_err]>=0)
        {
          rsrc_ = rsrc_err;
          ret = true;
        }
      }
      else
      {
        if (rpipe_[rsrc_out]>=0)
        {
          rsrc_ = rsrc_out;
          ret = true;
        }
      }
      return ret;
    }

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
        if (!this->write(traits_type::to_char_type(c)))
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
      if (take_from_buf())
      {
        take_from_buf() = false;
        return traits_type::to_int_type(char_buf());
      }
      else
      {
        char_type c;

        if (!this->read(c))
          return traits_type::eof();
        else
        {
          char_buf() = c;
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
      if (take_from_buf())
      {
        return traits_type::to_int_type(char_buf());
      }
      else
      {
        char_type c;

        if (!this->read(c))
          return traits_type::eof();
        else
        {
          take_from_buf() = true;
          char_buf() = c;
          return traits_type::to_int_type(c);
        }
      }
    }

  /**
   * Attempts to make @a c available as the next character to be read by
   * @c this->sgetc(), or if @a c is equal to @c traits_type::eof() then
   * the previous character in the sequence is made the next available
   * (at least, I think that's the intention?!)
   *
   * @param c a character to make available for extraction.
   * @return @a c if the character can be made available, @c traits_type::eof()
   * otherwise.
   */
  template <typename C, typename T>
    typename basic_pstreambuf<C,T>::int_type
    basic_pstreambuf<C,T>::pbackfail(int_type c)
    {
      if (!take_from_buf())
      {
        if (!traits_type::eq_int_type(c, traits_type::eof()))
        {
          char_buf() = traits_type::to_char_type(c); 
        }
        take_from_buf() = true;
        return traits_type::to_int_type(char_buf());
      }
      else
      {
         return traits_type::eof();
      }
    }

  /**
   * Attempts to insert @a c into the pipe. Used by overflow().
   * This currently only works for fixed width character encodings where
   * each character uses sizeof(char_type) bytes.
   *
   * @param c a character to insert.
   * @return true if the character could be inserted, false otherwise.
   */
  template <typename C, typename T>
    inline bool
    basic_pstreambuf<C,T>::write(char_type c)
    {
      if (wpipe() >= 0)
      {
        return (::write(wpipe(), &c, sizeof(char_type)) == sizeof(char_type));
      }
      return false;
    }

  /**
   * Attempts to extract a character from the pipe and store it in @a c.
   * Used by underflow().
   * This currently only works for fixed width character encodings where
   * each character uses sizeof(char_type) bytes.
   *
   * @param c a reference to hold the extracted character.
   * @return true if a character could be extracted, false otherwise.
   */
  template <typename C, typename T>
    inline bool
    basic_pstreambuf<C,T>::read(char_type& c)
    {
      if (rpipe() >= 0)
      {
        return (::read(rpipe(), &c, sizeof(char_type)) == sizeof(char_type));
      }
      return false;
    }


#if 0
  /**
   * Attempts to insert @a c into the pipe. Used by overflow().
   *
   * @param c a character to insert.
   * @return true if the character could be inserted, false otherwise.
   * @see write(char_type* s, std::streamsize n)
   */
  template <typename C, typename T>
    inline bool
    basic_pstreambuf<C,T>::write(char_type c)
    {
      return (this->write(&c, 1) == 1);
    }

  /**
   * Attempts to extract a character from the pipe and store it in @a c.
   * Used by underflow().
   *
   * @param c a reference to hold the extracted character.
   * @return true if a character could be extracted, false otherwise.
   * @see read(char_type* s, std::streamsize n)
   */
  template <typename C, typename T>
    inline bool
    basic_pstreambuf<C,T>::read(char_type& c)
    {
      return (this->read(&c, 1) == 1);
    }

  /**
   * Writes up to @a n characters to the pipe from the buffer @a s.
   * This currently only works for fixed width character encodings where
   * each character uses sizeof(char_type) bytes.
   *
   * @param s character buffer.
   * @param n buffer length.
   * @return the number of characters written.
   */
  template <typename C, typename T>
    inline std::streamsize
    basic_pstreambuf<C,T>::write(char_type* s, std::streamsize n)
    {
      return (wpipe() >= 0 ? ::write(wpipe(), s, n * sizeof(char_type)) : 0);
    }

  /**
   * Reads up to @a n characters from the pipe to the buffer @a s.
   * This currently only works for fixed width character encodings where
   * each character uses sizeof(char_type) bytes.
   *
   * @param s character buffer.
   * @param n buffer length.
   * @return the number of characters read.
   */
  template <typename C, typename T>
    inline std::streamsize
    basic_pstreambuf<C,T>::read(char_type* s, std::streamsize n)
    {
      return (rpipe() >= 0 ? ::read(rpipe(), s, n * sizeof(char_type)) : 0);
    }
#endif

  /** @return a reference to the output file descriptor */
  template <typename C, typename T>
    inline typename basic_pstreambuf<C,T>::fd_t&
    basic_pstreambuf<C,T>::wpipe()
    {
      return wpipe_;
    }

  /** @return a reference to the active input file descriptor */
  template <typename C, typename T>
    inline typename basic_pstreambuf<C,T>::fd_t&
    basic_pstreambuf<C,T>::rpipe()
    {
      return rpipe_[rsrc_];
    }

  /** @return a reference to the state of the active input character buffer */
  template <typename C, typename T>
    inline bool&
    basic_pstreambuf<C,T>::take_from_buf()
    {
      return take_from_buf_[rsrc_];
    }

  /** @return a reference to the active input character buffer */
  template <typename C, typename T>
    inline typename basic_pstreambuf<C,T>::char_type&
    basic_pstreambuf<C,T>::char_buf()
    {
      return char_buf_[rsrc_];
    }

  /**
   * Inspects each of the @a count file descriptors in the array @a filedes
   * and calls @c close() if they have a non-negative value.
   * @param filedes an array of file descriptors
   * @param count size of the array.
   */
  template <typename C, typename T>
    inline void
    basic_pstreambuf<C,T>::close_fd_array(fd_t* filedes, size_t count)
    {
      for (size_t i = 0; i < count; ++i)
        if (filedes[i] >= 0)
          ::close(filedes[i]);
    }


  /*
   * member definitions for pstream_base
   */

  /**
   * @class pstream_base
   * Abstract Base Class providing common functionality for basic_ipstream,
   * basic_opstream and basic_pstream.
   * pstream_base manages the basic_pstreambuf stream buffer that is used
   * by the derived classes to initialise an IOStream class.
   */

  /** Creates an uninitialised stream. */
  template <typename C, typename T>
    inline
    pstream_base<C,T>::pstream_base()
    : std::basic_ios<C,T>(NULL)
    , command_()
    , buf_()
    {
      this->init(&buf_);
    }

  /**
   * Initialises the stream buffer by calling open() with the supplied
   * arguments.
   *
   * @param command a string containing a shell command.
   * @param mode the I/O mode to use when opening the pipe.
   * @see open()
   */
  template <typename C, typename T>
    inline
    pstream_base<C,T>::pstream_base(const std::string& command, pmode mode)
    : std::basic_ios<C,T>(NULL)
    , command_(command)
    , buf_()
    {
      this->init(&buf_);
      this->open(command, mode);
    }
   
  /**
   * Initialises the stream buffer by calling open() with the supplied
   * arguments.
   *
   * @param file a string containing the pathname of a program to execute.
   * @param argv a vector of argument strings passed to the new program.
   * @param mode the I/O mode to use when opening the pipe.
   * @see open()
   */
  template <typename C, typename T>
    inline
    pstream_base<C,T>::pstream_base(const std::string& file, const std::vector<std::string>& argv, pmode mode)
    : std::basic_ios<C,T>(NULL)
    , command_(file)
    , buf_()
    {
      this->init(&buf_);
      this->open(file, argv, mode);
    }

  /**
   * This is a pure virtual function to make @c pstream_base abstract.
   * Because it is the destructor it will be called by derived classes
   * and so must be defined.
   * @sa If defining a pure virtual seems odd you should read
   * http://www.gotw.ca/gotw/031.htm (and the rest of the site as well!)
   */
  template <typename C, typename T>
    inline
    pstream_base<C,T>::~pstream_base()
    {
    }

  /**
   * Starts a new process by passing @a command to the shell and
   * opens pipes to the process as given by @a mode.
   *
   * @param command a string containing a shell command.
   * @param mode the I/O mode to use when opening the pipe.
   * @see basic_pstreambuf::open()
   */
  template <typename C, typename T>
    inline void
    pstream_base<C,T>::open(const std::string& command, pmode mode)
    {
      if (!buf_.open((command_=command), mode))
        this->setstate(std::ios_base::failbit);
    }

  /**
   * Starts a new process by executing @a file with the arguments in
   * @a argv and opens pipes to the process as given by @a mode.
   *
   * @param file a string containing the pathname of a program to execute.
   * @param argv a vector of argument strings passed to the new program.
   * @param mode the I/O mode to use when opening the pipe.
   * @see basic_pstreambuf::open()
   */
  template <typename C, typename T>
    inline void
    pstream_base<C,T>::open(const std::string& file, const std::vector<std::string>& argv, pmode mode)
    {
      if (!buf_.open((command_=file), argv, mode))
        this->setstate(std::ios_base::failbit);
    }

  /** Waits for the associated process to finish and closes the pipe. */
  template <typename C, typename T>
    inline void
    pstream_base<C,T>::close()
    {
      if (!buf_.close())
        this->setstate(std::ios_base::failbit);
    }

  /**
   * @return true if open() has been successfully called, false otherwise.
   * @see basic_pstreambuf::open()
   */
  template <typename C, typename T>
    inline bool
    pstream_base<C,T>::is_open() const
    {
      return buf_.is_open();
    }

  /** @return a string containing the command used to initialise the stream. */
  template <typename C, typename T>
    inline const std::string&
    pstream_base<C,T>::command() const
    {
      return command_;
    }

#if REDI_EVISCERATE_PSTREAMS
  /**
   * @def REDI_EVISCERATE_PSTREAMS
   * If this macro has a non-zero value then certain internals of the
   * @c basic_pstreambuf template class are exposed. In general this is
   * a Bad Thing, as the internal implementation is largely undocumented
   * and may be subject to change at any time, so this feature is only
   * provided because it might make PStreams useful in situations where
   * it is necessary to do Bad Things.
   */

  /**
   * @warning  This function exposes the internals of the stream buffer and
   *           should be used with caution. It is the caller's responsibility
   *           to flush streams etc. in order to clear any buffered data.
   *           The POSIX.1 function @c fdopen(3) is used to obtain the
   *           @c FILE pointers from the streambuf's private file descriptor
   *           members so consult your system's documentation for @c fdopen().
   *
   * @param  in   A FILE* that will refer to the process' stdin.
   * @param  out  A FILE* that will refer to the process' stdout.
   * @param  err  A FILE* that will refer to the process' stderr.
   * @return A bitwise-or of zero or more of @c pstdin, @c pstdout, @c pstderr.
   *
   * For each open stream shared with the child process a @c FILE* is
   * obtained and assigned to the corresponding parameter. For closed
   * streams @c NULL is assigned to the parameter.
   * The return value can be tested to see which parameters should be
   * @c !NULL by masking with the corresponding @c pmode value.
   */
  template <typename C, typename T>
    inline size_t
    basic_pstreambuf<C,T>::fopen(FILE*& in, FILE*& out, FILE*& err)
    {
      in = out = err = NULL;
      pmode open_files = 0;
      if (this->wpipe() > -1)
      {
        in = ::fdopen(this->wpipe(), "w");
        open_files |= pstdin;
      }
      if (this->rpipe(rsrc_out) > -1) 
      {
        in = ::fdopen(this->rpipe(rsrc_out), "r");
        open_files |= pstdout;
      }
      if (this->rpipe(rsrc_err) > -1)
      {
        in = ::fdopen(this->rpipe(rsrc_err), "r");
        open_files |= pstderr;
      }
      return open_files;
    }

  /**
   *  @warning This function exposes the internals of the stream buffer and
   *  should be used with caution.
   *
   *  @param  in   A FILE* that will refer to the process' stdin.
   *  @param  out  A FILE* that will refer to the process' stdout.
   *  @param  err  A FILE* that will refer to the process' stderr.
   *  @return A bitwise-or of zero or more of @c pstdin, @c pstdout, @c pstderr.
   *  @see    basic_pstreambuf::fopen()
   */
  template <typename C, typename T>
    inline size_t
    pstream_base<C,T>::fopen(FILE*& in, FILE*& out, FILE*& err)
    {
      return buf_.fopen(in, out, err);
    }

#endif


} // namespace redi

/**
 * @mainpage Pstreams Reference
 * @htmlinclude mainpage.html
 */

#endif  // REDI_PSTREAM_H

