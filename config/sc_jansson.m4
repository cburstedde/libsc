
dnl SC_CHECK_JANSSON(PREFIX)
dnl Check for the jansson library to process JSON
dnl
dnl This macro tries to compile and link various jansson functions.
dnl If a working jansson development environment is found, define
dnl PREFIX_ENABLE_JANSSON.  Set the automake conditional accordingly.
dnl If the header jansson.h exists, define PREFIX_HAVE_JANSSON_H.
dnl
AC_DEFUN([SC_CHECK_JANSSON], [
  dnl Look for header file and link library
  AC_CHECK_HEADERS([jansson.h])
  dnl Available from version 2.13 on, may be too young
  dnl AC_CHECK_LIB([jansson], [jansson_version_str])
  $1_JANSSON_SAVE_LIBS="$LIBS"
  AC_SEARCH_LIBS([json_string_set], [jansson])

  dnl Status output part 1
  AC_MSG_CHECKING([for jansson support])

  dnl Run link test for jansson program
  AS_VAR_PUSHDEF([myresult], [ac_cv_link_jansson])dnl
  AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#include <stdio.h>
#if defined HAVE_JANSSON_H
#include <jansson.h>
#else
#error "Header file for jansson not installed; disabling"
#endif
]],[[
#if 0
  /* available from version 2.13 on, may be too young */
  const char *jver = jansson_version_str ();
#endif
  json_error_t e;
  json_t * f;
  if ((f = json_loadf
           (stdin, JSON_DECODE_ANY | JSON_DISABLE_EOF_CHECK, &e))
      == NULL) {
    fprintf (stderr, "Jansson file load error: %s\n", e.text);
  }
  else {
    json_decref (f);
  }
]])], [AS_VAR_SET([myresult], [yes])], )

  dnl Act based on link test result -- status output part 2
  AS_VAR_IF([myresult], [yes],
      [AC_DEFINE([ENABLE_JANSSON], 1, [Found working jansson library])
       AC_MSG_RESULT([yes])],
      [AC_MSG_RESULT([no])
       LIBS="${$1_JANSSON_SAVE_LIBS}"])
  AM_CONDITIONAL([$1_ENABLE_JANSSON],
                 [AS_VAR_TEST_SET([myresult])])
  AS_VAR_POPDEF([myresult])
])
