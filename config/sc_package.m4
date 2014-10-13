
dnl sc_package.m4 - general custom macros
dnl
dnl This file is part of the SC Library.
dnl The SC library provides support for parallel scientific applications.
dnl
dnl Copyright (C) 2008,2009,2014 Carsten Burstedde, Lucas C. Wilcox.

dnl See SC_AS_SUBPACKAGE below for documentation of the mechanism

dnl Documentation for macro names: brackets indicate optional arguments

dnl SC_CHECK_INSTALL(PREFIX, REQUIRE_INCLUDE, REQUIRE_LDADD,
dnl                  REQUIRE_CONFIG, REQUIRE_ETC)
dnl The REQUIRE_* arguments can be either "true" or "false" (without quotes).
dnl This function throws an error if the variable PREFIX_DIR does not exist.
dnl The package must have been make install'd in that directory.
dnl Optionally require include, lib, config, and etc subdirectories.
dnl Set the shell variable PREFIX_INSTALL to "yes."
dnl
AC_DEFUN([SC_CHECK_INSTALL],
[
if test "x$$1_DIR" = xyes ; then
  AC_MSG_ERROR([Please provide an argument as in --with-PACKAGE=<directory>])
fi
if test ! -d "$$1_DIR" ; then
  AC_MSG_ERROR([Directory "$$1_DIR" does not exist])
fi
$1_INSTALL=yes
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
])

dnl SC_CHECK_PACKAGE(PREFIX, REQUIRE_INCLUDE, REQUIRE_LDADD,
dnl                  REQUIRE_CONFIG, REQUIRE_ETC)
dnl The REQUIRE_* arguments can be either "true" or "false" (without quotes).
dnl This function throws an error if the variable PREFIX_DIR does not exist.
dnl Looks for PREFIX_DIR/src to identify a source distribution.
dnl If not found, package must have been `make install`ed, in this case
dnl optionally require include, lib, config, and etc directories.
dnl Set the shell variable PREFIX_INSTALL to "yes" or "no."
dnl
AC_DEFUN([SC_CHECK_PACKAGE],
[
if test ! -d "$$1_DIR" ; then
  AC_MSG_ERROR([Directory "$$1_DIR" does not exist])
fi
if test -d "$$1_DIR/src" ; then
  $1_INSTALL=no
  $1_INC="$$1_DIR/src"
  $1_LIB="$$1_DIR/src"
  $1_CFG="$$1_DIR/config"
  $1_ETC=
  if $4 && test ! -d "$$1_CFG" ; then
    AC_MSG_ERROR([Specified source path $$1_CFG not found])
  fi
else
  SC_CHECK_INSTALL([$1], [$2], [$3], [$4], [$5])
fi
])

dnl---------------------- HOW SUBPACKAGES WORK ---------------------------
dnl
dnl A program PROG relies on libsc (or another package called ME, which
dnl could itself be using libsc).  In a build from source, me usually
dnl resides in PROG's subdirectory me.  This location can be overridden by
dnl the environment variable PROG_ME_SOURCE; this situation can arise when
dnl both PROG and me are subpackages to yet another software.  The
dnl path in PROG_ME_SOURCE must be relative to PROG's toplevel directory.
dnl All of this works without specifying a configure command line option.
dnl However, if me is already make install'd in the system and should
dnl be used from there, use --with-me=<path to me's install directory>.
dnl In this case, PROG expects subdirectories etc, include, lib, and
dnl share/aclocal, which are routinely created by me's make install.
dnl
dnl SC_ME_AS_SUBPACKAGE(PREFIX, prefix, ME, me)
dnl Call from a package that is using this package ME as a subpackage.
dnl Sets PREFIX_DIST_DENY=yes if me is make install'd.
dnl
AC_DEFUN([SC_ME_AS_SUBPACKAGE],
[
$1_$3_SUBDIR=
$1_$3_MK_USE=
$1_DISTCLEAN="$$1_DISTCLEAN $1_$3_SOURCE.log"

SC_ARG_WITH_PREFIX([$4], [path to installed package $4 (optional)], [$3], [$1])

if test "x$$1_WITH_$3" != xno ; then
  AC_MSG_NOTICE([Using make installed package $4])

  # Verify that we are using a me installation
  $1_DIST_DENY=yes
  $1_$3_DIR="$$1_WITH_$3"
  SC_CHECK_INSTALL([$1_$3], [true], [true], [true], [true])

  # Set variables for using the subpackage
  $1_$3_AMFLAGS="-I $$1_$3_CFG"
  $1_$3_MK_USE=yes
  $1_$3_MK_INCLUDE="include $$1_$3_ETC/Makefile.$4.mk"
  $1_$3_CPPFLAGS="\$($3_CPPFLAGS)"
  $1_$3_LDADD="$$1_$3_DIR/lib/lib$4.la"
else
  AC_MSG_NOTICE([Building with source of package $4])

  # Prepare for a build using me sources
  if test "x$$1_$3_SOURCE" = x ; then
    if test -f "$1_$3_SOURCE.log" ; then
      $1_$3_SOURCE=`cat $1_$3_SOURCE.log`
    else
      $1_$3_SOURCE="$4"
      $1_$3_SUBDIR="$4"
      AC_CONFIG_SUBDIRS([$4])
    fi
  else
    AC_CONFIG_COMMANDS([$1_$3_SOURCE.log],
                       [echo "$$1_$3_SOURCE" >$1_$3_SOURCE.log])
  fi
  $1_$3_AMFLAGS="-I \$(top_srcdir)/$$1_$3_SOURCE/config"
  $1_$3_MK_INCLUDE="include \${$2_sysconfdir}/Makefile.$4.mk"
  $1_$3_CPPFLAGS="-I$$1_$3_SOURCE/src -I\$(top_srcdir)/$$1_$3_SOURCE/src"
  $1_$3_LDADD="$$1_$3_SOURCE/src/lib$4.la"
fi

dnl Make sure we find the m4 macros provided by me
AC_SUBST([$1_$3_AMFLAGS])

dnl We call make in this subdirectory if not empty
AC_SUBST([$1_$3_SUBDIR])

dnl We will need these variables to compile and link with me
AM_CONDITIONAL([$1_$3_MK_USE], [test "x$$1_$3_MK_USE" != x])
AC_SUBST([$1_$3_MK_INCLUDE])
AC_SUBST([$1_$3_CPPFLAGS])
AC_SUBST([$1_$3_LDADD])
])
