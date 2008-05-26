dnl
dnl This file is part of the SC Library.
dnl

AC_INIT([libsc], [0.5], [ccgo@ices.utexas.edu])
AC_PREREQ([2.59])
AC_CONFIG_HEADERS([src/pre_config.h])
AC_CONFIG_SRCDIR([src/sc.h])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AM_INIT_AUTOMAKE([subdir-objects])
AX_PREFIX_CONFIG_H([src/sc_config.h], [SC])

# Process command line options.
echo "o---------------------------------------"
echo "| Processing options"
echo "o---------------------------------------"

AC_ARG_ENABLE([logging],
              [AC_HELP_STRING([--enable-logging=PRIO], [\
change log priority (see sc.h for possible values)])],
              [case "$enableval" in
                 yes) AC_MSG_ERROR([\
See sc.h for possible log priorities in --enable-logging=PRIO])
                 ;;
                 no) AC_DEFINE([SC_LOG_PRIORITY], [SC_LP_SILENT],
                               [minimal log priority])
                 ;;
                 *) AC_DEFINE_UNQUOTED([SC_LOG_PRIORITY], [$enableval],
                                       [minimal log priority])
               esac])
SC_ARG_ENABLE([debug], [enable debug mode (assertions and extra checks)],
              [DEBUG])
SC_ARG_ENABLE([resize-realloc], [use realloc in array resize],
              [RESIZE_REALLOC])
SC_ARG_WITHOUT([blas-code], [disable all BLAS dependent code],
               [BLAS])
SC_ARG_WITHOUT([getopt], [disable builtin getopt code, provide your own],
               [PROVIDE_GETOPT])
SC_ARG_WITHOUT([obstack], [disable builtin obstack code, provide your own],
               [PROVIDE_OBSTACK])
SC_ARG_WITHOUT([zlib], [disable builtin zlib code, provide your own],
               [PROVIDE_ZLIB])

# Checks for MPI C compiler and other programs (no F77 at this point!).
echo "o---------------------------------------"
echo "| Checking for MPI and other programs"
echo "o---------------------------------------"

ACX_MPI([SC], [icc gcc cc])
AM_PROG_CC_C_O
AC_PROG_RANLIB
ACX_WITH_LINT

# Checks for libraries.
echo "o---------------------------------------"
echo "| Checking for libraries"
echo "o---------------------------------------"

SC_REQUIRE_LIB([m], [fabs])
if test "$SC_WITH_PROVIDE_ZLIB" = "no" ; then
  SC_REQUIRE_LIB([z], [adler32_combine])
fi

# Checks for BLAS (and F77 environment only if necessary).
if test "$SC_WITH_BLAS" != "no" ; then

echo "o---------------------------------------"
echo "| Checking for BLAS and F77 support"
echo "o---------------------------------------"

dnl Try a default name mangling scheme without using F77.
dgemm=dgemm_;dgecon=dgecon_

echo "Checking BLAS without F77 support first"
ACX_BLAS([$dgemm],,[
echo "Could not find BLAS without F77 support"
])
if test "$acx_blas_ok" = no ; then

echo "Calling F77 autoconf macros"
dnl For some strange reason these wrappers need to be toplevel.
dnl Otherwise they are executed even if the test above fails.
AC_F77_WRAPPERS
AC_F77_LIBRARY_LDFLAGS
dgemm=;AC_F77_FUNC(dgemm)
dgecon=;AC_F77_FUNC(dgecon)

echo "Checking BLAS with proper F77 support"
ACX_BLAS([$dgemm],,[AC_MSG_ERROR([[\
Cannot find blas library, specify a path using LIBS=-L<DIR> (ex.\
 LIBS=-L/usr/path/lib) or a specific library using BLAS_LIBS=DIR/LIB\
 (for example BLAS_LIBS=/usr/path/lib/libcxml.a)]])])
fi

ACX_LAPACK([$dgecon],,[AC_MSG_ERROR([[\
Cannot find lapack library, specify a path using LIBS=-L<DIR> (ex.\
 LIBS=-L/usr/path/lib) or a specific library using LAPACK_LIBS=DIR/LIB\
 (for example LAPACK_LIBS=/usr/path/lib/libcxml.a)]])])
fi

# Checks for functions.
echo "o---------------------------------------"
echo "| Checking functions"
echo "o---------------------------------------"

if test "$SC_WITH_PROVIDE_GETOPT" = "no" ; then
  SC_REQUIRE_FUNCS([getopt_long])
fi
if test "$SC_WITH_PROVIDE_OBSTACK" = "no" ; then
  SC_REQUIRE_FUNCS([obstack_free])
fi

AC_CHECK_FUNCS([backtrace backtrace_symbols])

# Checks for header files.
echo "o---------------------------------------"
echo "| Checking headers"
echo "o---------------------------------------"

dnl AC_HEADER_STDC
dnl AC_CHECK_HEADERS([ctype.h stdarg.h stddef.h stdint.h stdio.h stdlib.h])
dnl AC_CHECK_HEADERS([signal.h unistd.h])
AC_CHECK_HEADERS([ctype.h inttypes.h libgen.h getopt.h obstack.h mpi.h zlib.h])
AC_CHECK_HEADERS([execinfo.h arpa/inet.h netinet/in.h unistd.h])
AC_CHECK_HEADERS([sys/time.h time.h])

# Checks for typedefs, structures, and compiler characteristics.
echo "o---------------------------------------"
echo "| Checking keywords and types"
echo "o---------------------------------------"

AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

echo "o----------------------------------"
echo "| Results are"
echo "o----------------------------------"
echo "| CC:          $CC"
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
