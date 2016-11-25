#                                               -*- Autoconf -*-

AC_PREREQ([2.59])
AC_INIT([edl], [0.1])
AC_CONFIG_AUX_DIR(autoconf)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.9 no-dist-gzip dist-bzip2 tar-ustar
                  -Wall subdir-objects foreign])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PREFIX_DEFAULT(${HOME}/local)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([config.h.in])

AC_LANG([C++])

# Checks for programs.
AC_PROG_CXX
AC_PROG_LEX
AC_CHECK_PROG([LEX_EXISTS], [$LEX], [yes], [no])
AS_IF([test "x$LEX_EXISTS" = "xno"],
      [AC_ERROR(lex is missing)])
AC_PROG_YACC
AC_CHECK_PROG([YACC_EXISTS], [$YACC], [yes], [no])
AS_IF([test "x$YACC_EXISTS" = "xno"],
      [AC_ERROR(yacc is missing)])
AC_PROG_LIBTOOL
AC_CHECK_PROG(DOT, dot, no)
AS_IF([test "x$DOT" = "xno"], [AC_DEFINE(HAVE_DOT, [1], [is dot installed])])

AC_CHECK_HEADER(cassert, [], [AC_ERROR(cassert not found)])
AC_CHECK_HEADER(functional, [], [AC_ERROR(functional not found)])
AC_CHECK_HEADER(typeinfo, [], [AC_ERROR(typeinfo not found)])

save_flags=$CXXFLAGS
CXXFLAGS+=" -std=c++11"

AC_MSG_CHECKING([if $CXX support -std=c++11 flag])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
    [stdcxx11=yes],
    [stdcxx11=no
     CXXFLAGS=$save_flags])
AC_MSG_RESULT($stdcxx11)

AC_MSG_CHECKING([if $CXX support C++11 standard])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
                   #include <string>
                   void foo() {
                       std::string one = std::to_string(1.0);
                   }
                   ])],
    [cxx11=yes],
    [cxx11=no])
AC_MSG_RESULT($cxx11)
AS_IF([test "x$cxx11" = "xno"],
      [AC_ERROR([$CXX does not supports C++11 standard])])

AC_CONFIG_FILES([Makefile
                 ir/Makefile
                 bin/Makefile
                 utils/Makefile
                 backend/Makefile
                 frontend/Makefile])

AC_ARG_ENABLE([debug],
             AS_HELP_STRING([--enable-debug],
                            [Enables DEBUG]))

AS_IF([test "x$enable_debug" = "xyes"],
      [debug="yes"],
      [AC_DEFINE([NDEBUG], [1], [disable asserts])
       debug="no"])


AC_OUTPUT

cat << EOF

LEX:        $LEX
YACC:       $YACC
CXX:        $CXX
CXXFLAGS:   $CXXFLAGS
DEBUG:      $debug

EOF
