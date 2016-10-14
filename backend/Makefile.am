EXTRA_DIST = BackEnd.h EsterBackEnd.h TopBackEnd.h

AM_CXXFLAGS = -I$(srcdir)/.. -I$(srcdir)/../ir -I$(srcdir)/../frontend \
			  -I$(srcdir)/../utils

noinst_LTLIBRARIES = libtop-backend.la # libester-backend.la
noinst_bindir = .
# libester_backend_la_SOURCES = EsterBackEnd.cpp
# libester_backend_la_LIBADD = ../ir/libir.la ../frontend/libparser.la \
# 							 ../utils/libutils.la

libtop_backend_la_SOURCES = TopBackEnd.cpp BackEnd.cpp
libtop_backend_la_LIBADD = ../ir/libir.la ../frontend/libparser.la \
							 ../utils/libutils.la
