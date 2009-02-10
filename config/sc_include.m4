
dnl sc_include.m4 - general custom macros
dnl
dnl This file is part of the SC Library.
dnl The SC library provides support for parallel scientific applications.
dnl
dnl Copyright (C) 2008,2009 Carsten Burstedde, Lucas Wilcox.

dnl Documentation for macro names: brackets indicate optional arguments

dnl The shell variable PREFIX_ARG_NOT_GIVEN_DEFAULT can be set.
dnl If the argument is not given and PREFIX_ARG_NOT_GIVEN_DEFAULT is nonempty,
dnl it will override the enableval/withval variable.
dnl PREFIX_ARG_NOT_GIVEN_DEFAULT is unset at the end of each SC_ARG_* macro.
dnl
dnl Here is an internal helper function to shorten the macros below.
dnl SC_ARG_NOT_GIVEN(PREFIX, VALUE)
dnl
AC_DEFUN([SC_ARG_NOT_GIVEN],
[
if test -z "$$1_ARG_NOT_GIVEN_DEFAULT" ; then
  $1_ARG_NOT_GIVEN_DEFAULT="$2"
fi
])

dnl SC_ARG_OVERRIDE_ENABLE(PREFIX, TOKEN)
dnl This function checks for the environment variable PREFIX_ENABLE_TOKEN
dnl and if present uses it to override $enableval.
dnl Otherwise PREFIX_ENABLE_TOKEN is set to $enableval.
dnl Make sure to set enableval in action-if-not-given beforehand.
dnl This macro survives multiple invocation.
dnl
AC_DEFUN([SC_ARG_OVERRIDE_ENABLE],
[
if test -z "$$1_ENABLE_$2" -o "$$1_ENABLE_$2_OVERRIDE" = "no" ; then
  $1_ENABLE_$2="$enableval"
  $1_ENABLE_$2_OVERRIDE="no"
else
  enableval="$$1_ENABLE_$2"
  echo "Option override $1_ENABLE_$2=$enableval"
  echo "export $1_ENABLE_$2=\"$enableval\"" \
    | sed 's/\$/\\$/g' >> $1.override.pre
fi
$1_OVERRIDES="$$1_OVERRIDES \"$1_ENABLE_$2\", \"$$1_ENABLE_$2\","
])

dnl SC_ARG_OVERRIDE_WITH(PREFIX, TOKEN)
dnl This function checks for the environment variable PREFIX_WITH_TOKEN
dnl and if present uses it to override $withval.
dnl Otherwise PREFIX_WITH_TOKEN is set to $withval.
dnl Make sure to set withval in action-if-not-given beforehand.
dnl This macro survives multiple invocation.
dnl
AC_DEFUN([SC_ARG_OVERRIDE_WITH],
[
if test -z "$$1_WITH_$2" -o "$$1_WITH_$2_OVERRIDE" = "no" ; then
  $1_WITH_$2="$withval"
  $1_WITH_$2_OVERRIDE="no"
else
  withval="$$1_WITH_$2"
  echo "Option override $1_WITH_$2=$withval"
  echo "export $1_WITH_$2=\"$withval\"" \
    | sed 's/\$/\\$/g' >> $1.override.pre
fi
$1_OVERRIDES="$$1_OVERRIDES \"$1_WITH_$2\", \"$$1_WITH_$2\","
])

dnl SC_ARG_OVERRIDE_VAR(PREFIX, VARNAME)
dnl
AC_DEFUN([SC_ARG_OVERRIDE_VAR],
[
if test -n "$$1_$2" ; then
  echo "Variable override $1_$2=$$1_$2"
  echo "export $1_$2=\"$$1_$2\"" \
    | sed 's/\$/\\$/g'>> $1.override.pre
fi
$1_OVERRIDES="$$1_OVERRIDES \"$1_$2\", \"$$1_$2\","
])

dnl SC_ARG_OVERRIDE_SAVE(PREFIX)
dnl
AC_DEFUN([SC_ARG_OVERRIDE_SAVE],
[
if test -f $1.override.pre ; then
  mv -f $1.override.pre $1.override
fi
AC_DEFINE_UNQUOTED([OVERRIDES], [$$1_OVERRIDES], [$1 argument overrides])
])

dnl SC_ARG_OVERRIDE_LOAD([PREFIX])
dnl
AC_DEFUN([SC_ARG_OVERRIDE_LOAD],
[
if test -f $1.override ; then
  . $1.override
fi
$1_OVERRIDES=
])

dnl SC_ARG_ENABLE_PREFIX(NAME, COMMENT, TOKEN, PREFIX, HELPEXTRA)
dnl Check for --enable/disable-NAME using shell variable PREFIX_ENABLE_TOKEN
dnl If shell variable is set beforehand it overrides the option
dnl If enabled, define TOKEN to 1 and set conditional PREFIX_TOKEN
dnl Default is disabled
dnl
AC_DEFUN([SC_ARG_ENABLE_PREFIX],
[
SC_ARG_NOT_GIVEN([$4], [no])
AC_ARG_ENABLE([$1],
              [AS_HELP_STRING([--enable-$1$5], [$2])],,
              [enableval="$$4_ARG_NOT_GIVEN_DEFAULT"])
SC_ARG_OVERRIDE_ENABLE([$4], [$3])
if test "$enableval" != "no" ; then
  AC_DEFINE([$3], 1, [$2])
fi
AM_CONDITIONAL([$4_$3], [test "$enableval" != "no"])
$4_ARG_NOT_GIVEN_DEFAULT=
])
AC_DEFUN([SC_ARG_ENABLE],
         [SC_ARG_ENABLE_PREFIX([$1], [$2], [$3], [SC], [$4])])

dnl SC_ARG_DISABLE_PREFIX(NAME, COMMENT, TOKEN, PREFIX, HELPEXTRA)
dnl Check for --enable/disable-NAME using shell variable PREFIX_ENABLE_TOKEN
dnl If shell variable is set beforehand it overrides the option
dnl If enabled, define TOKEN to 1 and set conditional PREFIX_TOKEN
dnl Default is enabled
dnl
AC_DEFUN([SC_ARG_DISABLE_PREFIX],
[
SC_ARG_NOT_GIVEN([$4], [yes])
AC_ARG_ENABLE([$1],
              [AS_HELP_STRING([--disable-$1$5], [$2])],,
              [enableval="$$4_ARG_NOT_GIVEN_DEFAULT"])
SC_ARG_OVERRIDE_ENABLE([$4], [$3])
if test "$enableval" != "no" ; then
  AC_DEFINE([$3], 1, [Undefine if: $2])
fi
AM_CONDITIONAL([$4_$3], [test "$enableval" != "no"])
$4_ARG_NOT_GIVEN_DEFAULT=
])
AC_DEFUN([SC_ARG_DISABLE],
         [SC_ARG_DISABLE_PREFIX([$1], [$2], [$3], [SC], [$4])])

dnl SC_ARG_WITH_PREFIX(NAME, COMMENT, TOKEN, PREFIX, HELPEXTRA)
dnl Check for --with/without-NAME using shell variable PREFIX_WITH_TOKEN
dnl If shell variable is set beforehand it overrides the option
dnl If with, define TOKEN to 1 and set conditional PREFIX_TOKEN
dnl Default is without
dnl
AC_DEFUN([SC_ARG_WITH_PREFIX],
[
SC_ARG_NOT_GIVEN([$4], [no])
AC_ARG_WITH([$1],
            [AS_HELP_STRING([--with-$1$5], [$2])],,
            [withval="$$4_ARG_NOT_GIVEN_DEFAULT"])
SC_ARG_OVERRIDE_WITH([$4], [$3])
if test "$withval" != "no" ; then
  AC_DEFINE([$3], 1, [$2])
fi
AM_CONDITIONAL([$4_$3], [test "$withval" != "no"])
$4_ARG_NOT_GIVEN_DEFAULT=
])
AC_DEFUN([SC_ARG_WITH],
         [SC_ARG_WITH_PREFIX([$1], [$2], [$3], [SC], [$4])])

dnl SC_ARG_WITHOUT_PREFIX(NAME, COMMENT, TOKEN, PREFIX, HELPEXTRA)
dnl Check for --with/without-NAME using shell variable PREFIX_WITH_TOKEN
dnl If shell variable is set beforehand it overrides the option
dnl If with, define TOKEN to 1 and set conditional PREFIX_TOKEN
dnl Default is with
dnl
AC_DEFUN([SC_ARG_WITHOUT_PREFIX],
[
SC_ARG_NOT_GIVEN([$4], [yes])
AC_ARG_WITH([$1],
            [AS_HELP_STRING([--without-$1$5], [$2])],,
            [withval="$$4_ARG_NOT_GIVEN_DEFAULT"])
SC_ARG_OVERRIDE_WITH([$4], [$3])
if test "$withval" != "no" ; then
  AC_DEFINE([$3], 1, [Undefine if: $2])
fi
AM_CONDITIONAL([$4_$3], [test "$withval" != "no"])
$4_ARG_NOT_GIVEN_DEFAULT=
])
AC_DEFUN([SC_ARG_WITHOUT],
         [SC_ARG_WITHOUT_PREFIX([$1], [$2], [$3], [SC], [$4])])

dnl SC_ARG_WITH_YES_PREFIX(NAME, COMMENT, TOKEN, PREFIX, HELPEXTRA)
dnl Check for --with/without-NAME using shell variable PREFIX_WITH_TOKEN
dnl If shell variable is set beforehand it overrides the option
dnl If with = yes, define TOKEN to 1 and set conditional PREFIX_TOKEN
dnl Default is no
dnl
AC_DEFUN([SC_ARG_WITH_YES_PREFIX],
[
SC_ARG_NOT_GIVEN([$4], [no])
AC_ARG_WITH([$1],
            [AS_HELP_STRING([--with-$1$5], [$2])],,
            [withval="$$4_ARG_NOT_GIVEN_DEFAULT"])
SC_ARG_OVERRIDE_WITH([$4], [$3])
if test "$withval" = "yes" ; then
  AC_DEFINE([$3], 1, [$2])
fi
AM_CONDITIONAL([$4_$3], [test "$withval" = "yes"])
$4_ARG_NOT_GIVEN_DEFAULT=
])
AC_DEFUN([SC_ARG_WITH_YES],
         [SC_ARG_WITH_YES_PREFIX([$1], [$2], [$3], [SC], [$4])])

dnl SC_ARG_WITHOUT_YES_PREFIX(NAME, COMMENT, TOKEN, PREFIX, HELPEXTRA)
dnl Check for --with/without-NAME using shell variable PREFIX_WITH_TOKEN
dnl If shell variable is set beforehand it overrides the option
dnl If with = yes, define TOKEN to 1 and set conditional PREFIX_TOKEN
dnl Default is yes
dnl
AC_DEFUN([SC_ARG_WITHOUT_YES_PREFIX],
[
SC_ARG_NOT_GIVEN([$4], [yes])
AC_ARG_WITH([$1],
            [AS_HELP_STRING([--without-$1$5], [$2])],,
            [withval="$$4_ARG_NOT_GIVEN_DEFAULT"])
SC_ARG_OVERRIDE_WITH([$4], [$3])
if test "$withval" = "yes" ; then
  AC_DEFINE([$3], 1, [Undefine if: $2])
fi
AM_CONDITIONAL([$4_$3], [test "$withval" = "yes"])
$4_ARG_NOT_GIVEN_DEFAULT=
])
AC_DEFUN([SC_ARG_WITHOUT_YES],
         [SC_ARG_WITHOUT_YES_PREFIX([$1], [$2], [$3], [SC], [$4])])

dnl SC_REQUIRE_LIB(LIBRARY LIST, FUNCTION)
dnl Check for FUNCTION in LIBRARY, exit with error if not found
dnl
AC_DEFUN([SC_REQUIRE_LIB],
    [AC_SEARCH_LIBS([$2], [$1],,
      [AC_MSG_ERROR([Could not find function $2 in $1])])])

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

dnl SC_DETERMINE_INSTALL(PREFIX)
dnl This function throws an error if the variable PREFIX_DIR does not exist.
dnl Looks for PREFIX_DIR/{include,lib,bin} to determine installation status.
dnl Set the shell variable PREFIX_INSTALL to "yes" or "no".
dnl
AC_DEFUN([SC_DETERMINE_INSTALL],
[
if test ! -d "$$1_DIR" ; then
  AC_MSG_ERROR([Directory "$$1_DIR" does not exist])
fi
if test -d "$$1_DIR/include" -o -d "$$1_DIR/lib" -o -d "$$1_DIR/bin" ; then
  $1_INSTALL="yes"
else
  $1_INSTALL="no"
fi
])

dnl SC_DETERMINE_INCLUDE_PATH(PREFIX, CPPFLAGS)
dnl This function expects the variable PREFIX_DIR to exist.
dnl Looks for PREFIX_DIR/include and then PREFIX_DIR/src.
dnl If neither is found, throws an error.
dnl Otherwise, set the shell variable PREFIX_CPPFLAGS to -I<dir> CPPFLAGS.
dnl
AC_DEFUN([SC_DETERMINE_INCLUDE_PATH],
[
$1_INC="$$1_DIR/include"
if test ! -d "$$1_INC" ; then
  $1_INC="$$1_DIR/src"
fi
if test ! -d "$$1_INC" ; then
  AC_MSG_ERROR([Include directories based on $$1_DIR not found])
fi
$1_CPPFLAGS="-I$$1_INC $2"
])

dnl SC_DETERMINE_LIBRARY_PATH(PREFIX, LIBS)
dnl This function expects the variable PREFIX_DIR to exist.
dnl Looks for PREFIX_DIR/lib and then PREFIX_DIR/src.
dnl If neither is found, throws an error.
dnl Otherwise, set the shell variable PREFIX_LDADD to -L<dir> LIBS.
dnl
AC_DEFUN([SC_DETERMINE_LIBRARY_PATH],
[
$1_LIB="$$1_DIR/lib"
if test ! -d "$$1_LIB" ; then
  $1_LIB="$$1_DIR/src"
fi
if test ! -d "$$1_LIB" ; then
  AC_MSG_ERROR([Library directories based on $$1_DIR not found])
fi
$1_LDADD="-L$$1_LIB $2"
])

dnl SC_DETERMINE_CONFIG_PATH(PREFIX)
dnl This function expects the variable PREFIX_DIR to exist.
dnl Looks for PREFIX_DIR/etc and then PREFIX_DIR/src.
dnl If neither is found, throws an error.
dnl
AC_DEFUN([SC_DETERMINE_CONFIG_PATH],
[
$1_CONFIG="$$1_DIR/share/aclocal"
if test ! -d "$$1_CONFIG" ; then
  $1_CONFIG="$$1_DIR/config"
fi
if test ! -d "$$1_CONFIG" ; then
  AC_MSG_ERROR([Config directories based on $$1_DIR not found])
fi
])
