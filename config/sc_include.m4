dnl
dnl SC acinclude.m4 - custom macros
dnl

dnl Documentation for macro names: brackets indicate optional arguments

dnl SC_ARG_ENABLE(NAME, COMMENT, TOKEN)
dnl Check for --enable/disable-NAME using shell variable sc_enable_TOKEN
dnl If enabled, define TOKEN to 1 and set conditional SC_TOKEN
dnl Default is disabled
dnl
AC_DEFUN([SC_ARG_ENABLE],
[
AC_ARG_ENABLE([$1], [AC_HELP_STRING([--enable-$1], [$2])],
              [sc_enable_$3="$enableval"], [sc_enable_$3="no"])
if test "$sc_enable_$3" != "no" ; then
  AC_DEFINE([$3], 1, [$2])
fi
AM_CONDITIONAL([SC_$3], [test "$sc_enable_$3" != "no"])
])

dnl SC_ARG_DISABLE(NAME, COMMENT, TOKEN)
dnl Check for --enable/disable-NAME using shell variable sc_enable_TOKEN
dnl If enabled, define TOKEN to 1 and set conditional SC_TOKEN
dnl Default is enabled
dnl
AC_DEFUN([SC_ARG_DISABLE],
[
AC_ARG_ENABLE([$1], [AC_HELP_STRING([--disable-$1], [$2])],
              [sc_enable_$3="$enableval"], [sc_enable_$3="yes"])
if test "$sc_enable_$3" != "no" ; then
  AC_DEFINE([$3], 1, [Undefine if: $2])
fi
AM_CONDITIONAL([SC_$3], [test "$sc_enable_$3" != "no"])
])

dnl SC_ARG_WITH(NAME, COMMENT, TOKEN)
dnl Check for --with/without-NAME using shell variable sc_with_TOKEN
dnl If with, define TOKEN to 1 and set conditional SC_TOKEN
dnl Default is without
dnl
AC_DEFUN([SC_ARG_WITH],
[
AC_ARG_WITH([$1], [AC_HELP_STRING([--with-$1], [$2])],
            [sc_with_$3="$withval"], [sc_with_$3="no"])
if test "$sc_with_$3" != "no" ; then
  AC_DEFINE([$3], 1, [$2])
fi
AM_CONDITIONAL([SC_$3], [test "$sc_with_$3" != "no"])
])

dnl SC_ARG_WITHOUT(NAME, COMMENT, TOKEN)
dnl Check for --with/without-NAME using shell variable sc_with_TOKEN
dnl If with, define TOKEN to 1 and set conditional SC_TOKEN
dnl Default is with
dnl
AC_DEFUN([SC_ARG_WITHOUT],
[
AC_ARG_WITH([$1], [AC_HELP_STRING([--without-$1], [$2])],
            [sc_with_$3="$withval"], [sc_with_$3="yes"])
if test "$sc_with_$3" != "no" ; then
  AC_DEFINE([$3], 1, [Undefine if: $2])
fi
AM_CONDITIONAL([SC_$3], [test "$sc_with_$3" != "no"])
])

dnl SC_REQUIRE_LIB(LIBRARY, FUNCTION)
dnl Check for FUNCTION in LIBRARY, exit with error if not found
dnl
AC_DEFUN([SC_REQUIRE_LIB],
    [AC_CHECK_LIB([$1], [$2], ,
      [AC_MSG_ERROR([Could not find function $2 in library $1])])])

dnl SC_REQUIRE_LIB_SEARCH(LIBRARY LIST, FUNCTION)
dnl Check for FUNCTION in any of LIBRARY LIST, exit with error if not found
dnl
AC_DEFUN([SC_REQUIRE_LIB_SEARCH],
    [AC_SEARCH_LIBS([$2], [$1], ,
      [AC_MSG_ERROR([Could not find function $2 in any of $1])])])

dnl SC_REQUIRE_FUNCS(FUNCTION LIST)
dnl Check for all functions in FUNCTION LIST, exit with error if not found
dnl
AC_DEFUN([SC_REQUIRE_FUNCS],
    [
     AC_FOREACH([sc_thefunc], [$1],
      [AC_CHECK_FUNC([sc_thefunc], ,
        [AC_MSG_ERROR([Could not find function sc_thefunc])])])])

dnl EOF acinclude.m4
