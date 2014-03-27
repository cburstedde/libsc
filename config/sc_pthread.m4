
dnl SC_CHECK_PTHREAD(PREFIX)
dnl Check for POSIX thread support and link a test program
dnl
AC_DEFUN([SC_CHECK_PTHREAD], [

SC_CHECK_LIB([pthread], [pthread_create], [LPTHREAD], [$1])
AC_MSG_CHECKING([for POSIX threads])

SC_ARG_ENABLE_PREFIX([pthread],
  [enable POSIX threads (optionally use --enable-pthread=<PTHREAD_CFLAGS>)],
  [PTHREAD], [$1])
if test "x$$1_ENABLE_PTHREAD" != xno ; then
  $1_PTHREAD_CFLAGS=
  if test "x$$1_ENABLE_PTHREAD" != xyes ; then
    $1_PTHREAD_CFLAGS="$$1_ENABLE_PTHREAD"
    dnl AC_MSG_ERROR([Please provide --enable-pthread without arguments])
  fi
  PRE_PTHREAD_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $$1_PTHREAD_CFLAGS"
  AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#include <pthread.h>
void *start_routine (void *v)
{
  return NULL;
}
]],[[
  pthread_t thread;
  pthread_create (&thread, NULL, &start_routine, NULL);
  pthread_join (thread, NULL);
  pthread_exit (NULL);
]])],,
                 [AC_MSG_ERROR([Unable to link with POSIX threads])])
dnl Keep the variables changed as done above
dnl CFLAGS="$PRE_PTHREAD_CFLAGS"

  AC_MSG_RESULT([successful])
else
  AC_MSG_RESULT([not used])
fi
])
