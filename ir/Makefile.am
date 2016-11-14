EXTRA_DIST = IR.h SymTab.h DOT.h Coord.h

AM_CXXFLAGS = -I$(srcdir)/.. -I$(srcdir)/../utils -I$(srcdir)/../frontend

noinst_LTLIBRARIES = libir.la
noinst_bindir = $(abs_top_builddir)
noinst_bin_PROGRAMS = test-ir

libir_la_SOURCES = Node.cpp Symbol.cpp DOT.cpp SymTab.cpp Expr.cpp Coord.cpp

test_ir_SOURCES = test.cpp
test_ir_LDADD = ../utils/libutils.la libir.la
