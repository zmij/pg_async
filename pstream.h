/* $Id: pstream.h,v 1.1 2001/12/13 00:39:16 redi Exp $
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

#ifndef REDI_PSTREAM_H
#define REDI_PSTREAM_H

#include <fstream>
#include <stdio.h>

namespace redi {

    class ipstream : public fstreambase, public istream
    {
    public:
        ipstream() : fstreambase(), fp_(0) { }
        ipstream(const char* name) : fstreambase(), fp_(popen(name,mode))
        { attachfd(); }
        ~ipstream() { if (fp_) pclose(fp_); }
        void open(const char* name) { fp_=popen(name,mode); attachfd(); } 
    private:
        void attachfd()
        {
            if (fp_ && (fileno(fp_)!=-1))
            {
                fstreambase::attach(fileno(fp_));
                errno=0;
            }
        }
        FILE* fp_;
        static const char* const mode="r";
    };

    class opstream : public fstreambase, public ostream
    {
    public:
        opstream() : fstreambase(), fp_(0) { }
        opstream(const char* name) : fstreambase(), fp_(popen(name,mode))
        { attachfd(); }
        ~opstream() { if (fp_) pclose(fp_); }
        void open(const char* name) { fp_=popen(name,mode); attachfd(); }
    private:
        void attachfd()
        {
            if (fp_ && (fileno(fp_)!=-1))
            {
                fstreambase::attach(fileno(fp_));
                errno=0;
            }
        }
        FILE* fp_;
        static const char* const mode="w";
    };

} // namespace redi

#endif  // REDI_PSTREAM_H

