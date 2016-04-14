EXTRA_DIST = BackEnd.h EsterBackEnd.h TopBackEnd.h

AM_CXXFLAGS = -I$(srcdir)/.. -I$(srcdir)/../ir -I$(srcdir)/../frontend \
			  -I$(srcdir)/../utils

noinst_LTLIBRARIES = libester-backend.la libtop-backend.la
libester_backend_la_SOURCES = EsterBackEnd.cpp
libester_backend_la_LIBADD = ../ir/libir.la ../frontend/libparser.la \
							 ../utils/libutils.la

libtop_backend_la_SOURCES = TopBackEnd.cpp
libtop_backend_la_LIBADD = ../ir/libir.la ../frontend/libparser.la \
							 ../utils/libutils.la
