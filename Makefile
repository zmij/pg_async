# $Id: Makefile,v 1.3 2002/01/07 11:36:54 redi Exp $
# PStreams Makefile
# Copyright (C) Jonathan Wakely
#
# This file is part of PStreams.
# 
# PStreams is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# PStreams is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with PStreams; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

CXX=g++3

all: test distro

test: test_pstreams
# redirect output to /dev/null so we only see test results
	./$< >/dev/null

test_pstreams: test_pstreams.cc pstream.h
	$(CXX) $(CXXFLAGS) -g3 -o $@ $<

distro: pstreams.tar.gz

ChangeLog:
	@cvs2cl.pl

pstreams.tar.gz: pstream.h pstreams.html COPYING TODO Makefile ChangeLog
	@tar czf $@ $^

TODO : pstream.h  pstreams.html test_pstreams.cc
	@grep -nH TODO $^ | sed -e 's@ *// *@@' > $@

.PHONY: TODO test distro ChangeLog

