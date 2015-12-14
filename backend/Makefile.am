EXTRA_DIST = BackEnd.h EsterBackEnd.h

AM_CXXFLAGS = -I$(srcdir)/.. -I$(srcdir)/../ir -I$(srcdir)/../frontend \
			  -I$(srcdir)/../utils

noinst_LTLIBRARIES = libester-backend.la
libester_backend_la_SOURCES = EsterBackEnd.cpp
libester_backend_la_LIBADD = ../ir/libir.la ../utils/libutils.la
