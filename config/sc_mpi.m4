dnl
dnl SC_MPI_CONFIG(PREFIX[, true])
dnl
dnl If the second argument is given, also includes configuration for C++.
dnl Checks the configure options
dnl --enable-mpi    If enabled, set AC_DEFINE and AC_CONDITIONAL.
dnl                 If enabled and CC is not set, do export CC=mpicc.
dnl                 This may be "too late" if AC_PROG_CC was called earlier.
dnl                 In that case you need to set CC=mpicc (or other compiler)
dnl                 on the configure command line. Same for F77, CXX.
dnl --enable-mpiio  Enables AC_DEFINE and test for MPI I/O.
dnl --with-mpitest=...  Specify alternate test driver (default mpirun -np 2).
dnl
dnl SC_MPI_ENGAGE(PREFIX)
dnl
dnl Relies on SC_MPI_CONFIG to be called before.
dnl Calls AC_PROG_CC and AC_PROG_CXX if C++ configuration is enabled.
dnl Does compile tests for MPI and MPI I/O if this is enabled.
dnl
dnl These macros are separate because AC_REQUIRE(AC_PROG_CC) will expand
dnl the AC_PROG_CC macro before entering SC_MPI_ENGAGE.

AC_DEFUN([SC_MPI_CONFIG],
[
HAVE_PKG_MPI=no
HAVE_PKG_MPIIO=no
m4_ifval([$2], [m4_define([SC_CHECK_MPI_CXX], [yes])])

dnl The shell variable SC_ENABLE_MPI is set
dnl unless it is provided by the environment.
dnl Therefore all further checking uses the HAVE_PKG_MPI shell variable
dnl and neither AC_DEFINE nor AM_CONDITIONAL are invoked at this point.
AC_ARG_ENABLE([mpi],
              [AS_HELP_STRING([--enable-mpi], [enable MPI])],,
              [enableval=no])
SC_ARG_OVERRIDE_ENABLE([$1], [MPI])
if test "$enableval" = yes ; then
  HAVE_PKG_MPI=yes
elif test "$enableval" != no ; then
  AC_MSG_ERROR([Please use --enable-mpi without an argument])
fi
AC_MSG_CHECKING([whether we are using MPI])
AC_MSG_RESULT([$HAVE_PKG_MPI])

dnl The shell variable SC_ENABLE_MPIIO is set
dnl unless it is provided by the environment.
dnl If enabled, MPI I/O will be verified by a compile/link test below.
AC_ARG_ENABLE([mpiio],
              [AS_HELP_STRING([--enable-mpiio], [enable MPI I/O])],,
              [enableval=no])
SC_ARG_OVERRIDE_ENABLE([$1], [MPIIO])
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

dnl Set compilers if not already set and set define and conditionals
if test "$HAVE_PKG_MPI" = yes ; then
  if test -z "$CC" ; then
    export CC=mpicc
  fi
  echo "                             CC set to $CC"
m4_ifset([SC_CHECK_MPI_CXX], [
  if test -z "$CXX" ; then
    export CXX=mpicxx
  fi
  echo "                            CXX set to $CXX"
])
  if test -z "$F77" ; then
    export F77=mpif77
  fi
  echo "                            F77 set to $F77"
  AC_DEFINE([MPI], 1, [Define to 1 if we are using MPI])
  AC_DEFINE([MPIIO], 1, [Define to 1 if we are using MPI I/O])
fi
AM_CONDITIONAL([$1_MPI], [test "$HAVE_PKG_MPI" = yes])
AM_CONDITIONAL([$1_MPIIO], [test "$HAVE_PKG_MPIIO" = yes])
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

AC_DEFUN([SC_MPI_ENGAGE],
[
dnl determine compilers
AC_REQUIRE([AC_PROG_CC])
m4_ifset([SC_CHECK_MPI_CXX], [AC_REQUIRE([AC_PROG_CXX])])

dnl compile and link tests must be done after the AC_PROC_CC lines
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

dnl figure out the MPI include directories
SC_MPI_INCLUDES
])
