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
dnl @synopsis ACX_LAPACK(LAPACK_FUNCTION, [ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl
dnl This macro looks for a library that implements the LAPACK
dnl linear-algebra interface (see http://www.netlib.org/lapack/).
dnl On success, it sets the LAPACK_LIBS output variable to
dnl hold the requisite library linkages.
dnl
dnl To link with LAPACK, you should link with:
dnl
dnl     $LAPACK_LIBS $BLAS_LIBS $LIBS $FLIBS
dnl
dnl in that order.  BLAS_LIBS is the output variable of the ACX_BLAS
dnl macro, called automatically.  FLIBS is the output variable of the
dnl AC_F77_LIBRARY_LDFLAGS macro (called if necessary by ACX_BLAS),
dnl and is sometimes necessary in order to link with F77 libraries.
dnl Users will also need to use AC_F77_DUMMY_MAIN (see the autoconf
dnl manual), for the same reason.
dnl
dnl The user may also use --with-lapack=<lib> in order to use some
dnl specific LAPACK library <lib>.  In order to link successfully,
dnl however, be aware that you will probably need to use the same
dnl Fortran compiler (which can be set via the F77 env. var.) as
dnl was used to compile the LAPACK and BLAS libraries.
dnl
dnl ACTION-IF-FOUND is a list of shell commands to run if a LAPACK
dnl library is found, and ACTION-IF-NOT-FOUND is a list of commands
dnl to run it if it is not found.  If ACTION-IF-FOUND is not specified,
dnl the default action will define HAVE_LAPACK.
dnl
dnl @version $Id: acx_lapack.m4,v 1.3 2006/04/21 02:29:27 jmwille Exp $
dnl @author Steven G. Johnson <stevenj@alum.mit.edu>
dnl edited by Jim Willenbring <jmwille@sandia.gov> to check for sgecon
dnl rather than cheev because by default (as of 8-13-2002) Trilinos
dnl does not build the complex portions of the lapack library.  Edited
dnl again on 5-13-2004 to check for dgecon instead of sgecon.
dnl Edited by Jim Willenbring on 4-17-2006 to stop looking for LAPACK if
dnl a specific LAPACK library specified by a user cannot be used.

dnl Edited by Carsten Burstedde <carsten@ices.utexas.edu>
dnl Expect the F77_ autoconf macros to be called outside of this file.
dnl Take as argument a mangled LAPACK function to check for.
dnl This way the ACX_LAPACK macro can be called multiple times
dnl with different Fortran environments to minimize F77 dependencies.
dnl Replaced obsolete AC_TRY_LINK_FUNC macro.

dnl The first argument of this macro should be a mangled LAPACK function.
AC_DEFUN([ACX_LAPACK], [
AC_REQUIRE([ACX_BLAS])
acx_lapack_ok=no
user_spec_lapack_failed=no

AC_ARG_WITH([lapack], [AC_HELP_STRING([--with-lapack=<lib>],
            [change default LAPACK library to <lib>
             or specify --without-lapack to use no LAPACK at all])])
case $with_lapack in
        yes | "") ;;
        no) acx_lapack_ok=disable ;;
        -* | */* | *.a | *.so | *.so.* | *.o) LAPACK_LIBS="$with_lapack" ;;
        *) LAPACK_LIBS="-l$with_lapack" ;;
esac

# Get fortran linker name of LAPACK function to check for.
dnl AC_F77_FUNC(dgecon)
dnl Expect the mangled LAPACK function name to be in $1.
acx_lapack_func="$1"

# We cannot use LAPACK if BLAS is not found
if test "$acx_blas_ok" = disable ; then
        acx_lapack_ok=disable
elif test "$acx_blas_ok" != yes; then
        acx_lapack_ok=noblas
fi

# First, check LAPACK_LIBS environment variable
if test "$acx_lapack_ok" = no; then
if test "x$LAPACK_LIBS" != x; then
        save_LIBS="$LIBS"; LIBS="$LAPACK_LIBS $BLAS_LIBS $LIBS $FLIBS"
        AC_MSG_CHECKING([for $acx_lapack_func in $LAPACK_LIBS])
	AC_LINK_IFELSE([AC_LANG_CALL([], [$acx_lapack_func])],
                       [acx_lapack_ok=yes], [user_spec_lapack_failed=yes])
        AC_MSG_RESULT($acx_lapack_ok)
        LIBS="$save_LIBS"
        if test acx_lapack_ok = no; then
                LAPACK_LIBS=""
        fi
fi
fi

# If the user specified a LAPACK library that could not be used we will
# halt the search process rather than risk finding a LAPACK library that
# the user did not specify.

if test "x$user_spec_lapack_failed" != xyes; then

# LAPACK linked to by default?  (is sometimes included in BLAS lib)
if test $acx_lapack_ok = no; then
        save_LIBS="$LIBS"; LIBS="$BLAS_LIBS $LIBS $FLIBS"
        AC_CHECK_FUNC($acx_lapack_func, [acx_lapack_ok=yes])
        LIBS="$save_LIBS"
fi

# Generic LAPACK library?
for lapack in lapack lapack_rs6k; do
        if test $acx_lapack_ok = no; then
                save_LIBS="$LIBS"; LIBS="$BLAS_LIBS $LIBS"
                AC_CHECK_LIB($lapack, $acx_lapack_func,
                    [acx_lapack_ok=yes; LAPACK_LIBS="-l$lapack"], [], [$FLIBS])
                LIBS="$save_LIBS"
        fi
done

AC_SUBST(LAPACK_LIBS)

fi # If the user specified library wasn't found, we skipped the remaining
   # checks.

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test "$acx_lapack_ok" = yes; then
        ifelse([$2],,[AC_DEFINE(HAVE_LAPACK,1,[Define if you have LAPACK library.])],[$2])
        :
elif test "$acx_lapack_ok" != disable ; then
        acx_lapack_ok=no
        $3
fi

])dnl ACX_LAPACK
