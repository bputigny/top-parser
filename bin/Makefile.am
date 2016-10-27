AM_CXXFLAGS = -I$(srcdir)/.. -I$(srcdir)/../ir -I$(srcdir)/../frontend \
			  -I$(srcdir)/../utils -I$(builddir)/../frontend \
			  -I$(srcdir)/../backend

bin_PROGRAMS = edl-top edl-top2 # edl-ester

# edl_ester_SOURCES = edl-ester.cpp
# edl_ester_LDADD = ../ir/libir.la ../frontend/libparser.la \
# 				  ../backend/libester-backend.la \
# 				  ../utils/libutils.la

edl_top_SOURCES = edl-top.cpp
edl_top_LDADD = ../ir/libir.la ../frontend/libparser.la \
				../backend/libtop-backend.la \
				../utils/libutils.la

edl_top2_SOURCES = edl-top2.cpp
edl_top2_LDADD = ../ir/libir.la ../frontend/libparser.la \
				../backend/libtop-backend.la \
				../utils/libutils.la
