
dnl sc_package.m4 - general custom macros
dnl
dnl This file is part of the SC Library.
dnl The SC library provides support for parallel scientific applications.
dnl
dnl Copyright (C) 2008,2009 Carsten Burstedde, Lucas Wilcox.

dnl Documentation for macro names: brackets indicate optional arguments

dnl SC_PACKAGE_SPECIFY(PREFIX, REQUIRE_INCLUDE, REQUIRE_LDADD,
dnl                    REQUIRE_CONFIG, REQUIRE_ETC)
dnl The REQUIRE_* arguments can be either "true" or "false" (without quotes).
dnl This function throws an error if the variable PREFIX_DIR does not exist.
dnl Looks for PREFIX_DIR/src to identify a source distribution.
dnl If not found, package must have been `make install`ed, in this case
dnl optionally require include, lib, config and etc directories.
dnl Set the shell variable PREFIX_INSTALL to "yes" or "no".
dnl
AC_DEFUN([SC_CHECK_PACKAGE],
[
if test ! -d "$$1_DIR" ; then
  AC_MSG_ERROR([Directory "$$1_DIR" does not exist])
fi
if test -d "$$1_DIR/src" ; then
  $1_INSTALL="no"
  $1_INC="$$1_DIR/src"
  $1_LIB="$$1_DIR/src"
  $1_CFG="$$1_DIR/config"
  $1_ETC=
  if $4 && test ! -d "$$1_CFG" ; then
    AC_MSG_ERROR([Specified source path $$1_CFG not found])
  fi
else
  $1_INSTALL="yes"
  $1_INC="$$1_DIR/include"
  $1_LIB="$$1_DIR/lib"
  $1_CFG="$$1_DIR/share/aclocal"
  $1_ETC="$$1_DIR/etc"
  if $2 && test ! -d "$$1_INC" ; then
    AC_MSG_ERROR([Specified installation path $$1_INC not found])
  fi
  if $3 && test ! -d "$$1_LIB" ; then
    AC_MSG_ERROR([Specified installation path $$1_LIB not found])
  fi
  if $4 && test ! -d "$$1_CFG" ; then
    AC_MSG_ERROR([Specified installation path $$1_CFG not found])
  fi
  if $5 && test ! -d "$$1_ETC" ; then
    AC_MSG_ERROR([Specified installation path $$1_ETC not found])
  fi
fi
])
