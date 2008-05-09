dnl

dnl This file is part of the SC Library.
dnl

AC_PREREQ([2.59])
AC_INIT([libsc], [0.1], [ccgo@ices.utexas.edu])
AC_CONFIG_HEADER([src/sc_config.h])
AC_CONFIG_SRCDIR([src/sc.h])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AM_INIT_AUTOMAKE([subdir-objects])

# Options.
SC_ARG_ENABLE([debug], [enable debug mode],
              [SC_DEBUG])
SC_ARG_ENABLE([resize-realloc], [use realloc in array resize],
              [SC_RESIZE_REALLOC])
SC_ARG_DISABLE([blas], [disable all BLAS dependent code],
               [SC_BLAS])
SC_ARG_WITHOUT([getopt], [disable builtin getopt code, provide your own],
               [SC_GETOPT])
SC_ARG_WITHOUT([obstack], [disable builtin obstack code, provide your own],
               [SC_OBSTACK])
SC_ARG_WITHOUT([zlib], [disable builtin zlib code, provide your own],
               [SC_ZLIB])

# Checks for programs.
echo "o----------------------------------"
echo "| Checking for programs"
echo "o----------------------------------"

ACX_MPI([icc gcc cc])
AM_PROG_CC_C_O
AC_PROG_RANLIB
SC_WITH_LINT

dnl For some reason the AS_IF does not disable the AC_F77_* calls.
if test "$sc_enable_SC_BLAS" != "no" ; then
echo "o----------------------------------"
echo "| Checking fortran conventions"
echo "o----------------------------------"
AC_F77_WRAPPERS
AC_F77_LIBRARY_LDFLAGS
fi

# Checks for libraries.
echo "o----------------------------------"
echo "| Checking for libraries"
echo "o----------------------------------"

SC_REQUIRE_LIB([m], [fabs])
AS_IF([test "$sc_with_SC_ZLIB" = "no"],
      [SC_REQUIRE_LIB([z], [adler32_combine])])
AS_IF([test "$sc_enable_SC_BLAS" != "no"],
      [
ACX_BLAS([], [AC_MSG_ERROR([[Cannot find blas library, specify a path using LDFLAGS=-L<DIR> (ex. LDFLAGS=-L/usr/path/lib) or a specific library using BLAS_LIBS=DIR/LIB (for example BLAS_LIBS=/usr/path/lib/libcxml.a)]])])

ACX_LAPACK([], [AC_MSG_ERROR([[Cannot find lapack library, specify a path using LDFLAGSs=-L<DIR> (ex. LDFLAGS=-L/usr/path/lib) or a specific library using LAPACK_LIBS=DIR/LIB (for example LAPACK_LIBS=/usr/path/lib/libcxml.a)]])])
      ])

echo "o----------------------------------"
echo "| Checking headers and functions"
echo "o----------------------------------"

# Checks for header files.
dnl AC_HEADER_STDC
dnl AC_CHECK_HEADERS([ctype.h stdarg.h stddef.h stdint.h stdio.h stdlib.h])
dnl AC_CHECK_HEADERS([signal.h unistd.h])
AC_CHECK_HEADERS([inttypes.h libgen.h getopt.h obstack.h zlib.h])
AC_CHECK_HEADERS([execinfo.h arpa/inet.h netinet/in.h unistd.h])

# Checks for functions.
AS_IF([test "$sc_with_SC_GETOPT" = "no"],
      [SC_REQUIRE_FUNCS([getopt_long])])
AS_IF([test "$sc_with_SC_OBSTACK" = "no"],
      [SC_REQUIRE_FUNCS([obstack_free])])
AC_CHECK_FUNCS([backtrace backtrace_symbols])

# Checks for typedefs, structures, and compiler characteristics.
echo "o----------------------------------"
echo "| Checking keywords and types"
echo "o----------------------------------"

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
AC_CONFIG_FILES([Makefile
                ])
AC_OUTPUT
