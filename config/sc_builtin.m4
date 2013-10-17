
dnl sc_builtin.m4 - custom macros for distributing third-party software
dnl
dnl This file is part of the SC Library.
dnl The SC library provides support for parallel scientific applications.
dnl
dnl Copyright (C) 2008,2009 Carsten Burstedde, Lucas Wilcox.

dnl Documentation for macro names: brackets indicate optional arguments

dnl SC_ARG_WITH_BUILTIN_PREFIX(NAME, TOKEN, PREFIX)
dnl Check for --without-NAME using shell variable PREFIX_WITH_TOKEN
dnl Only allowed values are yes or no, default is yes
dnl
AC_DEFUN([SC_ARG_WITH_BUILTIN_PREFIX],
[
AC_ARG_WITH([$1],
            [AS_HELP_STRING([--without-$1], [assume external $1 code is found])
AS_HELP_STRING(, [(default: check and use builtin if necessary)])],,
            [withval=yes])
if test "$withval" != "yes" -a "$withval" != "no" ; then
  AC_MSG_ERROR([Please use --without-$1 without an argument])
fi
])
AC_DEFUN([SC_ARG_WITH_BUILTIN],
         [SC_ARG_WITH_BUILTIN_PREFIX([$1], [$2], [SC])])

dnl SC_ARG_WITH_BUILTIN_ALL_PREFIX(PREFIX)
dnl Aggregate all libsc builtin option queries for convenience.
dnl
AC_DEFUN([SC_ARG_WITH_BUILTIN_ALL_PREFIX],
[
SC_ARG_WITH_BUILTIN_PREFIX([getopt], [GETOPT], [$1])
SC_ARG_WITH_BUILTIN_PREFIX([obstack], [OBSTACK], [$1])
SC_ARG_WITH_BUILTIN_PREFIX([zlib], [ZLIB], [$1])
SC_ARG_WITH_BUILTIN_PREFIX([lua], [LUA], [$1])
])
AC_DEFUN([SC_ARG_WITH_BUILTIN_ALL], [SC_ARG_WITH_BUILTIN_ALL_PREFIX([SC])])

dnl SC_BUILTIN_GETOPT_PREFIX(PREFIX)
dnl This function only activates if PREFIX_WITH_GETOPT is "yes".
dnl This function checks if getopt_long can be compiled.
dnl The shell variable PREFIX_PROVIDE_GETOPT is set to "yes" or "no".
dnl Both a define and automake conditional are set.
dnl
AC_DEFUN([SC_BUILTIN_GETOPT_PREFIX],
[
$1_PROVIDE_GETOPT="no"
if test "$$1_WITH_GETOPT" = "yes" ; then
  AC_MSG_CHECKING([for getopt])
  AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[#include <getopt.h>]], [[
int oi;
struct option lo;
getopt_long (0, 0, "abc:", &lo, &oi);
]])], [AC_MSG_RESULT([successful])], [
    AC_MSG_RESULT([failed])
    AC_MSG_NOTICE([did not find getopt. Activating builtin])
    $1_PROVIDE_GETOPT="yes"
    AC_DEFINE([PROVIDE_GETOPT], 1, [Use builtin getopt])
  ])
fi
AM_CONDITIONAL([$1_PROVIDE_GETOPT], [test "$$1_PROVIDE_GETOPT" = "yes"])
])
AC_DEFUN([SC_BUILTIN_GETOPT], [SC_BUILTIN_GETOPT_PREFIX([SC])])

dnl SC_BUILTIN_OBSTACK_PREFIX(PREFIX)
dnl This function only activates if PREFIX_WITH_OBSTACK is "yes".
dnl This function checks if a simple obstack program can be compiled.
dnl The shell variable PREFIX_PROVIDE_OBSTACK is set to "yes" or "no".
dnl Both a define and automake conditional are set.
dnl
AC_DEFUN([SC_BUILTIN_OBSTACK_PREFIX],
[
$1_PROVIDE_OBSTACK="no"
if test "$$1_WITH_OBSTACK" = "yes" ; then
  AC_MSG_CHECKING([for obstack])
  AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[#include <obstack.h>]], [[
struct obstack ob;
static void        *(*obstack_chunk_alloc) (size_t) = 0;
static void         (*obstack_chunk_free) (void *) = 0;
obstack_init (&ob);
obstack_free (&ob, 0);
]])], [AC_MSG_RESULT([successful])], [
    AC_MSG_RESULT([failed])
    AC_MSG_NOTICE([did not find obstack. Activating builtin])
    $1_PROVIDE_OBSTACK="yes"
    AC_DEFINE([PROVIDE_OBSTACK], 1, [Use builtin obstack])
  ])
fi
AM_CONDITIONAL([$1_PROVIDE_OBSTACK], [test "$$1_PROVIDE_OBSTACK" = "yes"])
])
AC_DEFUN([SC_BUILTIN_OBSTACK], [SC_BUILTIN_OBSTACK_PREFIX([SC])])

dnl SC_BUILTIN_ZLIB_PREFIX(PREFIX)
dnl This function only activates if PREFIX_WITH_ZLIB is "yes".
dnl This function checks if adler32_combine can be linked against.
dnl The shell variable PREFIX_PROVIDE_ZLIB is set to "yes" or "no".
dnl Both a define and automake conditional are set.
dnl
AC_DEFUN([SC_BUILTIN_ZLIB_PREFIX],
[
$1_PROVIDE_ZLIB="no"
if test "$$1_WITH_ZLIB" = "yes" ; then
  AC_MSG_NOTICE([Using builtin zlib 1.2.4 until that version is commonplace])
  $1_PROVIDE_ZLIB="yes"
  AC_DEFINE([PROVIDE_ZLIB], 1, [Use builtin zlib])
  dnl AC_SEARCH_LIBS([adler32_combine], [z],, [
  dnl   AC_MSG_NOTICE([did not find a recent zlib. Activating builtin])
  dnl   $1_PROVIDE_ZLIB="yes"
  dnl   AC_DEFINE([PROVIDE_ZLIB], 1, [Use builtin zlib])
  dnl ])
fi
AM_CONDITIONAL([$1_PROVIDE_ZLIB], [test "$$1_PROVIDE_ZLIB" = "yes"])
])
AC_DEFUN([SC_BUILTIN_ZLIB], [SC_BUILTIN_ZLIB_PREFIX([SC])])

dnl SC_BUILTIN_LUA_PREFIX(PREFIX)
dnl This function only activates if PREFIX_WITH_LUA is "yes".
dnl This function checks if lua_createtable can be linked against.
dnl The shell variable PREFIX_PROVIDE_LUA is set to "yes" or "no".
dnl Both a define and automake conditional are set.
dnl Must not be called conditionally since it uses AM_CONDITIONAL.
dnl
AC_DEFUN([SC_BUILTIN_LUA_PREFIX],
[
$1_PROVIDE_LUA="no"
if test "$$1_WITH_LUA" = "yes" ; then
  AC_CHECK_HEADERS([lua.h lua5.1/lua.h], [break])
  AC_SEARCH_LIBS([lua_createtable], [lua lua5 lua51 lua5.1],, [
    AC_MSG_NOTICE([did not find a recent lua. Activating builtin])
    $1_PROVIDE_LUA="yes"
    AC_DEFINE([PROVIDE_LUA], 1, [Use builtin lua])
  ])
fi
AM_CONDITIONAL([$1_PROVIDE_LUA], [test "$$1_PROVIDE_LUA" = "yes"])
])
AC_DEFUN([SC_BUILTIN_LUA], [SC_BUILTIN_LUA_PREFIX([SC])])

dnl SC_BUILTIN_ALL_PREFIX(PREFIX, CONDITION)
dnl Aggregate all checks from this file for convenience.
dnl If CONDITION is false, the PREFIX_WITH_* variables are set to "no".
dnl Must not be called conditionally since it uses AM_CONDITIONAL.
dnl
AC_DEFUN([SC_BUILTIN_ALL_PREFIX],
[
if !($2) ; then
  $1_WITH_GETOPT=no
  $1_WITH_OBSTACK=no
  $1_WITH_ZLIB=no
  $1_WITH_LUA=no
fi
SC_BUILTIN_GETOPT_PREFIX([$1])
SC_BUILTIN_OBSTACK_PREFIX([$1])
SC_BUILTIN_ZLIB_PREFIX([$1])
SC_BUILTIN_LUA_PREFIX([$1])
])
AC_DEFUN([SC_BUILTIN_ALL], [SC_BUILTIN_ALL_PREFIX([SC], [$1])])
