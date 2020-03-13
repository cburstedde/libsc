dnl This is a modified version of the Teuchos config dir from Trilinos
dnl with the following license.
dnl
dnl ***********************************************************************
dnl
dnl                    Teuchos: Common Tools Package
dnl                 Copyright (2004) Sandia Corporation
dnl
dnl Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
dnl license for use of this work by or on behalf of the U.S. Government.
dnl
dnl This library is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU Lesser General Public License as
dnl published by the Free Software Foundation; either version 2.1 of the
dnl License, or (at your option) any later version.
dnl
dnl This library is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Lesser General Public License for more details.
dnl
dnl You should have received a copy of the GNU Lesser General Public
dnl License along with this library; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
dnl USA
dnl Questions? Contact Michael A. Heroux (maherou@sandia.gov)
dnl
dnl ***********************************************************************
dnl
dnl @synopsis SC_BLAS(PREFIX, DGEMM-FUNCTION,
dnl                   [ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl
dnl This macro looks for a library that implements the BLAS
dnl linear-algebra interface (see http://www.netlib.org/blas/).
dnl On success, it sets the BLAS_LIBS output variable to
dnl hold the requisite library linkages.
dnl
dnl To link with BLAS, you should link with:
dnl
dnl 	$BLAS_LIBS $LIBS $FLIBS
dnl
dnl in that order.  FLIBS is the output variable of the
dnl AC_F77_LIBRARY_LDFLAGS macro (called if necessary by SC_BLAS),
dnl and is sometimes necessary in order to link with F77 libraries.
dnl Users will also need to use AC_F77_DUMMY_MAIN (see the autoconf
dnl manual), for the same reason.
dnl
dnl Many libraries are searched for, from ATLAS to CXML to ESSL.
dnl The user may also use --with-blas=<lib> in order to use some
dnl specific BLAS library <lib>.  In order to link successfully,
dnl however, be aware that you will probably need to use the same
dnl Fortran compiler (which can be set via the F77 env. var.) as
dnl was used to compile the BLAS library.
dnl
dnl ACTION-IF-FOUND is a list of shell commands to run if a BLAS
dnl library is found, and ACTION-IF-NOT-FOUND is a list of commands
dnl to run it if it is not found.  If ACTION-IF-FOUND is not specified,
dnl the default action will define HAVE_BLAS.
dnl
dnl This macro requires autoconf 2.50 or later.
dnl
dnl @version $Id: acx_blas.m4,v 1.3 2006/04/21 02:29:27 jmwille Exp $
dnl @author Steven G. Johnson <stevenj@alum.mit.edu>
dnl
dnl Edited by Jim Willenbring on 5-14-2004 to check for dgemm instead of
dnl sgemm.
dnl Edited by Jim Willenbring on 4-17-2006 to stop looking for BLAS if
dnl a specific BLAS library specified by a user cannot be used.

dnl Edited by Carsten Burstedde <carsten@ices.utexas.edu>
dnl Expect the F77_ autoconf macros to be called outside of this file.
dnl Take as argument a mangled DGEMM function to check for.
dnl This way the SC_BLAS macro can be called multiple times
dnl with different Fortran environments to minimize F77 dependencies.
dnl Replaced obsolete AC_TRY_LINK_FUNC macro.
dnl Disabled the PhiPack test since it requires BLAS_LIBS anyway.
dnl Fixed buggy generic Mac OS X library test.

dnl Subroutine to link a program using blas
dnl SC_BLAS_LINK (<added to CHECKING message>)
AC_DEFUN([SC_BLAS_LINK], [
        AC_MSG_CHECKING([for BLAS by linking a C program$1])
        AC_LINK_IFELSE([AC_LANG_PROGRAM(dnl
[[
#ifdef __cplusplus
extern "C"
void $sc_blas_func (char *, char *, int *, int *, int *, double *, double *,
                    int *, double *, int *, double *, double *, int *);
#endif
]], [[
int     i = 1;
double  alpha = 1., beta = 1.;
double  A = 1., B = 1., C = 1.;
$sc_blas_func ("N", "N", &i, &i, &i, &alpha, &A, &i, &B, &i, &beta, &C, &i);
]])],
[AC_MSG_RESULT([successful])],
[AC_MSG_RESULT([failed]); sc_blas_ok=no])
])

dnl The first argument of this macro should be the package prefix.
dnl The second argument of this macro should be a mangled DGEMM function.
AC_DEFUN([SC_BLAS], [
AC_PREREQ(2.50)
dnl Expect this to be called already.
dnl AC_REQUIRE([AC_F77_LIBRARY_LDFLAGS])
dnl AC_REQUIRE([AC_F77_WRAPPERS])
sc_blas_ok=no
user_spec_blas_failed=no

AC_ARG_WITH([blas], [AS_HELP_STRING([--with-blas=<lib>],
            [change default BLAS library to <lib>
             or specify --without-blas to use no BLAS and LAPACK at all])],,
	     [withval=yes])
case $withval in
	yes | "") ;;
	no) sc_blas_ok=disable ;;
	-* | */* | *.a | *.so | *.so.* | *.o) BLAS_LIBS="$withval" ;;
	*) BLAS_LIBS="-l$withval" ;;
esac

dnl Expect the mangled DGEMM function name to be in $2.
sc_blas_func="$2"

sc_blas_save_LIBS="$LIBS"
LIBS="$LIBS $FLIBS"

# First, check BLAS_LIBS environment variable
if test "x$sc_blas_ok" = xno; then
if test "x$BLAS_LIBS" != x; then
	save_LIBS="$LIBS"; LIBS="$BLAS_LIBS $LIBS"
	AC_MSG_CHECKING([for $sc_blas_func in $BLAS_LIBS])
	AC_LINK_IFELSE([AC_LANG_CALL([], [$sc_blas_func])],
                       [sc_blas_ok=yes], [user_spec_blas_failed=yes])
	AC_MSG_RESULT($sc_blas_ok)
	LIBS="$save_LIBS"
fi
fi

# If the user specified a blas library that could not be used we will
# halt the search process rather than risk finding a blas library that
# the user did not specify.

if test "x$user_spec_blas_failed" != xyes; then

# BLAS linked to by default?  (happens on some supercomputers)
if test "x$sc_blas_ok" = xno; then
	AC_CHECK_FUNC($sc_blas_func, [sc_blas_ok=yes])
fi

# BLAS in ATLAS library? (http://math-atlas.sourceforge.net/)
if test "x$sc_blas_ok" = xno; then
	AC_CHECK_LIB(atlas, ATL_xerbla,
		[AC_CHECK_LIB(f77blas, $sc_blas_func,
		[AC_CHECK_LIB(cblas, cblas_dgemm,
			[sc_blas_ok=yes
			 BLAS_LIBS="-lcblas -lf77blas -latlas"],
			[], [-lf77blas -latlas])],
			[], [-latlas])])
fi

# BLAS in PhiPACK libraries? (requires generic BLAS lib, too)
# Disabled since we might want more than sgemm and dgemm.
if test "x$sc_blas_ok" = xno && false ; then
	AC_CHECK_LIB(blas, $dgemm,
		[AC_CHECK_LIB(dgemm, $dgemm,
		[AC_CHECK_LIB(sgemm, $sgemm,
			[sc_blas_ok=yes; BLAS_LIBS="-lsgemm -ldgemm -lblas"],
			[], [-lblas])],
			[], [-lblas])])
fi

# BLAS in Intel MKL library?
if test "x$sc_blas_ok" = xno; then
	AC_CHECK_LIB(mkl, $sc_blas_func, [sc_blas_ok=yes;BLAS_LIBS="-lmkl"])
fi

# BLAS in Apple vecLib library?
if test "x$sc_blas_ok" = xno; then
	save_LIBS="$LIBS"; LIBS="-framework vecLib $LIBS"
	AC_CHECK_FUNC($sc_blas_func, [sc_blas_ok=yes;BLAS_LIBS="-framework vecLib"])
	LIBS="$save_LIBS"
fi

# BLAS in Alpha CXML library?
if test "x$sc_blas_ok" = xno; then
	AC_CHECK_LIB(cxml, $sc_blas_func, [sc_blas_ok=yes;BLAS_LIBS="-lcxml"])
fi

# BLAS in Alpha DXML library? (now called CXML, see above)
if test "x$sc_blas_ok" = xno; then
	AC_CHECK_LIB(dxml, $sc_blas_func, [sc_blas_ok=yes;BLAS_LIBS="-ldxml"])
fi

# BLAS in Sun Performance library?
if test "x$sc_blas_ok" = xno; then
	if test "x$GCC" != xyes; then # only works with Sun CC
		AC_CHECK_LIB(sunmath, acosp,
			[AC_CHECK_LIB(sunperf, $sc_blas_func,
                                [BLAS_LIBS="-xlic_lib=sunperf -lsunmath"
                                 sc_blas_ok=yes],[],[-lsunmath])])
	fi
fi

# BLAS in SCSL library?  (SGI/Cray Scientific Library)
if test "x$sc_blas_ok" = xno; then
	AC_CHECK_LIB(scs, $sc_blas_func, [sc_blas_ok=yes; BLAS_LIBS="-lscs"])
fi

# BLAS in SGIMATH library?
if test "x$sc_blas_ok" = xno; then
	AC_CHECK_LIB(complib.sgimath, $sc_blas_func,
		     [sc_blas_ok=yes; BLAS_LIBS="-lcomplib.sgimath"])
fi

# BLAS in IBM ESSL library? (requires generic BLAS lib, too)
if test "x$sc_blas_ok" = xno; then
	AC_CHECK_LIB(blas, $sc_blas_func,
		[AC_CHECK_LIB(essl, $sc_blas_func,
			[sc_blas_ok=yes; BLAS_LIBS="-lessl -lblas"],
			[], [-lblas $FLIBS])])
fi

# Generic Mac OS X library?
if test "x$sc_blas_ok" = xno; then
	save_LIBS="$LIBS"; LIBS="-framework Accelerate $LIBS"
	AC_CHECK_FUNC($sc_blas_func, [sc_blas_ok=yes
                               BLAS_LIBS="-framework Accelerate"])
	LIBS="$save_LIBS"
fi

# Generic BLAS library?
if test "x$sc_blas_ok" = xno; then
	AC_CHECK_LIB(blas, $sc_blas_func, [sc_blas_ok=yes; BLAS_LIBS="-lblas"])
fi

dnl AC_SUBST(BLAS_LIBS)

fi # If the user specified library wasn't found, we skipped the remaining
   # checks.

LIBS="$sc_blas_save_LIBS"
BLAS_FLIBS=

# Test link a BLAS program
if test "x$sc_blas_ok" = xyes ; then
    dnl Link without FLIBS first
    sc_blas_save_run_LIBS="$LIBS"
    LIBS="$BLAS_LIBS $LIBS"
    SC_BLAS_LINK([ without FLIBS])
    LIBS="$sc_blas_save_run_LIBS"

    if test "x$sc_blas_ok" = xno ; then
        dnl Link with FLIBS it didn't work without
        sc_blas_save_run_LIBS="$LIBS"
        LIBS="$BLAS_LIBS $LIBS $FLIBS"
        sc_blas_ok=yes
        SC_BLAS_LINK([ with FLIBS])
        LIBS="$sc_blas_save_run_LIBS"
        BLAS_FLIBS="$FLIBS"
    fi
fi
dnl Now BLAS_FLIBS may be set or not

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test "x$sc_blas_ok" = xyes ; then
        ifelse([$3],,
               [AC_DEFINE(HAVE_BLAS,1,[Define if you have a BLAS library.])],[$3])
        :
elif test "x$sc_blas_ok" != xdisable ; then
        sc_blas_ok=no
        $4
fi

])
