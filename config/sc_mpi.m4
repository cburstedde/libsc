dnl
dnl SC_MPI_CONFIG(PREFIX, , )
dnl
dnl If the second argument is nonempty, also includes configuration for F77 and FC.
dnl If the third argument is nonempty, also includes configuration for CXX.
dnl
dnl Checks the configure options
dnl --enable-mpi      If enabled and CC is not set, do export CC=mpicc.
dnl                   This may be "too late" if AC_PROG_CC was called earlier.
dnl                   In that case you need to set CC=mpicc (or other compiler)
dnl                   on the configure command line.
dnl                   Likewise for F77, FC and CXX if enabled in SC_MPI_CONFIG.
dnl --disable-mpiio   Only effective if --enable-mpi is given.  In this case,
dnl                   do not use MPI I/O in sc and skip the compile-and-link test.
dnl                   DEPRECATED: we will disallow this combination in the future.
dnl --disable-mpithread Only effective if --enable-mpi is given.  In this case,
dnl                   do not use MPI_Init_thread () and skip compile-and-link test.
dnl --disable-mpishared Only effective if --enable-mpi is given.  In this case,
dnl                   disable MPI shared windows and split node communicators.
dnl                   If not disabled, decide support by compile-and-link test
dnl                   of both shared windows and split node communicators.
dnl
dnl If MPI is enabled, set AC_DEFINE and AC_CONDITIONAL for PREFIX_ENABLE_MPI.
dnl If MPI I/O is not disabled, set these for PREFIX_ENABLE_MPIIO.
dnl If MPI_Init_thread is not disabled, set these for PREFIX_ENABLE_MPITHREAD.
dnl If MPI shared nodes are supported, set these for PREFIX_ENABLE_MPISHARED.
dnl If MPI split by socket is supported, set these for PREFIX_ENABLE_MPISOCKET.
dnl
dnl SC_MPI_ENGAGE(PREFIX)
dnl
dnl Relies on SC_MPI_CONFIG to be called before.
dnl Calls AC_PROG_CC and other macros related to the C compiler.
dnl Calls AC_PROG_F77 and others if F77 is enabled in SC_MPI_CONFIG.
dnl Calls AC_PROG_FC and others if FC is enabled in SC_MPI_CONFIG.
dnl Calls AC_PROG_CXX and others if CXX is enabled in SC_MPI_CONFIG.
dnl
dnl If MPI is enabled, a compile-and-link test is performed.  It aborts
dnl configuration on failure.
dnl If MPI is enabled and I/O is not disabled, a compile-and-link test
dnl for MPI I/O is performed.  It aborts configuration on failure.
dnl If MPI is enabled and MPITHREAD is not disabled, a compile-and-link test
dnl for MPI_Init_thread is performed.  It aborts configuration on failure.
dnl
dnl These macros are separate because of the AC_REQUIRE logic inside autoconf.

AC_DEFUN([SC_MPI_CONFIG],
[
HAVE_PKG_MPI=no
HAVE_PKG_MPIIO=no
HAVE_PKG_MPITHREAD=no
HAVE_PKG_MPISHARED=no
m4_ifval([$2], [m4_define([SC_CHECK_MPI_F77], [yes])])
m4_ifval([$2], [m4_define([SC_CHECK_MPI_FC], [yes])])
m4_ifval([$3], [m4_define([SC_CHECK_MPI_CXX], [yes])])

dnl The shell variable SC_ENABLE_MPI is set if --enable-mpi is given.
dnl Therefore all further checking uses the HAVE_PKG_MPI shell variable
dnl and neither AC_DEFINE nor AM_CONDITIONAL are invoked at this point.
AC_ARG_ENABLE([mpi],
              [AS_HELP_STRING([--enable-mpi],
               [enable MPI (force serial code otherwise)])],,
              [enableval=no])
if test "x$enableval" = xyes ; then
  HAVE_PKG_MPI=yes
elif test "x$enableval" != xno ; then
  AC_MSG_ERROR([use --enable-mpi without an argument, or with yes or no])
fi
AC_MSG_CHECKING([whether we are using MPI])
AC_MSG_RESULT([$HAVE_PKG_MPI])
AC_SUBST([HAVE_PKG_MPI])

dnl The shell variable SC_ENABLE_MPIIO is set if --disable-mpiio is not given.
dnl If not disabled, MPI I/O will be verified by a compile/link test below.
AC_ARG_ENABLE([mpiio],
              [AS_HELP_STRING([--disable-mpiio],
               [do not use MPI I/O (this option is DEPRECATED)])],,
              [enableval=yes])
if test "x$enableval" = xyes ; then
  if test "x$HAVE_PKG_MPI" = xyes ; then
    HAVE_PKG_MPIIO=yes
  fi
elif test "x$enableval" != xno ; then
  AC_MSG_ERROR([use --disable-mpiio without an argument (option DEPRECATED)])
fi
AC_MSG_CHECKING([whether we are using MPI I/O])
AC_MSG_RESULT([$HAVE_PKG_MPIIO])

dnl The variable SC_ENABLE_MPITHREAD is set if --disable-mpithread not given.
dnl If not disabled, MPI_Init_thread will be verified by a compile/link test.
AC_ARG_ENABLE([mpithread],
              [AS_HELP_STRING([--disable-mpithread],
               [do not use MPI_Init_thread (even if MPI is enabled)])],,
              [enableval=yes])
if test "x$enableval" = xyes ; then
  if test "x$HAVE_PKG_MPI" = xyes ; then
    HAVE_PKG_MPITHREAD=yes
  fi
elif test "x$enableval" != xno ; then
  AC_MSG_WARN([Ignoring --enable-mpithread with unsupported argument])
fi
AC_MSG_CHECKING([whether we are using MPI_Init_thread])
AC_MSG_RESULT([$HAVE_PKG_MPITHREAD])

dnl The variable SC_ENABLE_MPISHARED is set if --disable-mpishared not given.
dnl If not disabled, run the compile-and-link tests further down below for
dnl SC_MPIWINSHARED_C_COMPILE_AND_LINK and SC_MPICOMMSHARED_C_COMPILE_AND_LINK
dnl and enable node communicators and shared window support if both go through.
AC_ARG_ENABLE([mpishared],
              [AS_HELP_STRING([--disable-mpishared],
               [do not use MPI shared nodes (even if MPI is enabled)])],,
              [enableval=yes])
if test "x$enableval" = xyes ; then
  if test "x$HAVE_PKG_MPI" = xyes ; then
    HAVE_PKG_MPISHARED=yes
  fi
elif test "x$enableval" != xno ; then
  AC_MSG_WARN([Ignoring --enable-mpishared with unsupported argument])
fi

dnl We allow the user to specify the configure option --enable-valgrind.
dnl If given and valgrind is found, we use it to run tests on make check.
dnl If given and valgrind is not found, we abort with an error message.
dnl If not given, we run tests in the default manner (ignoring valgrind).
dnl If given and found, we add the #define PREFIX_ENABLE_VALGRIND.
$1_VALGRIND=
$1_VALGRIND_FLAGS=
AC_ARG_ENABLE([valgrind],
              [AS_HELP_STRING([--enable-valgrind],
               [use valgrind with make check to run tests])],,
              [enableval=no])
if test "x$enableval" = xyes ; then
  AC_CHECK_PROG([$1_VALGRIND], [valgrind], [valgrind])
  if test "x$$1_VALGRIND" != xvalgrind ; then
    AC_MSG_ERROR([--enable-valgrind given but valgrind not found in PATH])
  else
    AC_DEFINE([ENABLE_VALGRIND], 1, [Enable valgrind in executing tests])
  fi
  $1_VALGRIND_FLAGS="--error-exitcode=1"
fi
dnl We are not making $1_VALGRIND a precious variable for simplicity
dnl AC_ARG_VAR([$1_VALGRIND], [valgrind wrapper to invoke for tests on make check])
AC_SUBST([$1_VALGRIND])
AC_SUBST([$1_VALGRIND_FLAGS])

dnl Establish the MPI test environment
$1_MPIRUN=
$1_MPI_TEST_FLAGS=
if test "x$HAVE_PKG_MPI" = xyes ; then
  AC_CHECK_PROGS([$1_MPIRUN], [mpirun mpiexec])
  if test "x$$1_MPIRUN" = xmpirun ; then
    $1_MPI_TEST_FLAGS="-np 2"
  elif test "x$$1_MPIRUN" = xmpiexec ; then
    $1_MPI_TEST_FLAGS="-n 2"
  else
    $1_MPIRUN=
  fi
fi
dnl We are not using precious variables for simplicity
dnl AC_ARG_VAR([$1_MPIRUN], [mpirun wrapper to invoke for tests on make check])
AC_SUBST([$1_MPIRUN])
dnl AC_ARG_VAR([$1_MPI_TEST_FLAGS], [arguments passed to mpirun wrapper on make check])
AC_SUBST([$1_MPI_TEST_FLAGS])

dnl Set compilers if not already set and set define and conditionals
if test "x$HAVE_PKG_MPI" = xyes ; then
m4_ifset([SC_CHECK_MPI_F77], [
  if test "x$F77" = x ; then
    export F77=mpif77
  fi
  AC_MSG_NOTICE([                            F77 set to $F77])
])
m4_ifset([SC_CHECK_MPI_FC], [
  if test "x$FC" = x ; then
    export FC=mpif90
  fi
  AC_MSG_NOTICE([                             FC set to $FC])
])
  if test "x$CC" = x ; then
    export CC=mpicc
  fi
  AC_MSG_NOTICE([                             CC set to $CC])
m4_ifset([SC_CHECK_MPI_CXX], [
  if test "x$CXX" = x ; then
    export CXX=mpicxx
  fi
  AC_MSG_NOTICE([                            CXX set to $CXX])
])
  AC_DEFINE([MPI], 1, [DEPRECATED (use $1_ENABLE_MPI instead)])
  AC_DEFINE([ENABLE_MPI], 1, [Define to 1 if we are using MPI])
  if test "x$HAVE_PKG_MPIIO" = xyes ; then
    AC_DEFINE([MPIIO], 1, [DEPRECATED (use $1_ENABLE_MPIIO instead)])
    AC_DEFINE([ENABLE_MPIIO], 1, [Define to 1 if we are using MPI I/O])
  fi
  if test "x$HAVE_PKG_MPITHREAD" = xyes ; then
    AC_DEFINE([ENABLE_MPITHREAD], 1, [Define to 1 if we are using MPI_Init_thread])
  fi
else
m4_ifset([SC_CHECK_MPI_F77], [
  if test "x$F77" = x ; then
    AC_CHECK_PROGS([$1_F77_COMPILER], [gfortran g77 f77 ifort])
    if test "x$$1_F77_COMPILER" != x ; then
      F77="$$1_F77_COMPILER"
    fi
  fi
], [:])
m4_ifset([SC_CHECK_MPI_FC], [
  if test "x$FC" = x ; then
    AC_CHECK_PROGS([$1_FC_COMPILER], [gfortran ifort])
    if test "x$$1_FC_COMPILER" != x ; then
      FC="$$1_FC_COMPILER"
    fi
  fi
], [:])
fi
AM_CONDITIONAL([$1_ENABLE_MPI], [test "x$HAVE_PKG_MPI" = xyes])
AM_CONDITIONAL([$1_ENABLE_MPIIO], [test "x$HAVE_PKG_MPIIO" = xyes])
AM_CONDITIONAL([$1_ENABLE_MPITHREAD], [test "x$HAVE_PKG_MPITHREAD" = xyes])
])

dnl SC_MPI_F77_COMPILE_AND_LINK([action-if-successful], [action-if-failed])
dnl Compile and link an MPI F77 test program
dnl
dnl DEACTIVATED since it triggers a bug in autoconf:
dnl AC_LANG_PROGRAM(Fortran 77): ignoring PROLOGUE: [
dnl
dnl AC_DEFUN([SC_MPI_F77_COMPILE_AND_LINK],
dnl [
dnl AC_MSG_CHECKING([compile/link for MPI F77 program])
dnl AC_LINK_IFELSE([AC_LANG_PROGRAM(
dnl [[
dnl       include "mpif.h"
dnl ]], [[
dnl       call MPI_INIT (ierror)
dnl       call MPI_COMM_SIZE (MPI_COMM_WORLD, isize, ierror)
dnl       call MPI_COMM_RANK (MPI_COMM_WORLD, irank, ierror)
dnl       print*, isize, irank, ': Hello world'
dnl       call MPI_FINALIZE (ierror)
dnl ]])],
dnl [AC_MSG_RESULT([successful])
dnl  $1],
dnl [AC_MSG_RESULT([failed])
dnl  $2])
dnl ])

dnl SC_MPI_FC_COMPILE_AND_LINK([action-if-successful], [action-if-failed])
dnl Compile and link an MPI FC test program
dnl
dnl DEACTIVATED since it triggers a bug in autoconf:
dnl AC_LANG_PROGRAM(Fortran): ignoring PROLOGUE: [
dnl
dnl AC_DEFUN([SC_MPI_FC_COMPILE_AND_LINK],
dnl [
dnl AC_MSG_CHECKING([compile/link for MPI FC program])
dnl AC_LINK_IFELSE([AC_LANG_PROGRAM(
dnl [[
dnl       include "mpif90.h"
dnl ]], [[
dnl       call MPI_INIT (ierror)
dnl       call MPI_COMM_SIZE (MPI_COMM_WORLD, isize, ierror)
dnl       call MPI_COMM_RANK (MPI_COMM_WORLD, irank, ierror)
dnl       print*, isize, irank, ': Hello world'
dnl       call MPI_FINALIZE (ierror)
dnl ]])],
dnl [AC_MSG_RESULT([successful])
dnl  $1],
dnl [AC_MSG_RESULT([failed])
dnl  $2])
dnl ])

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
int mpiret =
MPI_Init ((int *) 0, (char ***) 0);
if (mpiret == MPI_ERR_ARG ||
    mpiret == MPI_ERR_COUNT ||
    mpiret == MPI_ERR_UNKNOWN ||
    mpiret == MPI_ERR_OTHER ||
    mpiret == MPI_ERR_NO_MEM) {
  mpiret = MPI_SUCCESS;
}
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

dnl SC_MPI_Aint_Diff_C_COMPILE_AND_LINK([action-if-successful], [action-if-failed])
dnl Compile and link an MPI_Aint_diff test program.
dnl
AC_DEFUN([SC_MPI_Aint_Diff_C_COMPILE_AND_LINK],
[
AC_MSG_CHECKING([compile/link for MPI_Aint_diff C program])
AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#undef MPI
#include <mpi.h>
]], [[
MPI_Aint a, b, res;
a = 42;
b = 12;
MPI_Init ((int *) 0, (char ***) 0);
res = MPI_Aint_diff (a, b);
MPI_Finalize ();
]])],
[AC_MSG_RESULT([successful])
 $1],
[AC_MSG_RESULT([failed])
 $2])
])

dnl SC_MPI_DATA_TYPE_C_COMPILE_AND_LINK([data-type], [action-if-successful],
dnl                                     [action-if-failed])
dnl Compile and link an MPI data type test program.
dnl
AC_DEFUN([SC_MPI_DATA_TYPE_C_COMPILE_AND_LINK],
[
AC_MSG_CHECKING([compile/link for the MPI data type $1])
AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#undef MPI
#include <mpi.h>
]], [[
int size;
MPI_Init ((int *) 0, (char ***) 0);
/* check if $1 is defined */
MPI_Type_size ($1, &size);
MPI_Finalize ();
]])],
[AC_MSG_RESULT([successful])
 $2],
[AC_MSG_RESULT([failed])
 $3])
])

dnl SC_MPI_CHECK_TYPE([prefix], [data-type])
dnl Checks if an MPI data type is available.
AC_DEFUN([SC_MPI_CHECK_TYPE], [
  $1_HAVE_$2=yes
  SC_MPI_DATA_TYPE_C_COMPILE_AND_LINK([$2], , [$1_HAVE_$2=no])
  if test "x$$1_HAVE_$2" = xyes; then
    AC_DEFINE([HAVE_$2], 1, [Define to 1 if we have $2])
  fi
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
int mpiret;
MPI_File fh;
MPI_Init ((int *) 0, (char ***) 0);
mpiret =
MPI_File_open (MPI_COMM_WORLD, "filename",
               MPI_MODE_WRONLY | MPI_MODE_APPEND,
               MPI_INFO_NULL, &fh);
if (mpiret == MPI_ERR_FILE ||
    mpiret == MPI_ERR_NOT_SAME ||
    mpiret == MPI_ERR_AMODE ||
    mpiret == MPI_ERR_UNSUPPORTED_DATAREP ||
    mpiret == MPI_ERR_UNSUPPORTED_OPERATION ||
    mpiret == MPI_ERR_NO_SUCH_FILE ||
    mpiret == MPI_ERR_FILE_EXISTS ||
    mpiret == MPI_ERR_BAD_FILE ||
    mpiret == MPI_ERR_ACCESS ||
    mpiret == MPI_ERR_NO_SPACE ||
    mpiret == MPI_ERR_QUOTA ||
    mpiret == MPI_ERR_READ_ONLY ||
    mpiret == MPI_ERR_FILE_IN_USE ||
    mpiret == MPI_ERR_DUP_DATAREP ||
    mpiret == MPI_ERR_CONVERSION ||
    mpiret == MPI_ERR_IO) {
  mpiret = MPI_SUCCESS;
}
MPI_File_close (&fh);
MPI_Finalize ();
]])],
[AC_MSG_RESULT([successful])
 $1],
[AC_MSG_RESULT([failed])
 $2])
])

dnl SC_MPITHREAD_C_COMPILE_AND_LINK([action-if-successful], [action-if-failed])
dnl Compile and link an MPI_Init_thread test program
dnl
AC_DEFUN([SC_MPITHREAD_C_COMPILE_AND_LINK],
[
AC_MSG_CHECKING([compile/link for MPI_Init_thread C program])
AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#undef MPI
#include <mpi.h>
]], [[
int mpithr;
MPI_Init_thread ((int *) 0, (char ***) 0, MPI_THREAD_MULTIPLE, &mpithr);
MPI_Finalize ();
]])],
[AC_MSG_RESULT([successful])
 $1],
[AC_MSG_RESULT([failed])
 $2])
])

dnl SC_MPIWINSHARED_C_COMPILE_AND_LINK([action-if-successful], [action-if-failed])
dnl Compile and link an MPI_Win_allocate_shared test program
dnl
AC_DEFUN([SC_MPIWINSHARED_C_COMPILE_AND_LINK],
[
AC_MSG_CHECKING([compile/link for MPI_Win_allocate_shared C program])
AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#undef MPI
#include <mpi.h>
]], [[
int disp_unit=0;
char *baseptr;
MPI_Win win;
MPI_Init ((int *) 0, (char ***) 0);
MPI_Win_allocate_shared(0,disp_unit,MPI_INFO_NULL,MPI_COMM_WORLD,
                        (void *) &baseptr,&win);
MPI_Win_shared_query(win,0,0,&disp_unit,(void *) &baseptr);
MPI_Win_lock(MPI_LOCK_EXCLUSIVE,0,MPI_MODE_NOCHECK,win);
MPI_Win_unlock(0,win);
MPI_Win_free(&win);
MPI_Finalize ();
]])],
[AC_MSG_RESULT([successful])
 $1],
[AC_MSG_RESULT([failed])
 $2])
])

dnl SC_MPICOMMSHARED_C_COMPILE_AND_LINK([action-if-successful], [action-if-failed])
dnl Compile and link an MPI_COMM_TYPE_SHARED test program
dnl
AC_DEFUN([SC_MPICOMMSHARED_C_COMPILE_AND_LINK],
[
AC_MSG_CHECKING([compile/link for MPI_COMM_TYPE_SHARED C program])
AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#undef MPI
#include <mpi.h>
]], [[
MPI_Comm subcomm;
MPI_Init ((int *) 0, (char ***) 0);
MPI_Comm_split_type(MPI_COMM_WORLD,MPI_COMM_TYPE_SHARED,0,MPI_INFO_NULL,&subcomm);
MPI_Finalize ();
]])],
[AC_MSG_RESULT([successful])
 $1],
[AC_MSG_RESULT([failed])
 $2])
])

dnl SC_OMPICOMMSOCKET_C_COMPILE_AND_LINK([action-if-successful], [action-if-failed])
dnl Compile and link an OMPI_COMM_TYPE_SOCKET test program
dnl
AC_DEFUN([SC_OMPICOMMSOCKET_C_COMPILE_AND_LINK],
[
AC_MSG_CHECKING([compile/link for OMPI_COMM_TYPE_SOCKET C program])
AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#undef MPI
#include <mpi.h>
]], [[
MPI_Comm subcomm;
MPI_Init ((int *) 0, (char ***) 0);
MPI_Comm_split_type(MPI_COMM_WORLD,OMPI_COMM_TYPE_SOCKET,0,MPI_INFO_NULL,&subcomm);
MPI_Finalize ();
]])],
[AC_MSG_RESULT([successful])
 $1],
[AC_MSG_RESULT([failed])
 $2])
])

dnl This was only used for our lint code, which needs to be replaced.
dnl
dnl dnl SC_MPI_INCLUDES
dnl dnl Call the compiler with various --show* options
dnl dnl to figure out the MPI_INCLUDES and MPI_INCLUDE_PATH varables
dnl dnl
dnl AC_DEFUN([SC_MPI_INCLUDES],
dnl [
dnl MPI_INCLUDES=
dnl MPI_INCLUDE_PATH=
dnl if test "x$HAVE_PKG_MPI" = xyes ; then
dnl   AC_MSG_NOTICE([Trying to determine MPI_INCLUDES])
dnl   for SHOW in -showme:compile -showme:incdirs -showme -show ; do
dnl     if test "x$MPI_INCLUDES" = x ; then
dnl       AC_MSG_CHECKING([$SHOW])
dnl       if MPI_CC_RESULT=`$CC $SHOW 2> /dev/null` ; then
dnl         AC_MSG_RESULT([Successful])
dnl         for CCARG in $MPI_CC_RESULT ; do
dnl           MPI_INCLUDES="$MPI_INCLUDES `echo $CCARG | grep '^-I'`"
dnl         done
dnl       else
dnl         AC_MSG_RESULT([Failed])
dnl       fi
dnl     fi
dnl   done
dnl   if test "x$MPI_INCLUDES" != x; then
dnl     MPI_INCLUDES=`echo $MPI_INCLUDES | sed -e 's/^ *//' -e 's/  */ /g'`
dnl     AC_MSG_NOTICE([   Found MPI_INCLUDES $MPI_INCLUDES])
dnl   fi
dnl   if test "x$MPI_INCLUDES" != x ; then
dnl     MPI_INCLUDE_PATH=`echo $MPI_INCLUDES | sed -e 's/^-I//'`
dnl     MPI_INCLUDE_PATH=`echo $MPI_INCLUDE_PATH | sed -e 's/-I/:/g'`
dnl     AC_MSG_NOTICE([   Found MPI_INCLUDE_PATH $MPI_INCLUDE_PATH])
dnl   fi
dnl fi
dnl AC_SUBST([MPI_INCLUDES])
dnl AC_SUBST([MPI_INCLUDE_PATH])
dnl ])

AC_DEFUN([SC_MPI_ENGAGE],
[
dnl determine compilers
m4_ifset([SC_CHECK_MPI_F77], [
AC_REQUIRE([AC_PROG_F77])
AC_PROG_F77_C_O
AC_REQUIRE([AC_F77_LIBRARY_LDFLAGS])
AC_F77_WRAPPERS
])
m4_ifset([SC_CHECK_MPI_FC], [
AC_REQUIRE([AC_PROG_FC])
AC_PROG_FC_C_O
AC_REQUIRE([AC_FC_LIBRARY_LDFLAGS])
AC_FC_WRAPPERS
])
AC_REQUIRE([AC_PROG_CC])
AC_PROG_CC_C_O
AM_PROG_CC_C_O
m4_ifset([SC_CHECK_MPI_CXX], [
AC_REQUIRE([AC_PROG_CXX])
AC_PROG_CXX_C_O
])

dnl compile and link tests must be done after the AC_PROC_CC lines
if test "x$HAVE_PKG_MPI" = xyes ; then
dnl  m4_ifset([SC_CHECK_MPI_F77], [
dnl    AC_LANG_PUSH([Fortran 77])
dnl    SC_MPI_F77_COMPILE_AND_LINK(, [AC_MSG_ERROR([MPI F77 test failed])])
dnl    AC_LANG_POP([Fortran 77])
dnl  ])
dnl  m4_ifset([SC_CHECK_MPI_FC], [
dnl    AC_LANG_PUSH([Fortran])
dnl    SC_MPI_FC_COMPILE_AND_LINK(, [AC_MSG_ERROR([MPI FC test failed])])
dnl    AC_LANG_POP([Fortran])
dnl  ])
  SC_MPI_C_COMPILE_AND_LINK(, [AC_MSG_ERROR([MPI C test failed])])
  m4_ifset([SC_CHECK_MPI_CXX], [
    AC_LANG_PUSH([C++])
    SC_MPI_CXX_COMPILE_AND_LINK(, [AC_MSG_ERROR([MPI CXX test failed])])
    AC_LANG_POP([C++])
  ])
  if test "x$HAVE_PKG_MPIIO" = xyes ; then
    SC_MPIIO_C_COMPILE_AND_LINK(,
      [AC_MSG_ERROR([MPI I/O not found; you may --disable-mpi altogether])])
  fi
  if test "x$HAVE_PKG_MPITHREAD" = xyes ; then
    SC_MPITHREAD_C_COMPILE_AND_LINK(,
      [AC_MSG_ERROR([MPI_Init_thread not found; you may try --disable-mpithread])])
  fi

  dnl Run test to check availability of MPI_Aint_diff
  $1_HAVE_AINT_DIFF=yes
  SC_MPI_Aint_Diff_C_COMPILE_AND_LINK(,[$1_HAVE_AINT_DIFF=no])
  if test "x$$1_HAVE_AINT_DIFF" = xyes ; then
    AC_DEFINE([HAVE_AINT_DIFF], 1,
              [Define to 1 if we have MPI_Aint_diff])
  fi

  dnl Run tests to check availability of newer MPI data types
  SC_MPI_CHECK_TYPE([$1], [MPI_UNSIGNED_LONG_LONG])
  SC_MPI_CHECK_TYPE([$1], [MPI_SIGNED_CHAR])
  SC_MPI_CHECK_TYPE([$1], [MPI_INT8_T])

  dnl Run test to check availability of MPI window
  $1_ENABLE_MPIWINSHARED=yes
  SC_MPIWINSHARED_C_COMPILE_AND_LINK(,[$1_ENABLE_MPIWINSHARED=no])
  if test "x$$1_ENABLE_MPIWINSHARED" = xyes ; then
    AC_DEFINE([ENABLE_MPIWINSHARED], 1,
              [Define to 1 if we can use MPI_Win_allocate_shared])
  fi
  dnl Run test to check availability of MPI split node communicator
  $1_ENABLE_MPICOMMSHARED=yes
  SC_MPICOMMSHARED_C_COMPILE_AND_LINK(,[$1_ENABLE_MPICOMMSHARED=no])
  if test "x$$1_ENABLE_MPICOMMSHARED" = xyes ; then
    AC_DEFINE([ENABLE_MPICOMMSHARED], 1,
              [Define to 1 if we can use MPI_COMM_TYPE_SHARED])
  fi
  dnl Run test to check availability of MPI split socket communicator
  $1_ENABLE_MPICOMMSOCKET=yes
  SC_OMPICOMMSOCKET_C_COMPILE_AND_LINK(,[$1_ENABLE_MPICOMMSOCKET=no])
  if test "x$$1_ENABLE_MPICOMMSOCKET" = xyes ; then
    AC_DEFINE([ENABLE_MPICOMMSOCKET], 1,
              [Define to 1 if we can use OMPI_COMM_TYPE_SOCKET])
  fi
  dnl Deactivate overall MPI 3 code when not available or not configured
  AC_MSG_CHECKING([whether we are using MPI 3 node shared memory])
  if test "x$$1_ENABLE_MPIWINSHARED" != xyes || \
     test "x$$1_ENABLE_MPICOMMSHARED" != xyes ; then
    HAVE_PKG_MPISHARED=no
  fi
  if test "x$HAVE_PKG_MPISHARED" = xyes ; then
    AC_DEFINE([ENABLE_MPISHARED], 1,
              [Define to 1 if we can use MPI split nodes and shared memory])
  fi
  AC_MSG_RESULT([$HAVE_PKG_MPISHARED])
fi
AM_CONDITIONAL([$1_ENABLE_MPISHARED], [test "x$HAVE_PKG_MPISHARED" = xyes])

dnl dnl figure out the MPI include directories
dnl SC_MPI_INCLUDES
])
