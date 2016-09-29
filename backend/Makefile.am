EXTRA_DIST = BackEnd.h EsterBackEnd.h TopBackEnd.h

AM_CXXFLAGS = -I$(srcdir)/.. -I$(srcdir)/../ir -I$(srcdir)/../frontend \
			  -I$(srcdir)/../utils

noinst_LTLIBRARIES = libtop-backend.la # libester-backend.la
noinst_bindir = .
noinst_bin_PROGRAMS = test-top-backend
# libester_backend_la_SOURCES = EsterBackEnd.cpp
# libester_backend_la_LIBADD = ../ir/libir.la ../frontend/libparser.la \
# 							 ../utils/libutils.la

libtop_backend_la_SOURCES = TopBackEnd.cpp BackEnd.cpp
libtop_backend_la_LIBADD = ../ir/libir.la ../frontend/libparser.la \
							 ../utils/libutils.la

test_top_backend_SOURCES = testTopBackEnd.cpp
test_top_backend_LDADD = ../ir/libir.la ../frontend/libparser.la \
						 ../utils/libutils.la libtop-backend.la
