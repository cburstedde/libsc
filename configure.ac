dnl
dnl This file is part of the SC Library.
dnl

AC_INIT([libsc],
        [m4_esyscmd([config/git-version-gen .tarball-version])],
        [ccgo@ices.utexas.edu])
AC_PREREQ(2.61)
AC_CONFIG_HEADERS([src/pre_config.h])
AC_CONFIG_SRCDIR([src/sc.h])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AC_PREFIX_DEFAULT([$PWD/local])
AM_INIT_AUTOMAKE([subdir-objects])
AX_PREFIX_CONFIG_H([src/sc_config.h], [SC])

# Process command line options.
echo "o---------------------------------------"
echo "| Processing options"
echo "o---------------------------------------"

AC_ARG_ENABLE([logging],
              [AS_HELP_STRING([--enable-logging=PRIO], [\
change log threshold (see sc.h for possible values)])],
              [case "$enableval" in
                 yes) AC_MSG_ERROR([\
See sc.h for possible log priorities in --enable-logging=PRIO])
                 ;;
                 no) AC_DEFINE([LOG_PRIORITY], [SC_LP_SILENT],
                               [minimal log priority])
                 ;;
                 *) AC_DEFINE_UNQUOTED([LOG_PRIORITY], [$enableval],
                                       [minimal log priority])
               esac])
SC_ARG_ENABLE([debug], [enable debug mode (assertions and extra checks)],
              [DEBUG])
SC_ARG_ENABLE([resize-realloc], [use realloc in array resize],
              [RESIZE_REALLOC])
SC_ARG_DISABLE([sc-allgather], [use stock MPI_Allgather], [ALLGATHER])
SC_ARG_DISABLE([options], [disable the libsc option parser],
              [OPTIONS])
SC_ARG_WITH([papi], [enable Flop counting with papi], [PAPI])
SC_ARG_WITH([getopt], [enable builtin getopt code], [PROVIDE_GETOPT])
SC_ARG_WITHOUT([obstack], [disable builtin obstack code, provide your own],
               [PROVIDE_OBSTACK])
SC_ARG_WITHOUT([zlib], [disable builtin zlib code, provide your own],
               [PROVIDE_ZLIB])

# Checks for MPI C compiler and other programs (no F77 at this point!).
echo "o---------------------------------------"
echo "| Checking MPI and other programs"
echo "o---------------------------------------"

ACX_MPI([SC], [icc gcc cc])
AM_PROG_CC_C_O
AC_PROG_RANLIB
ACX_WITH_LINT
ACX_C_RESTRICT
AX_C_VERSION

# Checks for libraries.
echo "o---------------------------------------"
echo "| Checking libraries"
echo "o---------------------------------------"

SC_REQUIRE_LIB([m], [fabs])
if test "$SC_WITH_PROVIDE_ZLIB" = "no" ; then
  SC_REQUIRE_LIB([z], [adler32_combine])
fi

# Checks for BLAS (and F77 environment only if necessary).
echo "o---------------------------------------"
echo "| Checking BLAS, LAPACK and F77 support"
echo "o---------------------------------------"

dnl Try a default name mangling scheme without using F77.
dgemv=dgemv_;dgemm=dgemm_
dgecon=dgecon_;dpotri=dpotri_
extraf77=no

echo "Checking BLAS without F77 support first"
ACX_BLAS([$dgemm],
         [AC_DEFINE([BLAS], 1, [Define to 1 if BLAS is used])],
         [echo "Could not find BLAS without F77 support"])
# at this point $acx_blas_ok is either of: yes no disable
if test "$acx_blas_ok" = no ; then

extraf77=yes
echo "Calling F77 autoconf macros"
dnl For some strange reason these wrappers need to be toplevel.
dnl Otherwise they are executed even if the test above fails.
AC_PROG_F77([gfortran g77 fl32 f77 fort77 xlf f90 xlf90])
AC_F77_WRAPPERS
AC_F77_LIBRARY_LDFLAGS
dgemv=;AC_F77_FUNC(dgemv)
if test "$dgemv" = unknown ; then dgemv=dgemv_ ; fi
dgecon=;AC_F77_FUNC(dgecon)
if test "$dgecon" = unknown ; then dgecon=dgecon_ ; fi
dpotri=;AC_F77_FUNC(dpotri)
if test "$dpotri" = unknown ; then dpotri=dpotri_ ; fi

echo "Checking BLAS with proper F77 support"
ACX_BLAS([$dgemv],
         [AC_DEFINE([BLAS], 1, [Define to 1 if BLAS is used])],
         [AC_MSG_ERROR([[\
Cannot find BLAS library, specify a path using LIBS=-L<DIR> (ex.\
 LIBS=-L/usr/path/lib) or a specific library using BLAS_LIBS=DIR/LIB\
 (for example BLAS_LIBS=/usr/path/lib/libcxml.a)]])])
fi
# at this point $acx_blas_ok is either of: yes disable
if test "$acx_blas_ok" = disable ; then
        echo "Not using BLAS"
fi
AM_CONDITIONAL([SC_BLAS], [test "$acx_blas_ok" = yes])

if test "$extraf77" = no ; then
echo "Checking LAPACK without F77 support first"
ACX_LAPACK([$dgecon],
           [AC_DEFINE([LAPACK], 1, [Define to 1 if LAPACK is used])],
           [echo "Could not find LAPACK without F77 support"])
# at this point $acx_lapack_ok is either of: yes no disable
if test "$acx_lapack_ok" = no ; then

extraf77=yes
echo "Calling F77 autoconf macros"
dnl For some strange reason these wrappers need to be toplevel.
dnl Otherwise they are executed even if the test above fails.
AC_PROG_F77([gfortran g77 fl32 f77 fort77 xlf f90 xlf90])
AC_F77_WRAPPERS
AC_F77_LIBRARY_LDFLAGS
dpotri=;AC_F77_FUNC(dpotri)
if test "$dpotri" = unknown ; then dpotri=dpotri_ ; fi
fi
else acx_lapack_ok=no
fi
if test "$acx_lapack_ok" = no ; then

echo "Checking LAPACK with proper F77 support"
ACX_LAPACK([$dpotri],
           [AC_DEFINE([LAPACK], 1, [Define to 1 if LAPACK is used])],
           [AC_MSG_ERROR([[\
Cannot find LAPACK library, specify a path using LIBS=-L<DIR> (ex.\
 LIBS=-L/usr/path/lib) or a specific library using LAPACK_LIBS=DIR/LIB\
 (for example LAPACK_LIBS=/usr/path/lib/libcxml.a)]])])
fi
# at this point $acx_lapack_ok is either of: yes disable
if test "$acx_lapack_ok" = disable ; then
        echo "Not using LAPACK"
fi
AM_CONDITIONAL([SC_LAPACK], [test "$acx_lapack_ok" = yes])

# Checks for header files.
echo "o---------------------------------------"
echo "| Checking headers"
echo "o---------------------------------------"

AC_CHECK_HEADERS([execinfo.h signal.h sys/time.h time.h])

# Checks for functions.
echo "o---------------------------------------"
echo "| Checking functions"
echo "o---------------------------------------"

if test "$SC_WITH_PROVIDE_GETOPT" = "no" ; then
  AC_CHECK_FUNC([getopt_long], ,AC_DEFINE([PROVIDE_GETOPT], 1, [Define to 1 if getopt is provided by sc]))
fi
if test "$SC_WITH_PROVIDE_OBSTACK" = "no" ; then
  SC_REQUIRE_FUNCS([obstack_free])
fi

AC_CHECK_FUNCS([backtrace backtrace_symbols])
AC_CHECK_FUNCS([tgamma trunc])

# Checks for typedefs, structures, and compiler characteristics.
echo "o---------------------------------------"
echo "| Checking keywords and types"
echo "o---------------------------------------"

AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

echo "o----------------------------------"
echo "| Results are"
echo "o----------------------------------"
echo "| CC:          $CC"
echo "| C_VERSION:   $C_VERSION"
echo "| CFLAGS:      $CFLAGS"
echo "| CPP:         $CPP"
echo "| CPPFLAGS:    $CPPFLAGS"
echo "| F77:         $F77"
echo "| FFLAGS:      $FFLAGS"
echo "| LDFLAGS:     $LDFLAGS"
echo "| BLAS_LIBS:   $BLAS_LIBS"
echo "| LAPACK_LIBS: $LAPACK_LIBS"
echo "| LIBS:        $LIBS"
echo "| FLIBS:       $FLIBS"
echo "o----------------------------------"

# Create output files.
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
