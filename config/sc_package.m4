
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
dnl A program PROG relies on libsc.  In a build from source, libsc usually
dnl resides in PROG's subdirectory sc.  This location can be overridden by
dnl the environment variable PROG_SC_SOURCE; this situation can arise when
dnl both PROG and libsc are subpackages to yet another software.  The
dnl path in PROG_SC_SOURCE must be relative to PROG's toplevel directory.
dnl All of this works without specifying a configure command line option.
dnl However, if libsc is already make install'd in the system and should
dnl be used from there, use --with-sc=<path to libsc install directory>.
dnl In this case, PROG expects subdirectories etc, include, lib, and
dnl share/aclocal, which are routinely created by libsc's make install.
dnl
dnl SC_AS_SUBPACKAGE(PREFIX, prefix)
dnl Call from a package that is using libsc as a subpackage.
dnl Sets PREFIX_DIST_DENY=yes if sc is make install'd.
dnl
AC_DEFUN([SC_AS_SUBPACKAGE],
[
$1_SC_SUBDIR=
$1_SC_MK_USE=
$1_DISTCLEAN="$$1_DISTCLEAN $1_SC_SOURCE.log"

SC_ARG_WITH_PREFIX([sc], [path to installed libsc (optional)], [SC], [$1])

if test "x$$1_WITH_SC" != xno ; then
  AC_MSG_NOTICE([Using make installed libsc])

  # Verify that we are using a libsc installation
  $1_DIST_DENY=yes
  $1_SC_DIR="$$1_WITH_SC"
  SC_CHECK_INSTALL([$1_SC], [true], [true], [true], [true])

  # Set variables for using the subpackage
  $1_SC_AMFLAGS="-I $$1_SC_CFG"
  $1_SC_MK_USE=yes
  $1_SC_MK_INCLUDE="include $$1_SC_ETC/Makefile.sc.mk"
  $1_SC_CPPFLAGS="\$(SC_CPPFLAGS)"
  $1_SC_LDADD="\$(SC_LDFLAGS) -lsc"
else
  AC_MSG_NOTICE([Building with sc source])

  # Prepare for a build using sc sources
  if test "x$$1_SC_SOURCE" = x ; then
    if test -f "$1_SC_SOURCE.log" ; then
      $1_SC_SOURCE=`cat $1_SC_SOURCE.log`
    else
      $1_SC_SOURCE="sc"
      $1_SC_SUBDIR="sc"
      AC_CONFIG_SUBDIRS([sc])
    fi
  else
    AC_CONFIG_COMMANDS([$1_SC_SOURCE.log],
                       [echo "$$1_SC_SOURCE" >$1_SC_SOURCE.log])
  fi
  $1_SC_AMFLAGS="-I \$(top_srcdir)/$$1_SC_SOURCE/config"
  $1_SC_MK_INCLUDE="include \${$2_sysconfdir}/Makefile.sc.mk"
  $1_SC_CPPFLAGS="-I\$(top_builddir)/$$1_SC_SOURCE/src \
-I\$(top_srcdir)/$$1_SC_SOURCE/src"
  $1_SC_LDADD="\$(top_builddir)/$$1_SC_SOURCE/src/libsc.la"
fi

dnl Make sure we find the m4 macros provided by libsc
AC_SUBST([$1_SC_AMFLAGS])

dnl We call make in this subdirectory if not empty
AC_SUBST([$1_SC_SUBDIR])

dnl We will need these variables to compile and link with libsc
AM_CONDITIONAL([$1_SC_MK_USE], [test "x$$1_SC_MK_USE" != x])
AC_SUBST([$1_SC_MK_INCLUDE])
AC_SUBST([$1_SC_CPPFLAGS])
AC_SUBST([$1_SC_LDADD])
])
