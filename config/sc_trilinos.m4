
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

dnl SC_TRILINOS_PACKAGE_DEFS([Package], [PACKAGE], [PREFIX])
dnl define PREFIX_PACKAGE_{CPPFLAGS,LDFLAGS,LIBS} for use with Trilinos export
dnl Makefiles
AC_DEFUN([SC_TRILINOS_PACKAGE_DEFS],
[
dnl for Trilinos 9, use PACKAGE_{INCLUDES,LIBS}
  if test "$$3_TRILINOS_VERSION" = "9" ; then
    $3_$2_CPPFLAGS="\$($2_INCLUDES)"
    $3_$2_LDFLAGS=""
    $3_$2_LIBS="\$($2_LIBS)"
  else
    AC_MSG_NOTICE([TRILINOS_MINOR_VERSION $$3_TRILINOS_MINOR_VERSION])
    case "$$3_TRILINOS_MINOR_VERSION" in
dnl 0 and 2 are the only official releases with all-caps
    0[[0-2]])
      $3_$2_CPPFLAGS="\$($2_INCLUDE_DIRS) \$($2_TPL_INCLUDE_DIRS)"
      $3_$2_LDFLAGS="\$($2_SHARED_LIB_RPATH_COMMAND) \$($2_EXTRA_LD_FLAGS) "\
"\$($2_LIBRARY_DIRS) \$($2_TPL_LIBRARY_DIRS)"
      $3_$2_LIBS="\$($2_LIBRARIES)"
      ;;
    *)
      $3_$2_CPPFLAGS="\$($1_INCLUDE_DIRS) \$($1_TPL_INCLUDE_DIRS)"
      $3_$2_LDFLAGS="\$($1_SHARED_LIB_RPATH_COMMAND) \$($1_EXTRA_LD_FLAGS) "\
"\$($1_LIBRARY_DIRS) \$($1_TPL_LIBRARY_DIRS)"
      $3_$2_LIBS="\$($1_LIBRARIES)"
      ;;
    esac
  fi
  AC_SUBST([$3_$2_CPPFLAGS])
  AC_SUBST([$3_$2_LDFLAGS])
  AC_SUBST([$3_$2_LIBS])
])

dnl SC_TRILINOS([PREFIX], [EXTRA_PACKAGES])
dnl EXTRA_PACKAGES can be empty or contain a comma-separated list
dnl of trilinos packages in uppercase.
dnl Currently only ML is recognized.
dnl
AC_DEFUN([SC_TRILINOS],
[
$1_TRILINOS_VERSION=
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
    TRILINOS_HEADER="$$1_TRILINOS_DIR/include/Trilinos_version.h"
    if test ! -f "$TRILINOS_HEADER" ; then
      AC_MSG_ERROR([Header file $TRILINOS_HEADER not found])
    fi
    if grep -qs 'TRILINOS_MAJOR_VERSION[[[:space:]+]]10' "$TRILINOS_HEADER"
    then
      $1_TRILINOS_VERSION=10
      $1_TRILINOS_CPPFLAGS="-I$$1_TRILINOS_DIR/include"
      AC_SUBST([$1_TRILINOS_CPPFLAGS])
      $1_TRILINOS_LDFLAGS="-L$$1_TRILINOS_DIR/lib"
      AC_SUBST([$1_TRILINOS_LDFLAGS])
      $1_TRILINOS_MINOR_VERSION=`grep -o 'TRILINOS_MAJOR_MINOR_VERSION 10[[0-9]]\{2\}' "$TRILINOS_HEADER" | sed "s/.* 10//"`
      AC_MSG_NOTICE([TRILINOS_MINOR_VERSION $$1_TRILINOS_MINOR_VERSION])
    elif grep -qs 'TRILINOS_MAJOR_VERSION[[[:space:]+]]9' "$TRILINOS_HEADER"
    then
      $1_TRILINOS_VERSION=9
    else
      AC_MSG_ERROR([Trilinos version not recognized])
    fi
    SC_TRILINOS_CHECK_MK([epetra], [Epetra], [EPETRA], [$1])
    SC_TRILINOS_CHECK_MK([teuchos], [Teuchos], [TEUCHOS], [$1])
    SC_TRILINOS_PACKAGE_DEFS([Epetra], [EPETRA], [$1])
    SC_TRILINOS_PACKAGE_DEFS([Teuchos], [TEUCHOS], [$1])
    m4_foreach([PKG], [$2], [
      if test "PKG" = "ML" ; then
        SC_TRILINOS_CHECK_MK([ml], [ML], [ML], [$1])
        SC_TRILINOS_PACKAGE_DEFS([ML], [ML], [$1])
      fi
    ])
    AC_MSG_RESULT([version $$1_TRILINOS_VERSION])
  fi
fi
AM_CONDITIONAL([$1_TRILINOS_9], [test "$$1_TRILINOS_VERSION" = 9])
AM_CONDITIONAL([$1_TRILINOS_10], [test "$$1_TRILINOS_VERSION" = 10])
AM_CONDITIONAL([$1_TRILINOS_ML], [test -n "$$1_TRILINOS_MK_ML"])
])
