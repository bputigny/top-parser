EXTRA_DIST = IR.h

AM_CXXFLAGS = -I$(srcdir)/.. -I$(srcdir)/../utils -I$(srcdir)/../frontend

noinst_LTLIBRARIES = libir.la
noinst_bindir = .
noinst_bin_PROGRAMS = test-ir

libir_la_SOURCES = Node.cpp Symbol.cpp # Expr.cpp Func.cpp Equation.cpp

test_ir_SOURCES = test.cpp
test_ir_LDADD = ../frontend/libparser.la ../utils/libutils.la libir.la
