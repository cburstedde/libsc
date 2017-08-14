
dnl SC_CHECK_OPENMP(PREFIX)
dnl Check for OpenMP support and link a test program
dnl
dnl This macro tries to link to omp_get_thread_num both as is and with -lgomp.
dnl If neither of this works, we throw an error.
dnl Use the LIBS variable on the configure line to specify a different library.
dnl
dnl Using --enable-openmp without any argument defaults to -fopenmp.
dnl For different CFLAGS use --enable-openmp="-my-openmp-cflags" or similar.
dnl
AC_DEFUN([SC_CHECK_OPENMP], [

dnl This link test changes the LIBS variable in place for posterity
dnl ... which is bad if openmp is NOT used
dnl SAVE_LIBS="$LIBS"
dnl SC_CHECK_LIB([gomp], [omp_get_thread_num], [OPENMP], [$1])
dnl LIBS="$SAVE_LIBS"
AC_MSG_CHECKING([for OpenMP])

SC_ARG_ENABLE_PREFIX([openmp],
  [enable OpenMP:
     Using --enable-openmp without arguments does not use any CFLAGS
       to supply CFLAGS use --enable-openmp=<OPENMP_CFLAGS>
     We check first for linking without any libraries and then with -lgomp
       to avoid the latter, specify LIBS=<OPENMP_LIBS> on configure line
  ],
  [OPENMP], [$1])
if test "x$$1_ENABLE_OPENMP" != xno ; then
  dnl This is not portable: depends on compiler/architecture
  dnl $1_OPENMP_CFLAGS="-fopenmp"
  $1_OPENMP_CFLAGS=
  if test "x$$1_ENABLE_OPENMP" != xyes ; then
    $1_OPENMP_CFLAGS="$$1_ENABLE_OPENMP"
  fi
  PRE_OPENMP_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $$1_OPENMP_CFLAGS"
  SC_CHECK_LIB_NOCOND([gomp], [omp_get_thread_num], [OPENMP], [$1])
  AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#include <omp.h>
]],[[
  omp_set_num_threads (2);
  #pragma omp parallel
  {
    int id = omp_get_thread_num ();
  }
]])],,
                 [AC_MSG_ERROR([Unable to link with OpenMP])])
dnl Keep the variables changed as done above
dnl CFLAGS="$PRE_OPENMP_CFLAGS"

  AC_MSG_RESULT([successful])
else
  AC_MSG_RESULT([not used])
fi
])
