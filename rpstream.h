/* $Id: rpstream.h,v 1.8 2003/02/27 17:29:43 redi Exp $
PStreams - POSIX Process I/O for C++
Copyright (C) 2001,2002 Jonathan Wakely

This file is part of PStreams.

PStreams is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of
the License, or (at your option) any later version.

PStreams is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with PStreams; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/**
 * @file rpstream.h
 * @brief Declares the Restricted PStream class.
 * @author Jonathan Wakely
 *
 * Defines class redi::rpstream.
 */

#ifndef REDI_RPSTREAM_H
#define REDI_RPSTREAM_H

#include "pstream.h"

// Check the PStreams version.
// #if PSTREAMS_VERSION < 0x0022
// #error This version of rpstream.h is incompatible with your pstream.h
// #error Please use pstream.h v0.34 or higher.
// #endif


// All PStreams classes are declared in namespace redi.
namespace redi
{
  /// Class template for Restricted PStreams.
  template <typename CharT, typename Traits = std::char_traits<CharT> >
    class basic_rpstream
    : public std::basic_ostream<CharT, Traits>
    , private std::basic_istream<CharT, Traits>
    , private pstream_common<CharT, Traits>
    {
      typedef std::basic_ostream<CharT, Traits>     ostream_type;
      typedef std::basic_istream<CharT, Traits>     istream_type;
      typedef pstream_common<CharT, Traits>         pbase_type;

      using pbase_type::buf_;  // declare name in this scope

    public:
      /// Type used to specify how to connect to the process
      typedef typename pbase_type::pmode            pmode;

      /// Default constructor, creates an uninitialised stream.
      basic_rpstream();

      /// Constructor that initialises the stream by starting a process.
      basic_rpstream(const std::string& command, pmode mode = std::ios_base::in|std::ios_base::out);

      /// Constructor that initialises the stream by starting a process.
      basic_rpstream(const std::string& file, const std::vector<std::string>& argv, pmode mode = std::ios_base::in|std::ios_base::out);

      /// Destructor
      ~basic_rpstream() { }

      /// Start a process.
      void
      open(const std::string& command, pmode mode = std::ios_base::in|std::ios_base::out);

      /// Start a process.
      void
      open(const std::string& file, const std::vector<std::string>& argv, pmode mode = std::ios_base::in|std::ios_base::out);

      /// Obtain a reference to the istream that reads the process' @c stderr
      istream_type&
      out();

      /// Obtain a reference to the istream that reads the process' @c stderr
      istream_type&
      err();
    };

  /// Type definition for common template specialisation.
  typedef basic_rpstream<char> rpstream;


  // TODO document RPSTREAMS better

  /*
   * member definitions for basic_rpstream
   */

  /**
   * @class basic_rpstream
   * Writing to an rpstream opened with @c pmode @c pstdin writes to the
   * standard input of the command.
   * It is not possible to read directly from an rpstream object, to use
   * an rpstream as in istream you must call either basic_rpstream::out()
   * or basic_rpstream::err(). This is to prevent accidental reads from
   * the wrong input source. If the rpstream was not opened with @c pmode
   * @c pstderr then the class cannot read the process' @c stderr, and
   * basic_rpstream::err() will return an istream that reads from the
   * process' @c stdout, and vice versa.
   * Reading from an rpstream opened with @c pmode @c pstdout and/or @c pstderr
   * reads the command's standard output and/or standard error.
   * Any of the process' @c stdin, @c stdout or @c stderr that is not
   * connected to the pstream (as specified by the @c pmode)
   * will be the same as the process that created the pstream object,
   * unless altered by the command itself.
   */

  /**
   * Creates an uninitialised stream.
   */
  template <typename C, typename T>
    basic_rpstream<C,T>::basic_rpstream()
    : ostream_type(NULL)
    , istream_type(NULL)
    , pbase_type()
    {
      this->init(&buf_);  // calls shared std::basic_ios virtual base class
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
    basic_rpstream<C,T>::basic_rpstream(const std::string& command, pmode mode)
    : ostream_type(NULL)
    , istream_type(NULL)
    , pbase_type(command, mode)
    {
      this->init(&buf_);
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
    basic_rpstream<C,T>::basic_rpstream(const std::string& file, const std::vector<std::string>& argv, pmode mode)
    : ostream_type(NULL)
    , istream_type(NULL)
    , pbase_type(file, argv, mode)
    {
      this->init(&buf_);
    }

  /**
   * Starts a new process by passing @a command to the shell
   * and opens a pipe to the process with the specified @a mode.
   * @param command a string containing a shell command.
   * @param mode the I/O mode to use when opening the pipe.
   * @see basic_rpstreambuf::open()
   */
  template <typename C, typename T>
    inline void
    basic_rpstream<C,T>::open(const std::string& command, pmode mode)
    {
      pbase_type::open(command, mode);
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
    basic_rpstream<C,T>::open(const std::string& file, const std::vector<std::string>& argv, pmode mode)
    {
      pbase_type::open(file, argv, mode);
    }

  /**
   * @return @c *this
   */
  template <typename C, typename T>
    inline typename basic_rpstream<C,T>::istream_type&
    basic_rpstream<C,T>::out()
    {
      buf_.read_err(false);
      return *this;
    }

  /**
   * @return @c *this
   */
  template <typename C, typename T>
    inline typename basic_rpstream<C,T>::istream_type&
    basic_rpstream<C,T>::err()
    {
      buf_.read_err(true);
      return *this;
    }

} // namespace redi

#endif  // REDI_RPSTREAM_H

