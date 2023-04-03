dnl
dnl This file is part of the SC Library.
dnl

dnl Minimalist example to link against a make installed libsc
dnl In the present version we expect a libsc installation with
dnl subdirectories lib, include.  This will be generalized later.

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

dnl Choose between serial and MPI installation
dnl This flag has to match the make installed libsc
LSC_ENABLE_MPI=no
AC_ARG_ENABLE(
  [mpi],
  [AS_HELP_STRING([--enable-mpi],
                  [choose MPI build (must match make installed libsc)])],
  [
if test "x$enableval" = xyes ; then
  LSC_ENABLE_MPI=yes
elif test "x$enableval" != xno ; then
  AC_MSG_ERROR([Please use --enable-mpi, --disable-mpi without argument])
fi
  ]
)

dnl Specify directory of make installed libsc
dnl This flag is required with a valid shared library installation
LSC_SC_LIB=
LSC_SC_INC=
AC_ARG_WITH(
  [sc],
  [AS_HELP_STRING([--with-sc=PATH],
                  [path to make installed sc library (shared build)])],
  [
LSC_SC_LIB="$withval/lib"
if test ! -e "$LSC_SC_LIB/libsc.so" ; then
  AC_MSG_ERROR([This demo requires installed lib/libsc.so])
fi
LSC_SC_INC="$withval/include"
if test ! -e "$LSC_SC_INC/sc_config.h" ; then
  AC_MSG_NOTICE([This demo requires installed include/sc_config.h])
fi
  ],
  [
AC_MSG_ERROR([This demo requires --with-sc=<path to make installed sc>])
  ]
)

dnl Check for programs
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

dnl Check for headers
AC_HEADER_STDC

dnl Check for consistency of configure options
AC_MSG_CHECKING([for consistency of MPI configuration])
SAVE_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -I$LSC_SC_INC"
if test "x$LSC_ENABLE_MPI" = xyes ; then
  CFLAGS="$CFLAGS -DLSC_ENABLE_MPI=1"
fi
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(
    [[
#include <stdio.h>
#include <sc_config.h>
    ]],
    [[
#if (defined SC_ENABLE_MPI && !defined LSC_ENABLE_MPI) ||\
    (!defined SC_ENABLE_MPI && defined LSC_ENABLE_MPI)
#error "Mismatch between sc's and linksc's --enable-mpi option"
#endif
    ]]
)],
  [
AC_MSG_RESULT([yes])
  ],
  [
AC_MSG_RESULT([no])
AC_MSG_ERROR([Mismatch between sc's and linksc's --enable-mpi option])
  ]
)
CFLAGS="$SAVE_CFLAGS"

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
