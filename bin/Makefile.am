AM_CXXFLAGS = -I$(srcdir)/.. -I$(srcdir)/../ir -I$(srcdir)/../frontend \
			  -I$(srcdir)/../utils -I$(builddir)/../frontend \
			  -I$(srcdir)/../backend

noinst_bindir = $(abs_top_builddir)
noinst_bin_PROGRAMS = readeq # edl-ester

# edl_ester_SOURCES = edl-ester.cpp
# edl_ester_LDADD = ../ir/libir.la ../frontend/libparser.la \
# 				  ../backend/libester-backend.la \
# 				  ../utils/libutils.la

readeq_SOURCES = readeq.cpp
readeq_LDADD = ../ir/libir.la ../frontend/libparser.la \
				../backend/libtop-backend.la \
				../utils/libutils.la
