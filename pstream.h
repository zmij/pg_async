/* $Id: pstream.h,v 1.2 2001/12/13 03:23:11 redi Exp $
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

ipstream(const char* cmd);
opstream(const char* cmd);
pstream(const char* cmd);
     Passes cmd to /bin/sh and opens a pipe to the new process.

void open(const char* cmd);
     Passes cmd to /bin/sh and opens a pipe to the new process.

Writing to an open pstream writes to the standard input of the command;
the command's standard output is the same as that of the process that
created the pstream object, unless this is altered by the command itself.
Conversely, reading from a pstream reads the command's standard output,
and the command's standard input is the same as that of the process that
created the object.

N.B. the pstream class for both reading and writing to a process is only
available on some sytems, where popen(3) is implemented using a
bidirectional pipe.
*/

#ifndef REDI_PSTREAM_H
#define REDI_PSTREAM_H

#define PSTREAMS_VERSION 0x0002


// check whether to provide pstream
// popen() needs to use bidirectional pipe
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
# define REDI_PSTREAMS_POPEN_USES_BIDIRECTIONAL_PIPE 1
#elif defined(__NetBSD_Version__) && __NetBSD_Version__ >= 0x01040000
# define REDI_PSTREAMS_POPEN_USES_BIDIRECTIONAL_PIPE 1
#endif


#if __GNUC__ == 2 && __GNUC_MINOR__ >= 7
// gcc 2.7 / 2.8 / 2.9x

// turn on non-standard extensions
#define _STREAM_COMPAT 1

#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <string.h>

namespace redi {

    class pstreambase : public fstreambase {
    public:
        void open(const char* cmd)
        { attach(fileno(popen(cmd, mode_))); errno=0; }
        
    protected:
        pstreambase(const char* mode) : fstreambase() { copy_mode(mode); }

        pstreambase(const char* cmd, const char* mode)
        : fstreambase(fileno(popen(cmd, mode))) { errno=0; copy_mode(mode); }

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
        pstream(const char* cmd) : pstreambase(cmd, MODE) { }
    };
#endif

#else   // ! __GNUC__ == 2 && __GNUC_MINOR__ >= 7

#warning "PStreams currently only supports GCC versions >= 2.7 and < 3"

#include <fstream>
#include <cstdio>
#include <cerrno>

    // very basic implementations
    // incomplete interface: no open(const char*)
    // use non-standard extensions
    // use at your own risk!

    class opstream : public std::ofstream {
        static const char MODE[] = "w";
    public:
        opstream(const char* cmd) : std::ofstream(fileno(popen(cmd, MODE)))
        { errno=0; }
        ~opstream() { pclose(fdopen(rdbuf()->fd(), MODE)); }
    };

    class ipstream : public std::ifstream {
        static const char MODE[] = "r";
    public:
        ipstream(const char* cmd) : std::ifstream(fileno(popen(cmd, MODE)))
        { errno=0; }
        ~ipstream() { pclose(fdopen(rdbuf()->fd(), MODE)); }
    };

#if REDI_PSTREAMS_POPEN_USES_BIDIRECTIONAL_PIPE
    class pstream : public std::fstream {
        static const char MODE[] = "r+";
    public:
        pstream(const char* cmd) : std::fstream(fileno(popen(cmd, MODE)))
        { errno=0; }
        ~pstream() { pclose(fdopen(rdbuf()->fd(), MODE)); }
    };
#endif

#endif  // __GNUC__ == 2 && __GNUC_MINOR__ >= 7
    
}  // namespace redi

#endif  // REDI_PSTREAM_H

