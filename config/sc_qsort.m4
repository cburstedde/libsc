dnl
dnl SC_CHECK_QSORT_R(PREFIX)
dnl
dnl Checking for the prototype variant of qsort_r.
dnl There exist two conflicting versions in Linux and BSD.
dnl We use a compile-and-link test to determine which is used.
dnl #define HAVE_{GNU,BSD}_QSORT_R depending  on results.
dnl
dnl The PREFIX argument is currently unused but should be supplied.
dnl
AC_DEFUN([SC_CHECK_QSORT_R], [

   AC_MSG_CHECKING([whether qsort_r conforms to GNU standard])
   AC_LINK_IFELSE([AC_LANG_PROGRAM(
   [[
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#ifdef __cplusplus
#pragma GCC diagnostic ignored "-Wpragmas"
#endif
#pragma GCC diagnostic error "-Wimplicit-function-declaration"
#pragma GCC diagnostic error "-Wincompatible-pointer-types"
static int
comparator (const void *aa, const void *bb, void *arg)
{
  const int          *a = (int *) aa, *b = (int *) bb, *p = (int *) arg;
  int                 cmp = *a - *b;
  int                 inv_start = p[0], inv_end = p[1];
  char                norm =
    (*a < inv_start || *a > inv_end || *b < inv_start || *b > inv_end);
  return norm ? -cmp : cmp;
}
   ]],
   [[
int                 arr[4] = { 4, 3, 2, 1 };
int                 p[] = { 0, 5 };
qsort_r (arr, 4, sizeof (int), comparator, p);
   ]])],
   [AC_DEFINE([HAVE_GNU_QSORT_R], [1], [Define to 1 if qsort_r conforms to GNU standard])
    AC_MSG_RESULT([yes])],
   [AC_MSG_RESULT([no])])

   AC_MSG_CHECKING([whether qsort_r conforms to BSD standard])
   AC_LINK_IFELSE([AC_LANG_PROGRAM(
   [[
#include <stdlib.h>
#ifndef __cplusplus
#pragma GCC diagnostic error "-Wimplicit-function-declaration"
#endif
static int
comparator (void *arg, const void *aa, const void *bb)
{
  const int          *a = (int *) aa, *b = (int *) bb, *p = (int *) arg;
  int                 cmp = *a - *b;
  int                 inv_start = p[0], inv_end = p[1];
  char                norm =
    (*a < inv_start || *a > inv_end || *b < inv_start || *b > inv_end);
  return norm ? -cmp : cmp;
}
   ]],
   [[
int                 arr[4] = { 4, 3, 2, 1 };
int                 p[] = { 0, 5 };
qsort_r (arr, 4, sizeof (int), p, comparator);
   ]])],
   [AC_DEFINE([HAVE_BSD_QSORT_R], [1], [Define to 1 if qsort_r conforms to BSD standard])
    AC_MSG_RESULT([yes])],
   [AC_MSG_RESULT([no])])
])
