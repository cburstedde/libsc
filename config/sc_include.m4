
dnl sc_include.m4 - general custom macros
dnl
dnl This file is part of the SC Library.
dnl The SC library provides support for parallel scientific applications.
dnl
dnl Copyright (C) 2008,2009 Carsten Burstedde, Lucas Wilcox.

dnl Documentation for macro names: brackets indicate optional arguments

dnl SC_VERSION(PREFIX)
dnl Expose major, minor, and point version numbers as CPP defines.
dnl Also creates a makefile variable PACKAGE_PREFIX with value PREFIX.
dnl
AC_DEFUN([SC_VERSION],
[
  AX_SPLIT_VERSION
  AC_DEFINE_UNQUOTED([VERSION_MAJOR],[$AX_MAJOR_VERSION],[Package major version])
  AC_DEFINE_UNQUOTED([VERSION_MINOR],[$AX_MINOR_VERSION],[Package minor version])
  AC_DEFINE_UNQUOTED([VERSION_POINT],[$AX_POINT_VERSION],[Package point version])
  AC_SUBST([PACKAGE_PREFIX], [$1])
])

dnl SC_ARG_ENABLE_PREFIX(NAME, COMMENT, TOKEN, PREFIX, HELPEXTRA)
dnl Check for --enable/disable-NAME using shell variable PREFIX_ENABLE_TOKEN
dnl If shell variable is set beforehand it overrides the option
dnl If enabled, define TOKEN to 1 and set conditional PREFIX_ENABLE_TOKEN
dnl Default is disabled
dnl
AC_DEFUN([SC_ARG_ENABLE_PREFIX],
[
AC_ARG_ENABLE([$1],
              [AS_HELP_STRING([--enable-$1$5], [$2])],,
              [enableval=no])
if test "x$enableval" != xno ; then
  AC_DEFINE([$3], 1, [DEPRECATED (use $4_ENABLE_$3 instead)])
  AC_DEFINE([ENABLE_$3], 1, [$2])
fi
AM_CONDITIONAL([$4_ENABLE_$3], [test "x$enableval" != xno])
$4_ENABLE_$3="$enableval"
])
AC_DEFUN([SC_ARG_ENABLE],
         [SC_ARG_ENABLE_PREFIX([$1], [$2], [$3], [SC], [$4])])

dnl SC_ARG_DISABLE_PREFIX(NAME, COMMENT, TOKEN, PREFIX, HELPEXTRA)
dnl Check for --enable/disable-NAME using shell variable PREFIX_ENABLE_TOKEN
dnl If shell variable is set beforehand it overrides the option
dnl If enabled, define TOKEN to 1 and set conditional PREFIX_ENABLE_TOKEN
dnl Default is enabled
dnl
AC_DEFUN([SC_ARG_DISABLE_PREFIX],
[
AC_ARG_ENABLE([$1],
              [AS_HELP_STRING([--disable-$1$5], [$2])],,
              [enableval=yes])
if test "x$enableval" != xno ; then
  AC_DEFINE([$3], 1, [DEPRECATED (use $4_ENABLE_$3 instead)])
  AC_DEFINE([ENABLE_$3], 1, [Undefine if: $2])
fi
AM_CONDITIONAL([$4_ENABLE_$3], [test "x$enableval" != xno])
$4_ENABLE_$3="$enableval"
])
AC_DEFUN([SC_ARG_DISABLE],
         [SC_ARG_DISABLE_PREFIX([$1], [$2], [$3], [SC], [$4])])

dnl SC_ARG_WITH_PREFIX(NAME, COMMENT, TOKEN, PREFIX, HELPEXTRA)
dnl Check for --with/without-NAME using shell variable PREFIX_WITH_TOKEN
dnl If shell variable is set beforehand it overrides the option
dnl If with, define TOKEN to 1 and set conditional PREFIX_WITH_TOKEN
dnl Default is without
dnl
AC_DEFUN([SC_ARG_WITH_PREFIX],
[
AC_ARG_WITH([$1],
            [AS_HELP_STRING([--with-$1$5], [$2])],,
            [withval=no])
if test "x$withval" != xno ; then
  AC_DEFINE([$3], 1, [DEPRECATED (use $4_WITH_$3 instead)])
  AC_DEFINE([WITH_$3], 1, [$2])
fi
AM_CONDITIONAL([$4_WITH_$3], [test "x$withval" != xno])
$4_WITH_$3="$withval"
])
AC_DEFUN([SC_ARG_WITH],
         [SC_ARG_WITH_PREFIX([$1], [$2], [$3], [SC], [$4])])

dnl SC_ARG_WITHOUT_PREFIX(NAME, COMMENT, TOKEN, PREFIX, HELPEXTRA)
dnl Check for --with/without-NAME using shell variable PREFIX_WITH_TOKEN
dnl If shell variable is set beforehand it overrides the option
dnl If with, define TOKEN to 1 and set conditional PREFIX_WITH_TOKEN
dnl Default is with
dnl
AC_DEFUN([SC_ARG_WITHOUT_PREFIX],
[
AC_ARG_WITH([$1],
            [AS_HELP_STRING([--without-$1$5], [$2])],,
            [withval=yes])
if test "x$withval" != xno ; then
  AC_DEFINE([$3], 1, [DEPRECATED (use $4_WITH_$3 instead)])
  AC_DEFINE([WITH_$3], 1, [Undefine if: $2])
fi
AM_CONDITIONAL([$4_WITH_$3], [test "x$withval" != xno])
$4_WITH_$3="$withval"
])
AC_DEFUN([SC_ARG_WITHOUT],
         [SC_ARG_WITHOUT_PREFIX([$1], [$2], [$3], [SC], [$4])])

dnl SC_SEARCH_LIBS(FUNCTION, INCLUDE, INVOCATION, SEARCH_LIBS,
dnl                ACTION_IF_FOUND, ACTION_IF_NOT_FOUND, OTHER_LIBS)
dnl
dnl Try to link to a given function first with no extra libs
dnl and then with each out of a list of libraries until found.
dnl The FUNCTION is just the function name in one word, while
dnl its INVOCATION is one valid statement of a C/C++ program.
dnl It may require INCLUDE statements to compile and link ok.
dnl On the inside we call AC_LANG_PROGRAM(INCLUDE, INVOCATION).
dnl The SEARCH_LIBS are a white-space separated list or empty.
dnl The OTHER_LIBS may be empty or a list of depency libraries.
dnl
dnl This macro is modified from AC_SEARCH_LIBS.  In particular,
dnl we use a separate cache variable ac_cv_sc_search_FUNCTION.
dnl
AC_DEFUN([SC_SEARCH_LIBS],
[
AS_VAR_PUSHDEF([ac_Search], [ac_cv_sc_search_$1])dnl
AC_CACHE_CHECK([for library containing $1], [ac_Search],
[
ac_func_sc_search_save_LIBS=$LIBS
for ac_lib in '' $4; do
  if test -z "$ac_lib"; then
    ac_res="none required"
  else
    ac_res=-l$ac_lib
    LIBS="-l$ac_lib $7 $ac_func_sc_search_save_LIBS"
  fi
  AC_LINK_IFELSE([AC_LANG_PROGRAM([$2], [$3])],
                 [AS_VAR_SET([ac_Search], [$ac_res])])
  AS_VAR_SET_IF([ac_Search], [break])
done
AS_VAR_SET_IF([ac_Search],, [AS_VAR_SET([ac_Search], [no])])
LIBS=$ac_func_sc_search_save_LIBS
])
AS_VAR_COPY([ac_res], [ac_Search])
AS_IF([test "$ac_res" != no],
  [test "$ac_res" = "none required" || LIBS="$ac_res $LIBS"
   $5], [$6])
AS_VAR_POPDEF([ac_Search])dnl
])

dnl SC_CHECK_MATH(PREFIX)
dnl Check whether sqrt is found, possibly in -lm, using a link test.
dnl We AC_DEFINE HAVE_MATH to 1 depending on whether it found.
dnl We throw a fatal error if sqrt does not link at all.
dnl
AC_DEFUN([SC_CHECK_MATH],
[
  SC_SEARCH_LIBS([sqrt],
[[
/* provoke side effect to inhibit compiler optimization */
#include <math.h>
#include <stdio.h>
]],
[[
/* make this so complex that the compiler cannot predict the result */
double a = 3.14149;
for (; sqrt (a) < a; a *= 1.000023) { putc ('1', stdout); }
]], [m],
  [AC_DEFINE([HAVE_MATH], [1], [Define to 1 if sqrt links successfully])],
  [AC_MSG_ERROR([unable to link with sqrt, cos, sin, both as is and with -lm])])
])

dnl SC_CHECK_ZLIB(PREFIX)
dnl Check whether adler32_combine is found, possibly in -lz, using a link test.
dnl We AC_DEFINE HAVE_ZLIB to 1 depending on whether it is found.
dnl We set the shell variable PREFIX_HAVE_ZLIB to yes if found.
dnl
AC_DEFUN([SC_CHECK_ZLIB],
[
  SC_SEARCH_LIBS([adler32_combine], [[#include <zlib.h>]],
[[
z_off_t len = 3000;
uLong a = 1, b = 2;
if (a == adler32_combine (a, b, len)) {;}
]], [z],
  [AC_DEFINE([HAVE_ZLIB], [1], [Define to 1 if zlib's adler32_combine links])
   $1_HAVE_ZLIB="yes"],
  [$1_HAVE_ZLIB=])
])

dnl SC_CHECK_JSON(PREFIX)
dnl Check whether json_integer, json_real are found (in -ljansson).
dnl We AC_DEFINE HAVE_JSON to 1 depending on whether it is found.
dnl We set the shell variable PREFIX_HAVE_JSON to yes if found.
dnl
AC_DEFUN([SC_CHECK_JSON],
[
  SC_SEARCH_LIBS([json_integer], [[#include <jansson.h>]],
[[
json_t *jint, *jreal;
if (jint == json_integer ((json_int_t) 15)) { json_decref (jint); }
if (jreal == json_real (.5)) { json_decref (jreal); }
]], [jansson],
  [AC_DEFINE([HAVE_JSON], [1],
             [Define to 1 if json_integer and json_real link])
   $1_HAVE_JSON="yes"],
  [$1_HAVE_JSON=])
])

dnl SC_CHECK_LIB(LIBRARY LIST, FUNCTION, TOKEN, PREFIX)
dnl Check for FUNCTION first as is, then in each of the libraries.
dnl Set shell variable PREFIX_HAVE_TOKEN to nonempty if found.
dnl Call AM_CONDITIONAL with PREFIX_HAVE_TOKEN:
dnl   This means it must be called outside of any shell if block.
dnl Call AC_DEFINE with HAVE_TOKEN if found.
AC_DEFUN([SC_CHECK_LIB], [
AC_SEARCH_LIBS([$2], [$1])
AM_CONDITIONAL([$4_HAVE_$3], [test "x$ac_cv_search_$2" != xno])
$4_HAVE_$3=
if test "x$ac_cv_search_$2" != xno ; then
AC_DEFINE([HAVE_$3], [1], [Have we found function $2.])
$4_HAVE_$3=yes
fi
])

dnl SC_CHECK_LIB_NOCOND(LIBRARY LIST, FUNCTION, TOKEN, PREFIX)
dnl Check for FUNCTION first as is, then in each of the libraries.
dnl Set shell variable PREFIX_HAVE_TOKEN to nonempty if found.
dnl Does not establish an automake conditional;
dnl   safe to call from within a shell if block.
dnl Call AC_DEFINE with HAVE_TOKEN if found.
AC_DEFUN([SC_CHECK_LIB_NOCOND], [
AC_SEARCH_LIBS([$2], [$1])
$4_HAVE_$3=
if test "x$ac_cv_search_$2" != xno ; then
AC_DEFINE([HAVE_$3], [1], [Have we found function $2.])
$4_HAVE_$3=yes
fi
])

dnl SC_REQUIRE_FUNCS(FUNCTION LIST)
dnl Check for all functions in FUNCTION LIST, exit with error if not found
dnl
AC_DEFUN([SC_REQUIRE_FUNCS],
[
m4_foreach_w([sc_thefunc], [$1],
             [AC_CHECK_FUNC([sc_thefunc], ,
                            [AC_MSG_ERROR([\
cannot find function sc_thefunc])])])
])

dnl SC_DETERMINE_INSTALL(PREFIX)
dnl This function throws an error if the variable PREFIX_DIR does not exist.
dnl Looks for PREFIX_DIR/{include,lib,bin} to determine installation status.
dnl Set the shell variable PREFIX_INSTALL to "yes" or "no".
dnl
AC_DEFUN([SC_DETERMINE_INSTALL],
[
if test ! -d "$$1_DIR" ; then
  AC_MSG_ERROR([directory "$$1_DIR" does not exist])
fi
if test -d "$$1_DIR/include" || test -d "$$1_DIR/lib" || \
   test -d "$$1_DIR/bin" || test -d "$$1_DIR/share/aclocal" ; then
  $1_INSTALL=yes
else
  $1_INSTALL=no
fi
])

dnl SC_DETERMINE_INCLUDE_PATH(PREFIX, CPPFLAGS)
dnl This function expects the variable PREFIX_DIR to exist.
dnl Looks for PREFIX_DIR/include and then PREFIX_DIR/src.
dnl If neither is found, throws an error.
dnl Otherwise, set the shell variable PREFIX_CPPFLAGS to -I<dir> CPPFLAGS.
dnl
AC_DEFUN([SC_DETERMINE_INCLUDE_PATH],
[
$1_INC="$$1_DIR/include"
if test ! -d "$$1_INC" ; then
  $1_INC="$$1_DIR/src"
fi
if test ! -d "$$1_INC" ; then
  AC_MSG_ERROR([include directories based on $$1_DIR not found])
fi
$1_CPPFLAGS="-I$$1_INC $2"
])

dnl SC_DETERMINE_LIBRARY_PATH(PREFIX, LIBS)
dnl This function expects the variable PREFIX_DIR to exist.
dnl Looks for PREFIX_DIR/lib and then PREFIX_DIR/src.
dnl If neither is found, throws an error.
dnl Otherwise, set the shell variable PREFIX_LDADD to -L<dir> LIBS.
dnl
AC_DEFUN([SC_DETERMINE_LIBRARY_PATH],
[
$1_LIB="$$1_DIR/lib"
if test ! -d "$$1_LIB" ; then
  $1_LIB="$$1_DIR/src"
fi
if test ! -d "$$1_LIB" ; then
  AC_MSG_ERROR([library directories based on $$1_DIR not found])
fi
$1_LDADD="-L$$1_LIB $2"
])

dnl SC_DETERMINE_CONFIG_PATH(PREFIX)
dnl This function expects the variable PREFIX_DIR to exist.
dnl Looks for PREFIX_DIR/share/aclocal and then PREFIX_DIR/src.
dnl If neither is found, throws an error.
dnl Sets shell variables PREFIX_CONFIG and PREFIX_AMFLAGS.
dnl
AC_DEFUN([SC_DETERMINE_CONFIG_PATH],
[
$1_CONFIG="$$1_DIR/share/aclocal"
if test ! -d "$$1_CONFIG" ; then
  $1_CONFIG="$$1_DIR/config"
fi
if test ! -d "$$1_CONFIG" ; then
  AC_MSG_ERROR([config directories based on $$1_DIR not found])
fi
$1_AMFLAGS="-I $$1_CONFIG"
])

dnl dnl SC_CHECK_BLAS_LAPACK(PREFIX)
dnl dnl This function uses the macros SC_BLAS and SC_LAPACK.
dnl dnl It requires previous configure macros for F77 support,
dnl dnl which are called by SC_MPI_CONFIG/SC_MPI_ENGAGE.
dnl dnl
dnl AC_DEFUN([SC_CHECK_BLAS_LAPACK],
dnl [
dnl m4_ifset([SC_CHECK_MPI_F77],[
dnl dgemm=;AC_F77_FUNC(dgemm)
dnl if test "x$dgemm" = xunknown ; then dgemm=dgemm_ ; fi
dnl
dnl AC_MSG_NOTICE([Checking BLAS])
dnl SC_BLAS([$1], [$dgemm],
dnl         [AC_DEFINE([WITH_BLAS], 1, [Define to 1 if BLAS is used])],
dnl         [AC_MSG_ERROR([[\
dnl cannot find BLAS library, specify a path using LIBS=-L<DIR> (ex.\
dnl  LIBS=-L/usr/path/lib) or a specific library using BLAS_LIBS=DIR/LIB\
dnl  (for example BLAS_LIBS=/usr/path/lib/libcxml.a)]])])
dnl
dnl # at this point $sc_blas_ok is either of: yes disable
dnl if test "x$sc_blas_ok" = xdisable ; then
dnl         AC_MSG_NOTICE([Not using BLAS])
dnl fi
dnl AM_CONDITIONAL([$1_WITH_BLAS], [test "x$sc_blas_ok" = xyes])
dnl
dnl dgecon=;AC_F77_FUNC(dgecon)
dnl if test "x$dgecon" = xunknown ; then dgecon=dgecon_ ; fi
dnl
dnl AC_MSG_NOTICE([Checking LAPACK])
dnl SC_LAPACK([$1], [$dgecon],
dnl           [AC_DEFINE([WITH_LAPACK], 1, [Define to 1 if LAPACK is used])],
dnl           [AC_MSG_ERROR([[\
dnl cannot find LAPACK library, specify a path using LIBS=-L<DIR> (ex.\
dnl  LIBS=-L/usr/path/lib) or a specific library using LAPACK_LIBS=DIR/LIB\
dnl  (for example LAPACK_LIBS=/usr/path/lib/libcxml.a)]])])
dnl
dnl # at this point $sc_lapack_ok is either of: yes disable
dnl if test "x$sc_lapack_ok" = xdisable ; then
dnl         AC_MSG_NOTICE([Not using LAPACK])
dnl fi
dnl AM_CONDITIONAL([$1_WITH_LAPACK], [test "x$sc_lapack_ok" = xyes])
dnl
dnl # Append the necessary blas/lapack and fortran libraries to LIBS
dnl LIBS="$LAPACK_LIBS $BLAS_LIBS $LIBS $LAPACK_FLIBS $BLAS_FLIBS"
dnl ])
dnl ])

dnl SC_CHECK_LIBRARIES(PREFIX)
dnl This macro bundles the checks for all libraries and link tests
dnl that are required by libsc.  It can be used by other packages that
dnl link to libsc to add appropriate options to LIBS.
dnl We also test for some tool executables.
dnl
AC_DEFUN([SC_CHECK_LIBRARIES],
[
AC_DEFINE([USING_AUTOCONF], 1, [Define to 1 if using autoconf build])
AC_CHECK_PROG([$1_HAVE_DOT], [dot], [YES], [NO])
SC_CHECK_MATH([$1])
SC_CHECK_ZLIB([$1])
SC_CHECK_JSON([$1])
dnl SC_CHECK_LIB([lua53 lua5.3 lua52 lua5.2 lua51 lua5.1 lua5 lua],
dnl              [lua_createtable], [LUA], [$1])
dnl SC_CHECK_BLAS_LAPACK([$1])
SC_BUILTIN_ALL_PREFIX([$1])
SC_CHECK_PTHREAD([$1])
SC_CHECK_OPENMP([$1])
SC_CHECK_MEMALIGN([$1])
SC_CHECK_QSORT_R([$1])
SC_CHECK_V4L2([$1])
dnl SC_CUDA([$1])
])

dnl SC_AS_SUBPACKAGE(PREFIX)
dnl Call from a package that is using libsc as a subpackage.
dnl Sets PREFIX_DIST_DENY=yes if sc is make install'd.
dnl
AC_DEFUN([SC_AS_SUBPACKAGE],
         [SC_ME_AS_SUBPACKAGE([$1], [m4_tolower([$1])],
                              [SC], [sc], [libsc])])

dnl SC_FINAL_MESSAGES(PREFIX)
dnl This macro prints messages at the end of the configure run.
dnl
AC_DEFUN([SC_FINAL_MESSAGES],
[
if test "x$$1_HAVE_ZLIB" = x ; then
AC_MSG_NOTICE([- $1 ----------------------------------------------------
We did not find a recent zlib containing the function adler32_combine.
This is OK if the following does not matter to you:
 - Calling some functions that rely on zlib will abort your program.
   These include sc_array_checksum and sc_vtk_write_compressed.
 - The data produced by sc_io_encode is not compressed.
 - The function sc_io_decode is slower than with zlib.
You can fix this by compiling a recent zlib and pointing LIBS to it.])
fi
if test "x$$1_HAVE_JSON" = x ; then
AC_MSG_NOTICE([- $1 ----------------------------------------------------
We did not find a JSON library containing json_integer and json_real.
This means that loading JSON files for option values will fail.
You can fix this by installing the jansson development library.])
fi
])
