# $Id: Makefile,v 1.15 2003/03/10 01:31:10 redi Exp $
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

OPTIM=-g3
EXTRA_CFLAGS=
EXTRA_CXXFLAGS=

CFLAGS=-Wall -Wpointer-arith -Wcast-qual -Wcast-align -Wredundant-decls $(OPTIM)
CXXFLAGS=$(CFLAGS) -Woverloaded-virtual

SOURCES = pstream.h
GENERATED_FILES = ChangeLog MANIFEST TODO 
EXTRA_FILES = AUTHORS COPYING Doxyfile INSTALL Makefile README mainpage.html \
              images/pstreams1.png

DIST_FILES= $(SOURCES) $(GENERATED_FILES) $(EXTRA_FILES)

all: docs $(GENERATED_FILES)

test: test_pstreams test_minimum
	@./test_minimum >/dev/null 2>&1 || echo "TEST EXITED WITH STATUS $$?"
	@./test_pstreams >/dev/null || echo "TEST EXITED WITH STATUS $$?"

test_pstreams: test_pstreams.cc pstream.h
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o $@ $<

test_minimum: test_minimum.cc pstream.h
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o $@ $<

MANIFEST:
	@echo "$(DIST_FILES)" > $@

docs: pstream.h mainpage.html
	@ver=`sed -n -e 's:^#define *PSTREAMS_VERSION.*// *\([0-9\.]*\):\1:p' $<`;\
	 perl -pi -e "s/^(<p>Version) [0-9\.]*(<\/p>)/\1 $$ver\2/" mainpage.html
	@doxygen Doxyfile

ChangeLog:
	@cvs2cl.pl

TODO : pstream.h mainpage.html test_pstreams.cc
	@grep -nH TODO $^ | sed -e 's@ *// *@@' > $@

.PHONY: TODO test docs MANIFEST ChangeLog

