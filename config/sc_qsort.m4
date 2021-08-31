dnl
dnl Test script for checking qsort_r
dnl 
AC_DEFUN([SC_QSORT_TEST], [
   AC_MSG_CHECKING([for GNU variant of qsort_r])
   AC_LINK_IFELSE([AC_LANG_PROGRAM(
   [[
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include<stdlib.h>

#ifdef __cplusplus
#pragma GCC diagnostic ignored "-Wpragmas"
#endif
#pragma GCC diagnostic error "-Wimplicit-function-declaration"
#pragma GCC diagnostic error "-Wincompatible-pointer-types"

int
comparator (const void *aa, const void *bb, void *arg)
{
  const int          *a = (int *) aa, *b = (int *) bb, *p = (int *) arg;
  int                 cmp = *a - *b;
  int                 inv_start = p[0], inv_end = p[1];
  char                norm = (*a < inv_start || *a > inv_end || *b < inv_start
                              || *b > inv_end);
  return norm ? -cmp : cmp;
}
   ]],
   [[
int                 arr[4] = { 4, 3, 2, 1 };
int                 p[] = { 0, 5 };

qsort_r (arr, 4, sizeof (int), comparator, p);
   ]])],
   [AC_DEFINE([HAVE_GNU_QSORT_R],[1],[Define to 1 if qsort_r conforms to GNU standard])
    AC_MSG_RESULT([qsort_r conforms to GNU standard])],
   [AC_DEFINE([HAVE_GNU_QSORT_R],[0],[Define to 1 if qsort_r conforms to GNU standard])
    AC_MSG_RESULT([qsort_r does not conform to GNU standard])])

   AC_MSG_CHECKING([for BSD variant of qsort_r])
   AC_LINK_IFELSE([AC_LANG_PROGRAM(
   [[
#include<stdlib.h>

#ifndef __cplusplus
#pragma GCC diagnostic error "-Wimplicit-function-declaration"
#endif

int
comparator (void *arg, const void *aa, const void *bb)
{
  const int          *a = (int *) aa, *b = (int *) bb, *p = (int *) arg;
  int                 cmp = *a - *b;
  int                 inv_start = p[0], inv_end = p[1];
  char                norm = (*a < inv_start || *a > inv_end || *b < inv_start
                              || *b > inv_end);
  return norm ? -cmp : cmp;
}
   ]],
   [[
int                 arr[4] = { 4, 3, 2, 1 };
int                 p[] = { 0, 5 };

qsort_r (arr, 4, sizeof (int), p, comparator);
   ]])],
   [AC_DEFINE([HAVE_BSD_QSORT_R],[1],[Define to 1 if qsort_r conforms to BSD standard])
    AC_MSG_RESULT([qsort_r conforms to BSD standard])],
   [AC_DEFINE([HAVE_BSD_QSORT_R],[0],[Define to 1 if qsort_r conforms to BSD standard])
      AC_MSG_RESULT([qsort_r does not conform to BSD standard])])
 ])