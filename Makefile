# $Id: Makefile,v 1.10 2002/07/24 23:00:21 redi Exp $
# PStreams Makefile
# Copyright (C) Jonathan Wakely
#
# This file is part of PStreams.
# 
# PStreams is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2.1 of
# the License, or (at your option) any later version.
# 
# PStreams is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with PStreams; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# TODO configure script (allow doxgenatiing of EVISCERATE functions)

CXX=g++3

CFLAGS=-Wall -Wpointer-arith -Wcast-qual -Wcast-align -Wredundant-decls
CXXFLAGS=$(CFLAGS) -Woverloaded-virtual

SOURCES = pstream.h rpstream.h
GENERATED_FILES = ChangeLog MANIFEST TODO 
EXTRA_FILES = AUTHORS COPYING Doxyfile INSTALL Makefile README mainpage.html \
              images/pstreams1.png

DIST_FILES= $(SOURCES) $(GENERATED_FILES) $(EXTRA_FILES)

all: docs $(GENERATED_FILES)

test: test_pstreams test_minimum
	@: ./test_minimum >/dev/null
	@./test_pstreams >/dev/null || echo "TEST EXITED WITH STATUS $?"

test_pstreams: test_pstreams.cc pstream.h
	$(CXX) $(CXXFLAGS) -g3 -o $@ $<

test_minimum: test_minimum.cc pstream.h
	$(CXX) $(CXXFLAGS) -o $@ $<

MANIFEST:
	@echo "$(DIST_FILES)" > $@

docs: pstream.h
	@doxygen Doxyfile

ChangeLog:
	@cvs2cl.pl

TODO : pstream.h rpstream.h mainpage.html test_pstreams.cc
	@grep -nH TODO $^ | sed -e 's@ *// *@@' > $@

.PHONY: TODO test docs MANIFEST ChangeLog

