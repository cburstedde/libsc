dnl
dnl This file contains macros that handle common autoconf magic.
dnl

dnl
dnl Automates standard options.
dnl
AC_DEFUN([SC_PROVIDE_OPTIONS], [

AC_ARG_ENABLE([logging],
              [AC_HELP_STRING([--enable-logging=PRIO], [\
change log priority (see sc.h for possible values)])],
              [case "$enableval" in
                 yes) AC_MSG_ERROR([\
See sc.h for possible log priorities in --enable-logging=PRIO])
                 ;;
                 no) AC_DEFINE([SC_LOG_PRIORITY], [SC_LP_SILENT],
                               [minimal log priority])
                 ;;
                 *) AC_DEFINE_UNQUOTED([SC_LOG_PRIORITY], [$enableval],
                                       [minimal log priority])
               esac])
SC_ARG_ENABLE([debug], [enable debug mode (assertions and extra checks)],
              [DEBUG])
SC_ARG_ENABLE([resize-realloc], [use realloc in array resize],
              [RESIZE_REALLOC])
SC_ARG_WITHOUT([blas-code], [disable all BLAS dependent code],
               [BLAS])
SC_ARG_WITHOUT([getopt], [disable builtin getopt code, provide your own],
               [PROVIDE_GETOPT])
SC_ARG_WITHOUT([obstack], [disable builtin obstack code, provide your own],
               [PROVIDE_OBSTACK])
SC_ARG_WITHOUT([zlib], [disable builtin zlib code, provide your own],
               [PROVIDE_ZLIB])

])

dnl
dnl Checks for libraries.
dnl
AC_DEFUN([SC_PROVIDE_LIBRARIES], [

SC_REQUIRE_LIB([m], [fabs])
if test "$sc_with_PROVIDE_ZLIB" = "no" ; then
  SC_REQUIRE_LIB([z], [adler32_combine])
fi

])

dnl
dnl Checks for functions.
dnl
AC_DEFUN([SC_PROVIDE_FUNCTIONS], [

if test "$sc_with_PROVIDE_GETOPT" = "no" ; then
  SC_REQUIRE_FUNCS([getopt_long])
fi
if test "$sc_with_PROVIDE_OBSTACK" = "no" ; then
  SC_REQUIRE_FUNCS([obstack_free])
fi

])
