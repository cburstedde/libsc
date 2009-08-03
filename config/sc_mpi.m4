dnl This started off as a modified version of the Teuchos config dir
dnl from Trilinos with the following license.
dnl
dnl ***********************************************************************
dnl
dnl                    Teuchos: Common Tools Package
dnl                 Copyright (2004) Sandia Corporation
dnl
dnl Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
dnl license for use of this work by or on behalf of the U.S. Government.
dnl
dnl This library is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU Lesser General Public License as
dnl published by the Free Software Foundation; either version 2.1 of the
dnl License, or (at your option) any later version.
dnl
dnl This library is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Lesser General Public License for more details.
dnl
dnl You should have received a copy of the GNU Lesser General Public
dnl License along with this library; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
dnl USA
dnl Questions? Contact Michael A. Heroux (maherou@sandia.gov)
dnl
dnl ***********************************************************************
dnl
dnl Now almost nothing of the original code is left.
dnl
dnl @synopsis SC_MPI(PREFIX, [list of non-MPI C compilers],
dnl                          [list of non-MPI CXX compilers])
dnl
dnl This macro calls AC_PROG_CC (and AC_PROG_CXX if above list is specified).
dnl
dnl --enable-mpi           Turn on MPI, use compilers mpicc and mpif77.
dnl --with-mpicc=<...>     Specify MPI C compiler, can be "no".
dnl --without-mpicc        Do not use a special MPI C compiler.
dnl --with-mpicxx=<...>    Specify MPI CXX compiler, can be "no".
dnl --without-mpicxx       Do not use a special MPI CXX compiler.
dnl --with-mpif77=<...>    Specify MPI F77 compiler, can be "no".
dnl --without-mpif77       Do not use a special MPI F77 compiler.
dnl --with-mpitest=<...>   Use this as PREFIX_MPI_TESTS_ENVIRONMENT.
dnl
dnl All --with* options turn on MPI, so --enable-mpi is then not needed.
dnl
dnl Order of precedence for the selection of MPI compilers
dnl 1. environment variables MPI_CC, MPI_F77
dnl 2. --with-mpicc=<...>, --with-mpif77=<...>
dnl 3. environment variables CC, F77
dnl 4. mpicc, mpif77 unless --without-mpicc, --without-mpif77
dnl
dnl If MPI is turned on PREFIX_MPI will be defined for autoconf/automake
dnl and MPI will be defined in the config header file.

dnl SC_MPI_CONFIG(PREFIX, [TEST_CXX])
dnl Figure out the MPI configuration.
dnl If TEST_CXX is non-empty, the macro SC_CHECK_MPI_CXX is defined.
dnl
AC_DEFUN([SC_MPI_CONFIG],
[
HAVE_PKG_MPI=no
HAVE_PKG_MPIIO=no
MPI_CC_NONE=
MPI_CXX_NONE=
MPI_F77_NONE=
m4_ifval([$2], [m4_define([SC_CHECK_MPI_CXX], [yes])])

dnl The shell variable SC_ENABLE_MPI is set
dnl unless it is provided by the environment.
dnl MPI can also be enabled implicitly later.
dnl Therefore all further checking uses the HAVE_PKG_MPI shell variable
dnl and neither AC_DEFINE nor AM_CONDITIONAL are invoked at this point.
AC_ARG_ENABLE([mpi],
              [AS_HELP_STRING([--enable-mpi], [enable MPI])],,
              [enableval=no])
SC_ARG_OVERRIDE_ENABLE([SC], [MPI])
if test "$enableval" = yes ; then
  HAVE_PKG_MPI=yes
elif test "$enableval" != no ; then
  AC_MSG_ERROR([Please use --enable-mpi without an argument])
fi

dnl Potentially override the mpi C compiler (and turn on MPI)
SC_ARG_NOT_GIVEN_DEFAULT="notgiven"
SC_ARG_WITH([mpicc], [specify MPI C compiler, can be "no"],
            [MPICC], [=MPICC])
if test "$withval" = yes ; then
  AC_MSG_ERROR([Please use --with-mpicc=MPICC with a valid mpi C compiler])
elif test "$withval" = no ; then
  HAVE_PKG_MPI=yes
  MPI_CC_NONE=yes
elif test "$withval" != notgiven ; then
  HAVE_PKG_MPI=yes
  AC_CHECK_PROG([MPI_CC], [$withval], [$withval], [false])
fi

dnl Potentially override the mpi CXX compiler (and turn on MPI)
m4_ifset([SC_CHECK_MPI_CXX], [
SC_ARG_NOT_GIVEN_DEFAULT="notgiven"
SC_ARG_WITH([mpicxx], [specify MPI CXX compiler, can be "no"],
            [MPICXX], [=MPICXX])
if test "$withval" = yes ; then
  AC_MSG_ERROR([Please use --with-mpicxx=MPICXX with a valid mpi CXX compiler])
elif test "$withval" = no ; then
  HAVE_PKG_MPI=yes
  MPI_CXX_NONE=yes
elif test "$withval" != notgiven ; then
  HAVE_PKG_MPI=yes
  AC_CHECK_PROG([MPI_CXX], [$withval], [$withval], [false])
fi
])

dnl Potentially override the mpi Fortran compiler (and turn on MPI)
SC_ARG_NOT_GIVEN_DEFAULT="notgiven"
SC_ARG_WITH([mpif77], [specify MPI F77 compiler, can be "no"],
            [MPIF77], [=MPIF77])
if test "$withval" = yes ; then
  AC_MSG_ERROR([Please use --with-mpif77=MPIF77 with a valid mpi F77 compiler])
elif test "$withval" = no ; then
  HAVE_PKG_MPI=yes
  MPI_F77_NONE=yes
elif test "$withval" != notgiven ; then
  HAVE_PKG_MPI=yes
  AC_CHECK_PROG([MPI_F77], [$withval], [$withval], [false])
fi

dnl HAVE_PKG_MPI is now determined and will not be changed below
AC_MSG_CHECKING([whether we are using MPI])
AC_MSG_RESULT([$HAVE_PKG_MPI])

dnl The shell variable SC_ENABLE_MPIIO is set
dnl unless it is provided by the environment.
dnl If enabled, MPI I/O will be verified by a compile/link test below.
AC_ARG_ENABLE([mpiio],
              [AS_HELP_STRING([--enable-mpiio], [enable MPI I/O])],,
              [enableval=no])
SC_ARG_OVERRIDE_ENABLE([SC], [MPIIO])
if test "$enableval" = yes ; then
  if test "$HAVE_PKG_MPI" = yes ; then
    HAVE_PKG_MPIIO=yes
  fi
elif test "$enableval" != no ; then
  AC_MSG_ERROR([Please use --enable-mpiio without an argument])
fi
AC_MSG_CHECKING([whether we are using MPI I/O])
AC_MSG_RESULT([$HAVE_PKG_MPIIO])

dnl Potentially override the MPI test environment
SC_ARG_NOT_GIVEN_DEFAULT="yes"
SC_ARG_WITH([mpitest], [use DRIVER to run MPI tests (default: mpirun -np 2)],
            [MPI_TESTS], [[[=DRIVER]]])
if test "$HAVE_PKG_MPI" = yes ; then
  if test "$withval" = yes ; then
    withval="mpirun -np 2"
  elif test "$withval" = no ; then
    withval=""
  fi
  AC_SUBST([$1_MPI_TESTS_ENVIRONMENT], [$withval])
fi

dnl Finalize the CC and F77 variables and run a C test program
dnl Also determine if MPI I/O can be used safely
if test "$HAVE_PKG_MPI" = yes ; then
  if test -z "$MPI_CC" -a -z "$MPI_CC_NONE" ; then
    MPI_CC=mpicc
  fi
  if test -n "$MPI_CC" ; then
    CC="$MPI_CC"
  fi
  echo "                             MPI_CC set to $MPI_CC"

m4_ifset([SC_CHECK_MPI_CXX], [
  if test -z "$MPI_CXX" -a -z "$MPI_CXX_NONE" ; then
    MPI_CXX=mpicxx
  fi
  if test -n "$MPI_CXX" ; then
    CXX="$MPI_CXX"
  fi
  echo "                            MPI_CXX set to $MPI_CXX"
])

  if test -z "$MPI_F77" -a -z "$MPI_F77_NONE" ; then
    MPI_F77=mpif77
  fi
  if test -n "$MPI_F77" ; then
    F77="$MPI_F77"
  fi
  echo "                            MPI_F77 set to $MPI_F77"

  AC_DEFINE([MPI], 1, [Define to 1 if we are using MPI])
else
  unset MPI_CC
  unset MPI_CXX
  unset MPI_F77
fi
AM_CONDITIONAL([$1_MPI], [test "$HAVE_PKG_MPI" = yes])
])

dnl SC_MPI_C_COMPILE_AND_LINK([action-if-successful], [action-if-failed])
dnl Compile and link an MPI C test program
dnl
AC_DEFUN([SC_MPI_C_COMPILE_AND_LINK],
[
AC_MSG_CHECKING([compile/link for MPI C program])
AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#undef MPI
#include <mpi.h>
]], [[
MPI_Init ((int *) 0, (char ***) 0);
MPI_Finalize ();
]])],
[AC_MSG_RESULT([successful])
 $1],
[AC_MSG_RESULT([failed])
 $2])
])

dnl SC_MPI_CXX_COMPILE_AND_LINK([action-if-successful], [action-if-failed])
dnl Compile and link an MPI CXX test program
dnl
AC_DEFUN([SC_MPI_CXX_COMPILE_AND_LINK],
[
AC_MSG_CHECKING([compile/link for MPI CXX program])
AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#undef MPI
#include <mpi.h>
#include <iostream>
]], [[
std::cout << "Hello C++ MPI" << std::endl;
MPI_Init ((int *) 0, (char ***) 0);
MPI_Finalize ();
]])],
[AC_MSG_RESULT([successful])
 $1],
[AC_MSG_RESULT([failed])
 $2])
])

dnl SC_MPIIO_C_COMPILE_AND_LINK([action-if-successful], [action-if-failed])
dnl Compile and link an MPI I/O test program
dnl
AC_DEFUN([SC_MPIIO_C_COMPILE_AND_LINK],
[
AC_MSG_CHECKING([compile/link for MPI I/O C program])
AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#undef MPI
#include <mpi.h>
]], [[
MPI_File fh;
MPI_Init ((int *) 0, (char ***) 0);
MPI_File_open (MPI_COMM_WORLD, "filename",
               MPI_MODE_WRONLY | MPI_MODE_APPEND,
               MPI_INFO_NULL, &fh);
MPI_File_close (&fh);
MPI_Finalize ();
]])],
[AC_MSG_RESULT([successful])
 $1],
[AC_MSG_RESULT([failed])
 $2])
])

dnl SC_MPI_INCLUDES
dnl Call the compiler with various --show* options
dnl to figure out the MPI_INCLUDES and MPI_INCLUDE_PATH varables
dnl
AC_DEFUN([SC_MPI_INCLUDES],
[
MPI_INCLUDES=
MPI_INCLUDE_PATH=
if test "$HAVE_PKG_MPI" = yes ; then
  echo "Trying to determine MPI_INCLUDES"
  for SHOW in -showme:compile -showme:incdirs -showme -show ; do
    if test -z "$MPI_INCLUDES" ; then
      echo -n "   Trying compile argument $SHOW... "
      if MPI_CC_RESULT=`$CC $SHOW 2> /dev/null` ; then
        echo "Successful"
        for CCARG in $MPI_CC_RESULT ; do
          MPI_INCLUDES="$MPI_INCLUDES `echo $CCARG | grep '^-I'`"
        done
      else
        echo "Failed"
      fi
    fi
  done
  if test -n "$MPI_INCLUDES" ; then
    MPI_INCLUDES=`echo $MPI_INCLUDES | sed -e 's/^ *//' -e 's/  */ /g'`
    echo "   Found MPI_INCLUDES $MPI_INCLUDES"
  fi
  if test -n "$MPI_INCLUDES" ; then
    MPI_INCLUDE_PATH=`echo $MPI_INCLUDES | sed -e 's/^-I//'`
    MPI_INCLUDE_PATH=`echo $MPI_INCLUDE_PATH | sed -e 's/-I/:/g'`
    echo "   Found MPI_INCLUDE_PATH $MPI_INCLUDE_PATH"
  fi
fi
AC_SUBST([MPI_INCLUDES])
AC_SUBST([MPI_INCLUDE_PATH])
])

dnl SC_MPI
dnl Configure MPI and check its C (and optionally CXX) compiler in one line
dnl
AC_DEFUN([SC_MPI],
[
SC_MPI_CONFIG([$1], [$3])       dnl possibly defines macro SC_CHECK_MPI_CXX
AC_PROG_CC([$2])
m4_ifset([SC_CHECK_MPI_CXX], [AC_PROG_CXX([$3])])

dnl compile and link tests must be done after the PROC_CC line
if test "$HAVE_PKG_MPI" = yes ; then
  SC_MPI_C_COMPILE_AND_LINK(, [AC_MSG_ERROR([MPI C test failed])])
  m4_ifset([SC_CHECK_MPI_CXX], [
    AC_LANG_PUSH([C++])
    SC_MPI_CXX_COMPILE_AND_LINK(, [AC_MSG_ERROR([MPI CXX test failed])])
    AC_LANG_POP([C++])
  ])
  if test "$HAVE_PKG_MPIIO" = yes ; then
    SC_MPIIO_C_COMPILE_AND_LINK(
      [AC_DEFINE([MPIIO], 1, [Define to 1 if we are using MPI I/O])],
      [AC_MSG_ERROR([MPI I/O specified but not found])])
  fi
fi
AM_CONDITIONAL([$1_MPIIO], [test "$HAVE_PKG_MPIIO" = yes])

dnl figure out the MPI include directories
SC_MPI_INCLUDES
])
