dnl
dnl SC acinclude.m4 - custom macros
dnl

dnl Documentation for macro names: brackets indicate optional arguments

dnl SC_ARG_ENABLE_PREFIX(NAME, COMMENT, TOKEN, PREFIX)
dnl Check for --enable/disable-NAME using shell variable PREFIX_ENABLE_TOKEN
dnl If enabled, define TOKEN to 1 and set conditional PREFIX_TOKEN
dnl Default is disabled
dnl
AC_DEFUN([SC_ARG_ENABLE_PREFIX],
[
AC_ARG_ENABLE([$1], [AS_HELP_STRING([--enable-$1], [$2])],
              [$4_ENABLE_$3="$enableval"], [$4_ENABLE_$3="no"])
if test "$$4_ENABLE_$3" != "no" ; then
  AC_DEFINE([$3], 1, [$2])
fi
AM_CONDITIONAL([$4_$3], [test "$$4_ENABLE_$3" != "no"])
])
AC_DEFUN([SC_ARG_ENABLE], [SC_ARG_ENABLE_PREFIX([$1], [$2], [$3], [SC])])

dnl SC_ARG_DISABLE_PREFIX(NAME, COMMENT, TOKEN, PREFIX)
dnl Check for --enable/disable-NAME using shell variable PREFIX_ENABLE_TOKEN
dnl If enabled, define TOKEN to 1 and set conditional PREFIX_TOKEN
dnl Default is enabled
dnl
AC_DEFUN([SC_ARG_DISABLE_PREFIX],
[
AC_ARG_ENABLE([$1], [AS_HELP_STRING([--disable-$1], [$2])],
              [$4_ENABLE_$3="$enableval"], [$4_ENABLE_$3="yes"])
if test "$$4_ENABLE_$3" != "no" ; then
  AC_DEFINE([$3], 1, [Undefine if: $2])
fi
AM_CONDITIONAL([$4_$3], [test "$$4_ENABLE_$3" != "no"])
])
AC_DEFUN([SC_ARG_DISABLE], [SC_ARG_DISABLE_PREFIX([$1], [$2], [$3], [SC])])

dnl SC_ARG_WITH_PREFIX(NAME, COMMENT, TOKEN, PREFIX)
dnl Check for --with/without-NAME using shell variable PREFIX_WITH_TOKEN
dnl If with, define TOKEN to 1 and set conditional PREFIX_TOKEN
dnl Default is without
dnl
AC_DEFUN([SC_ARG_WITH_PREFIX],
[
AC_ARG_WITH([$1], [AS_HELP_STRING([--with-$1], [$2])],
            [$4_WITH_$3="$withval"], [$4_WITH_$3="no"])
if test "$$4_WITH_$3" != "no" ; then
  AC_DEFINE([$3], 1, [$2])
fi
AM_CONDITIONAL([$4_$3], [test "$$4_WITH_$3" != "no"])
])
AC_DEFUN([SC_ARG_WITH], [SC_ARG_WITH_PREFIX([$1], [$2], [$3], [SC])])

dnl SC_ARG_WITHOUT_PREFIX(NAME, COMMENT, TOKEN, PREFIX)
dnl Check for --with/without-NAME using shell variable PREFIX_WITH_TOKEN
dnl If with, define TOKEN to 1 and set conditional PREFIX_TOKEN
dnl Default is with
dnl
AC_DEFUN([SC_ARG_WITHOUT_PREFIX],
[
AC_ARG_WITH([$1], [AS_HELP_STRING([--without-$1], [$2])],
            [$4_WITH_$3="$withval"], [$4_WITH_$3="yes"])
if test "$$4_WITH_$3" != "no" ; then
  AC_DEFINE([$3], 1, [Undefine if: $2])
fi
AM_CONDITIONAL([$4_$3], [test "$$4_WITH_$3" != "no"])
])
AC_DEFUN([SC_ARG_WITHOUT], [SC_ARG_WITHOUT_PREFIX([$1], [$2], [$3], [SC])])

dnl SC_ARG_WITH_YES_PREFIX(NAME, COMMENT, TOKEN, PREFIX)
dnl Check for --with/without-NAME using shell variable PREFIX_WITH_TOKEN
dnl If with = yes, define TOKEN to 1 and set conditional PREFIX_TOKEN
dnl Default is no
dnl
AC_DEFUN([SC_ARG_WITH_YES_PREFIX],
[
AC_ARG_WITH([$1], [AS_HELP_STRING([--with-$1], [$2])],
            [$4_WITH_$3="$withval"], [$4_WITH_$3="no"])
if test "$$4_WITH_$3" = "yes" ; then
  AC_DEFINE([$3], 1, [$2])
fi
AM_CONDITIONAL([$4_$3], [test "$$4_WITH_$3" = "yes"])
])
AC_DEFUN([SC_ARG_WITH_YES],
         [SC_ARG_WITH_YES_PREFIX([$1], [$2], [$3], [SC])])

dnl SC_ARG_WITHOUT_YES_PREFIX(NAME, COMMENT, TOKEN, PREFIX)
dnl Check for --with/without-NAME using shell variable PREFIX_WITH_TOKEN
dnl If with = yes, define TOKEN to 1 and set conditional PREFIX_TOKEN
dnl Default is yes
dnl
AC_DEFUN([SC_ARG_WITHOUT_YES_PREFIX],
[
AC_ARG_WITH([$1], [AS_HELP_STRING([--without-$1], [$2])],
            [$4_WITH_$3="$withval"], [$4_WITH_$3="yes"])
if test "$$4_WITH_$3" = "yes" ; then
  AC_DEFINE([$3], 1, [Undefine if: $2])
fi
AM_CONDITIONAL([$4_$3], [test "$$4_WITH_$3" = "yes"])
])
AC_DEFUN([SC_ARG_WITHOUT_YES],
         [SC_ARG_WITHOUT_YES_PREFIX([$1], [$2], [$3], [SC])])

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
m4_foreach_w([sc_thefunc], [$1],
             [AC_CHECK_FUNC([sc_thefunc], ,
                            [AC_MSG_ERROR([\
Could not find function sc_thefunc])])])
])

dnl EOF acinclude.m4
