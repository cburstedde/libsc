
dnl SC_CHECK_MEMALIGN(PREFIX)
dnl Let the user either specify --with-memalign (configure tries to determine
dnl the alignment size) or --with-memalign=X (number of bytes).  Searches for
dnl memalign functions (allignedposix_memalign, memalign).
dnl
AC_DEFUN([SC_CHECK_MEMALIGN], [

AC_MSG_CHECKING([memory alignment])

SC_ARG_WITH_PREFIX([memalign],
  [used aligned malloc (optionally use --with-memalign=<NUM_BYTES>)],
  [MEMALIGN], [$1])
if test "x$$1_ENABLE_MEMALIGN" != xno ; then
  $1_MEMALIGN_BYTES=
  if test "x$$1_ENABLE_MEMALIGN" != xyes ; then
    $1_MEMALIGN_BYTES="$$1_ENABLE_MEMALIGN"
  fi
  if test "x$$1_MEMALIGN_BYTES" = x ; then
    AC_MSG_NOTICE([Attempting to determine the best memory alignment])
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

