
dnl SC_CHECK_OPENMP(PREFIX)
dnl Check for OpenMP support and link a test program
dnl
AC_DEFUN([SC_CHECK_OPENMP], [

SC_CHECK_LIB([gomp], [omp_set_num_threads], [OPENMP], [$1])
AC_MSG_CHECKING([for OpenMP])

SC_ARG_ENABLE_PREFIX([openmp],
  [enable OpenMP (optionally use --enable-openmp=<OPENMP_CFLAGS>)],
  [OPENMP], [$1])
if test "x$$1_ENABLE_OPENMP" != xno ; then
  $1_OPENMP_CFLAGS=-fopenmp
  if test "x$$1_ENABLE_OPENMP" != xyes ; then
    $1_OPENMP_CFLAGS="$$1_ENABLE_OPENMP"
    dnl AC_MSG_ERROR([Please provide --enable-openmp without arguments])
  fi
  PRE_OPENMP_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $$1_OPENMP_CFLAGS"
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
