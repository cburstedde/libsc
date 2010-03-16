
dnl trilinos.m4 - general custom macros
dnl
dnl This file is part of the SC Library.
dnl The SC library provides support for parallel scientific applications.
dnl
dnl Copyright (C) 2008-2010 Carsten Burstedde, Lucas Wilcox.

dnl Documentation for macro names: brackets indicate optional arguments

dnl SC_TRILINOS_CHECK_MK(package, Package, PACKAGE, PREFIX)
dnl Check for the Makefile of a trilinos package
dnl Requires variable SC_TRILINOS_DIR pointing to a trilinos installation
dnl
AC_DEFUN([SC_TRILINOS_CHECK_MK],
[
dnl Trilinos <= 9
$4_TRILINOS_MK_$3="$$4_TRILINOS_DIR/include/Makefile.export.$1"
if test ! -f "$$4_TRILINOS_MK_$3" ; then
  dnl Trilinos 10
  $4_TRILINOS_MK_$3="$$4_TRILINOS_DIR/include/Makefile.export.$2"
  if test ! -f "$$4_TRILINOS_MK_$3" ; then
    AC_MSG_ERROR([$$4_TRILINOS_MK_$3 not found])
  fi
fi
AC_SUBST([$4_TRILINOS_MK_$3])
])

dnl SC_TRILINOS([PREFIX], [EXTRA_PACKAGES])
dnl EXTRA_PACKAGES can be empty or contain a comma-separated list
dnl of trilinos packages in uppercase.
dnl Currently only ML is recognized.
dnl
AC_DEFUN([SC_TRILINOS],
[
SC_ARG_WITH_PREFIX([trilinos], [set <dir> to Trilinos installation],
                   [TRILINOS], [$1], [=<dir>])
if test "$$1_WITH_TRILINOS" != "no" ; then
  if test "$$1_WITH_TRILINOS" = "yes" ; then
    AC_MSG_ERROR([Please specify Trilinos installation --with-trilinos=<dir>])
  else
    AC_MSG_CHECKING([Trilinos include directory and Makefiles])
    $1_TRILINOS_DIR="$$1_WITH_TRILINOS"
    if test ! -d "$$1_TRILINOS_DIR" ; then
      AC_MSG_ERROR([$$1_TRILINOS_DIR not found])
    fi
    if test ! -d "$$1_TRILINOS_DIR/include" ; then
      AC_MSG_ERROR([$$1_TRILINOS_DIR/include not found])
    fi
    if test ! -d "$$1_TRILINOS_DIR/lib" ; then
      AC_MSG_ERROR([$$1_TRILINOS_DIR/lib not found])
    fi
    if test -f "$$1_TRILINOS_DIR/include/Makefile.export.epetra" ; then
      $1_TRILINOS_VERSION=9
    else
      $1_TRILINOS_VERSION=10
      $1_TRILINOS_CPPFLAGS="-I$$1_TRILINOS_DIR/include"
      AC_SUBST([$1_TRILINOS_CPPFLAGS])
      $1_TRILINOS_LDFLAGS="-L$$1_TRILINOS_DIR/lib"
      AC_SUBST([$1_TRILINOS_LDFLAGS])
    fi
    SC_TRILINOS_CHECK_MK([epetra], [Epetra], [EPETRA], [$1])
    SC_TRILINOS_CHECK_MK([teuchos], [Teuchos], [TEUCHOS], [$1])
    m4_foreach([PKG], [$2], [
      if test "PKG" = "ML" ; then
        SC_TRILINOS_CHECK_MK([ml], [ML], [ML], [$1])
      fi
    ])
    AC_MSG_RESULT([version $$1_TRILINOS_VERSION])
  fi
fi
AM_CONDITIONAL([$4_TRILINOS_ML], [test -n "$4_TRILINOS_MK_ML"])
])
