BUILT_SOURCES = parser.hpp parser.cpp scanner.cpp

EXTRA_DIST = Analysis.h FrontEnd.h

AM_YFLAGS = -d
AM_CXXFLAGS = -I$(srcdir)/.. -I$(srcdir)/../ir -I$(srcdir)/../utils

noinst_LTLIBRARIES = libparser.la
noinst_bindir = $(abs_top_builddir)
noinst_bin_PROGRAMS = test-frontend

libparser_la_SOURCES = Analysis.cpp parser.ypp scanner.lpp FrontEnd.cpp
libparser_la_CXXFLAGS = $(AM_CXXFLAGS) -Wno-deprecated-register
libparser_la_LIBADD = ../ir/libir.la

test_frontend_SOURCES = test.cpp
test_frontend_LDADD = libparser.la ../ir/libir.la ../utils/libutils.la

clean-local:
	rm -f parser.h parser.hpp parser.cpp scanner.cpp

.h.hpp:
	cp $< $@
