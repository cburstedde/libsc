
dnl sc_shared.m4 - macros to support shared libraries
dnl
dnl This file is part of the SC Library.
dnl The SC library provides support for parallel scientific applications.
dnl
dnl Copyright (C) 2009 Carsten Burstedde, Lucas Wilcox.

dnl SC_LIBTOOL(PREFIX)
dnl Initialize libtool and check for --enable-shared option.
dnl Shared builds are disabled by default.
dnl This seems to call AC_PROG_CC. Call this macro after CC has been set.
dnl
AC_DEFUN([SC_LIBTOOL],
[
LT_INIT([disable-shared])
if test "$enable_shared" != no ; then
  $1_ENABLE_SHARED="yes"
  $1_LIB_SUFFIX="la"
  if test "$enable_static" == no; then
    $1_ENABLE_STATIC="no"
  fi
else
  $1_ENABLE_STATIC="yes"
  $1_ENABLE_SHARED="no"
  $1_LIB_SUFFIX="a"
fi
AC_SUBST([$1_LIB_SUFFIX])   dnl use with care: confuses automake dependencies
AM_CONDITIONAL([$1_ENABLE_SHARED], [test "$enable_shared" != no])
AM_CONDITIONAL([$1_ENABLE_STATIC], [test "$enable_static" != no])
])
