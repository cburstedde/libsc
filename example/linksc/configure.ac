dnl
dnl This file is part of the SC Library.
dnl

dnl Initialize autoconf
AC_INIT([linksc], [1.0.0], [p4est@ins.uni-bonn.de])
AC_CONFIG_SRCDIR([linksc.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([config])
AC_CONFIG_HEADERS([linksc.h])
AC_PREFIX_DEFAULT([$PWD/local])

dnl Initialize automake
AM_INIT_AUTOMAKE([parallel-tests subdir-objects])
AM_SILENT_RULES

dnl Initialize libtool
LT_INIT

dnl Check for programs
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

dnl Configure path to directory with installed libsc.so
AC_ARG_WITH(
  [sc],
  [AS_HELP_STRING([--with-sc=PATH],
                  [path to make installed sc library])],
  [],
  []
)
AC_MSG_ERROR([This demo requires --with-sc=<path to installation>])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
