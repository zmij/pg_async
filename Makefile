# $Id: Makefile,v 1.29 2008/07/07 22:33:17 redi Exp $
# PStreams Makefile
# Copyright (C) Jonathan Wakely
#
# This file is part of PStreams.
# 
# PStreams is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
# 
# PStreams is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# TODO configure script (allow doxgenating of EVISCERATE functions)

OPTIM=-g3
EXTRA_CFLAGS=
EXTRA_CXXFLAGS=

CFLAGS=-pedantic -Werror -Wall -W -Wpointer-arith -Wcast-qual -Wcast-align -Wredundant-decls $(OPTIM)
CXXFLAGS=$(CFLAGS) -std=c++98 -Woverloaded-virtual

INSTALL_PREFIX ?= /usr/local

SOURCES = pstream.h
GENERATED_FILES = ChangeLog MANIFEST
EXTRA_FILES = AUTHORS COPYING.LIB Doxyfile INSTALL Makefile README \
	    mainpage.html test_pstreams.cc test_minimum.cc

DIST_FILES = $(SOURCES) $(GENERATED_FILES) $(EXTRA_FILES)

VERS = 0.6.0

all: docs $(GENERATED_FILES)

test: test_pstreams test_minimum
	@for test in $^ ; do echo $$test ; ./$$test >/dev/null 2>&1 || echo "$$test EXITED WITH STATUS $$?" ; done

test_%: test_%.cc pstream.h
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(LDFLAGS) -o $@ $<

MANIFEST: Makefile
	@for i in $(DIST_FILES) ; do echo "pstreams-$(VERS)/$$i" ; done > $@

docs: pstream.h mainpage.html
	@doxygen Doxyfile

mainpage.html: Makefile
	@perl -pi -e "s/^(<p>Version) [0-9\.]*(<\/p>)/\1 $(VERS)\2/" $@

ChangeLog:
	@if [ -f CVS/Root ] ; then cvs2cl.pl --no-times -U usermap -F trunk ; fi

dist: pstreams-$(VERS).tar.gz pstreams-docs-$(VERS).tar.gz

pstreams-$(VERS).tar.gz: pstream.h $(GENERATED_FILES)
	@ln -s . pstreams-$(VERS)
	@tar -czvf $@ `cat MANIFEST`
	@rm pstreams-$(VERS)

pstreams-docs-$(VERS).tar.gz: docs
	@ln -s doc/html pstreams-docs-$(VERS)
	@tar -czvhf $@ pstreams-docs-$(VERS)
	@rm pstreams-docs-$(VERS)

TODO : pstream.h mainpage.html test_pstreams.cc
	@grep -nH TODO $^ | sed -e 's@ *// *@@' > $@

clean:
	@rm -f  test_minimum test_pstreams
	@rm -rf doc TODO

install:
	@install -d $(INSTALL_PREFIX)/include/pstreams
	@install -v -m0644 pstream.h $(INSTALL_PREFIX)/include/pstreams

.PHONY: TODO test ChangeLog

