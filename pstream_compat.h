/* $Id: pstream_compat.h,v 1.3 2003/04/28 10:57:41 redi Exp $
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
 * @file pstream_compat.h
 * Declares non-standard implementations of the PStreams classes for older
 * compilers.
 */

#ifndef REDI_PSTREAM_COMPAT_H
#define REDI_PSTREAM_COMPAT_H

#if ! ( BACK_COMPAT == 1 || GCC_BACK_COMPAT == 1 )

#error \
    You must define either BACK_COMPAT or GCC_BACK_COMPAT to be 1 \
    to use this file. For gcc versions 2.7 to 2.9x (including egcs) \
    define GCC_BACKCOMPAT = 1. For other compilers define BACK_COMPAT = 1 \
    For ISO C++ conforming compilers use pstream.h instead.

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
    pstream() : pstreambase(MODE) { }
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

#endif // REDI_PSTREAM_COMPAT_H

