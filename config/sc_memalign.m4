
dnl SC_CHECK_MEMALIGN(PREFIX) Let the user specify --with-memalign=X (number of
dnl bytes).  Searches for memalign functions (aligned_alloc (C11),
dnl posix_memalign (POSIX), memalign (POSIX (deprecated))).
dnl
AC_DEFUN([SC_CHECK_MEMALIGN], [

AC_MSG_CHECKING([memory alignment])

SC_ARG_ENABLE_PREFIX([memalign],[use aligned malloc],[MEMALIGN],[$1],[=<NUM_BYTES>])
if test "x$$1_ENABLE_MEMALIGN" != xno ; then
  if test "x$$1_ENABLE_MEMALIGN" = xyes ; then
    AC_MSG_ERROR([Please specify the number of bytes to align allocations])
  fi
  $1_MEMALIGN_BYTES=$$1_ENABLE_MEMALIGN
  AC_DEFINE_UNQUOTED([MEMALIGN_BYTES],[$$1_MEMALIGN_BYTES],[desired alignment (in bytes) of allocations])
  AC_MSG_RESULT([$$1_MEMALIGN_BYTES])
dnl aligned_alloc
  $1_WITH_ALIGNED_ALLOC="yes"
  AC_SEARCH_LIBS([aligned_alloc])
  if test "x$ac_cv_search_aligned_alloc" != "xnone required" ; then
    $1_WITH_ALIGNED_ALLOC=no
  fi
  if test "x$$1_WITH_ALIGNED_ALLOC" = xyes ; then
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include<stdlib.h>]],
[[
int *a = (int *) aligned_alloc($$1_MEMALIGN_BYTES,3*sizeof(*a));
free(a);
]])],
                   [],[$1_WITH_ALIGNED_ALLOC=no])
  fi
  if test "x$$1_WITH_ALIGNED_ALLOC" = xyes ; then
    AC_DEFINE([WITH_ALIGNED_ALLOC],[1],[define to 1 if aligned_alloc() found])
  fi
dnl posix_memalign
  $1_WITH_POSIX_MEMALIGN="yes"
  AC_SEARCH_LIBS([posix_memalign])
  if test "x$ac_cv_search_posix_memalign" != "xnone required" ; then
    $1_WITH_POSIX_MEMALIGN=no
  fi
  if test "x$$1_WITH_POSIX_MEMALIGN" = xyes ; then
    AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#include<stdlib.h>
#include<errno.h>
]],
[[
int *a;
int err = posix_memalign((void **) &a, $$1_MEMALIGN_BYTES,3*sizeof(*a));
free(a);
]])],
                   [],[$1_WITH_POSIX_MEMALIGN=no])
  fi
  if test "x$$1_WITH_POSIX_MEMALIGN" = xyes ; then
    AC_DEFINE([WITH_POSIX_MEMALIGN],[1],[define to 1 if posix_memalign() found])
  fi
dnl memalign
  $1_WITH_MEMALIGN="yes"
  AC_SEARCH_LIBS([memalign])
  if test "x$ac_cv_search_memalign" != "xnone required" ; then
    $1_WITH_MEMALIGN=no
  fi
  if test "x$$1_WITH_MEMALIGN" = xyes ; then
    AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#include<stdlib.h>
]],
[[
int *a = (int *) memalign($$1_MEMALIGN_BYTES,3*sizeof(*a));
free(a);
]])],
                   [],[$1_WITH_MEMALIGN=no])
  fi
  if test "x$$1_WITH_MEMALIGN" = xyes ; then
    AC_DEFINE([WITH_MEMALIGN],[1],[define to 1 if memalign() found])
  fi
else
  AC_MSG_RESULT([not used])
fi
])

