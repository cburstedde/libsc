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
SC_ARG_ENABLE([debug], [enable debug mode], [SC_DEBUG])
SC_ARG_ENABLE([resize-realloc], [use realloc in array resize],
              [SC_RESIZE_REALLOC])

# Checks for programs.
echo "o----------------------------------"
echo "| Checking for programs"
echo "o----------------------------------"

ACX_MPI([icc gcc cc])
SC_WITH_LINT
AM_PROG_CC_C_O
AC_PROG_RANLIB

# Checks for libraries.
echo "o----------------------------------"
echo "| Checking for libraries"
echo "o----------------------------------"

SC_REQUIRE_LIB([m], [fabs])
SC_REQUIRE_LIB([z], [adler32_combine])

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
SC_REQUIRE_FUNCS([getopt_long obstack_free])
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
echo "| LDFLAGS:     $LDFLAGS"
echo "| LIBS:        $LIBS"
echo "o----------------------------------"

# Create output files.
AC_CONFIG_FILES([Makefile
                ])
AC_OUTPUT
