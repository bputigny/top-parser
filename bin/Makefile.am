AM_CXXFLAGS = -I$(srcdir)/.. -I$(srcdir)/../ir -I$(srcdir)/../frontend \
			  -I$(srcdir)/../utils -I$(builddir)/../frontend \
			  -I$(srcdir)/../backend

bin_PROGRAMS = edl-ester

edl_ester_SOURCES = edl-ester.cpp
edl_ester_LDADD = ../ir/libir.la ../frontend/libparser.la \
				  ../backend/libester-backend.la \
				  ../utils/libutils.la
