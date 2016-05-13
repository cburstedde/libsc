
dnl SC_CHECK_MEMALIGN(PREFIX)
dnl Let the user specify --enable-memalign[=X] or --disable-memalign.
dnl The alignment argument X must be a multiple of sizeof (void *).
dnl
dnl The default is --enable-memalign, which sets X to (sizeof (void *)).
dnl
dnl This macro also searches for the aligned allocation functions aligned_alloc
dnl (C11 / glibc >= 2.16) and posix_memalign (POSIX / glibc >= 2.1.91) and
dnl defines SC_HAVE_ALIGNED_ALLOC and SC_HAVE_POSIX_MEMALIGN, respectively.
dnl If found and alignment is enabled, this macro runs the link tests.
dnl
dnl If memory alignment is selected, the sc_malloc calls and friends will
dnl use the aligned version, relying on posix_memalign if it exists.
dnl
AC_DEFUN([SC_CHECK_MEMALIGN], [

dnl check for presence of functions
AC_CHECK_FUNCS([aligned_alloc posix_memalign])

dnl custom memory alignment option
AC_MSG_CHECKING([for memory alignment option])
SC_ARG_DISABLE_PREFIX([memalign],
  [use aligned malloc (optionally use --enable-memalign=<bytes>)],
  [MEMALIGN], [$1])

dnl read the value of the configuration argument
if test "x$$1_ENABLE_MEMALIGN" != xno ; then
  if test "x$$1_ENABLE_MEMALIGN" != xyes ; then

    dnl make sure the alignment is a number 
    $1_MEMALIGN_BYTES=`echo "$$1_ENABLE_MEMALIGN" | tr -c -d '[[:digit:]]'`
    $1_MEMALIGN_BYTES_LINK="$$1_MEMALIGN_BYTES"
    if test "x$$1_MEMALIGN_BYTES" = x ; then
      AC_MSG_ERROR([Please provide --enable-memalign with a numeric value or nothing])
    fi
  else
    $1_MEMALIGN_BYTES_LINK="SIZEOF_VOID_P"
    $1_MEMALIGN_BYTES="$1_$$1_MEMALIGN_BYTES_LINK"
  fi
  AC_DEFINE_UNQUOTED([MEMALIGN_BYTES], [($$1_MEMALIGN_BYTES)],
                     [desired alignment of allocations in bytes])
  AC_MSG_RESULT([$$1_MEMALIGN_BYTES])

dnl verify that aligned_alloc can be linked against
  if test "x$ac_cv_func_aligned_alloc" = xyes ; then
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdlib.h>]],
[[
int *a = (int *) aligned_alloc ($$1_MEMALIGN_BYTES_LINK, 3 * sizeof(*a));
free(a);
]])],
                   [], [AC_MSG_ERROR([Linking aligned_alloc failed])])
  fi

dnl verify that posix_memalign can be linked against
  if test "x$ac_cv_func_posix_memalign" = xyes ; then
    AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#include<stdlib.h>
#include<errno.h>
]],
[[
int *a;
int err = posix_memalign((void **) &a, $$1_MEMALIGN_BYTES_LINK, 3 * sizeof(*a));
free(a);
]])],
                   [], [AC_MSG_ERROR([Linking posix_memalign failed])])
  fi

dnl the function memalign is obsolete and not used
dnl   $1_WITH_MEMALIGN="yes"
dnl   AC_SEARCH_LIBS([memalign])
dnl   if test "x$ac_cv_search_memalign" != "xnone required" ; then
dnl     $1_WITH_MEMALIGN=no
dnl   fi
dnl   if test "x$$1_WITH_MEMALIGN" = xyes ; then
dnl     AC_LINK_IFELSE([AC_LANG_PROGRAM(
dnl [[
dnl #include<stdlib.h>
dnl ]],
dnl [[
dnl int *a = (int *) memalign($$1_MEMALIGN_BYTES,3*sizeof(*a));
dnl free(a);
dnl ]])],
dnl                    [],[$1_WITH_MEMALIGN=no])
dnl   fi
dnl   if test "x$$1_WITH_MEMALIGN" = xyes ; then
dnl     AC_DEFINE([WITH_MEMALIGN],[1],[define to 1 if memalign() found])
dnl   fi
else
  AC_MSG_RESULT([not used])
fi
])
