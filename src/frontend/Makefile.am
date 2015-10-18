BUILT_SOURCES = parser.hpp parser.cpp scanner.cpp

EXTRA_DIST = SymTab.h Analysis.h FrontEnd.h

AM_YFLAGS = -d
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/ir -I$(top_srcdir)/src/utils

noinst_LTLIBRARIES = libir.la libparser.la
libir_la_SOURCES = 
libir_la_LIBADD = ../ir/libir.la ../utils/libutils.la

libparser_la_SOURCES = SymTab.cpp Analysis.cpp parser.ypp scanner.lpp FrontEnd.cpp
libparser_la_CXXFLAGS = $(AM_CXXFLAGS) -Wno-deprecated-register
libparser_la_LIBADD = libir.la

clean-local:
	rm -f parser.hpp parser.cpp scanner.cpp
