/* $Id: pstreamconf.h,v 1.1 2001/12/13 03:27:44 redi Exp $
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

#ifndef REDI_PSTREAMCONF_H
#define REDI_PSTREAMCONF_H

#define PSTREAMS_VERSION 0x0002

// identify compiler version
#if __GNUC__ == 3
// gcc 3.x
# define REDI_PSTREAMS_GCC 3
#elif __GNUC__ == 2 && __GNUC_MINOR__ >= 7
// gcc 2.7 / 2.8 / 2.9x
# define REDI_PSTREAMS_GCC 2
#else
# define REDI_PSTREAMS_GCC 0
#endif

// check whether to provide pstream
// popen() needs to use bidirectional pipe
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
# define REDI_PSTREAMS_POPEN_USES_BIDIRECTIONAL_PIPE 1
#elif defined(__NetBSD_Version__) && __NetBSD_Version__ >= 0x01040000
# define REDI_PSTREAMS_POPEN_USES_BIDIRECTIONAL_PIPE 1
#endif


#endif  // REDI_PSTREAMCONF_H

