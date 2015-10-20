SUBDIRS = utils ir backend frontend

AM_CXXFLAGS = -I$(srcdir)/ir -I$(srcdir)/backend -I$(builddir)/frontend \
			  -I$(srcdir)/frontend -I$(srcdir)/utils

bin_PROGRAMS = edl-ester
edl_ester_SOURCES = edl-ester.cpp
edl_ester_LDADD = backend/libester-backend.la frontend/libparser.la

