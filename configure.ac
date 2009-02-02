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

SC_ARG_OVERRIDE_LOAD([SC])

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
SC_ARG_ENABLE([alloc-page], [align memory on page boundaries], [ALLOC_PAGE])
SC_ARG_ENABLE([alloc-line], [stripe memory between cache lines], [ALLOC_LINE])
SC_ARG_ENABLE([sc-allgather], [internally use replacement for MPI_Allgather],
              [ALLGATHER])
SC_ARG_WITH([papi], [enable Flop counting with papi], [PAPI])

# The getopt check and builtin fallback code can be ignored altogether
AC_ARG_WITH([getopt], [AS_HELP_STRING([--without-getopt],
              [assume external getopt code is found, don't check
               (default: check and use builtin if necessary)])],,
            [withval=yes])
SC_ARG_OVERRIDE_WITH([SC], [GETOPT])
if test "$withval" != "yes" -a "$withval" != "no" ; then
  AC_MSG_ERROR([Please use --without-getopt without an argument])
fi

# The obstack check and builtin fallback code can be ignored altogether
AC_ARG_WITH([obstack], [AS_HELP_STRING([--without-obstack],
              [assume external obstack code is found, don't check
               (default: check and use builtin if necessary)])],,
            [withval=yes])
SC_ARG_OVERRIDE_WITH([SC], [OBSTACK])
if test "$withval" != "yes" -a "$withval" != "no" ; then
  AC_MSG_ERROR([Please use --without-obstack without an argument])
fi

# The zlib check and builtin fallback code can be ignored altogether
AC_ARG_WITH([zlib], [AS_HELP_STRING([--without-zlib],
              [assume external zlib code is found, don't check
               (default: check and use builtin zlib if necessary)])],,
            [withval=yes])
SC_ARG_OVERRIDE_WITH([SC], [ZLIB])
if test "$withval" != "yes" -a "$withval" != "no" ; then
  AC_MSG_ERROR([Please use --without-zlib without an argument])
fi

if test "$SC_ENABLE_ALLOC_PAGE" != no -a \
        "$SC_ENABLE_ALLOC_LINE" != no ; then
  AC_MSG_ERROR([Please enable either alloc-page or alloc-line, not both.])
fi

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

SC_PROVIDE_GETOPT="no"
if test "$SC_WITH_GETOPT" = "yes" ; then
  AC_MSG_CHECKING([for getopt])
  AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[#include <getopt.h>]], [[
int oi;
struct option lo;
getopt_long (0, 0, "abc:", &lo, &oi);
]])], [AC_MSG_RESULT([successful])], [
    AC_MSG_RESULT([failed])
    AC_MSG_NOTICE([Did not find getopt. Activating builtin])
    SC_PROVIDE_GETOPT="yes"
    AC_DEFINE([PROVIDE_GETOPT], 1, [Use builtin getopt])
  ])
fi
AM_CONDITIONAL([SC_PROVIDE_GETOPT], [test "$SC_PROVIDE_GETOPT" = "yes"])

SC_PROVIDE_OBSTACK="no"
if test "$SC_WITH_OBSTACK" = "yes" ; then
  AC_MSG_CHECKING([for obstack])
  AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[#include <obstack.h>]], [[
struct obstack ob;
static void        *(*obstack_chunk_alloc) (size_t) = 0;
static void         (*obstack_chunk_free) (void *) = 0;
obstack_init (&ob);
obstack_free (&ob, 0);
]])], [AC_MSG_RESULT([successful])], [
    AC_MSG_RESULT([failed])
    AC_MSG_NOTICE([Did not find obstack. Activating builtin])
    SC_PROVIDE_OBSTACK="yes"
    AC_DEFINE([PROVIDE_OBSTACK], 1, [Use builtin obstack])
  ])
fi
AM_CONDITIONAL([SC_PROVIDE_OBSTACK], [test "$SC_PROVIDE_OBSTACK" = "yes"])

SC_PROVIDE_ZLIB="no"
if test "$SC_WITH_ZLIB" = "yes" ; then
  AC_CHECK_LIB([z], [adler32_combine],, [
    AC_MSG_NOTICE([Did not find a recent zlib. Activating builtin])
    SC_PROVIDE_ZLIB="yes"
  ])
fi
AM_CONDITIONAL([SC_PROVIDE_ZLIB], [test "$SC_PROVIDE_ZLIB" = "yes"])

# Checks for BLAS (and F77 environment only if necessary).
echo "o---------------------------------------"
echo "| Checking BLAS, LAPACK and F77 support"
echo "o---------------------------------------"

dnl Try a default name mangling scheme without using F77.
dgemm=dgemm_;dgecon=dgecon_
extraf77=no

echo "Checking BLAS without F77 support first"
ACX_BLAS([$dgemm],
         [AC_DEFINE([BLAS], 1, [Define to 1 if BLAS is used])],
         [echo "Could not find BLAS without F77 support"])
for CVAR in `set | awk -F= "/^ac_cv_.*${dgemm}/ { print \\$1 }"` ; do
  # echo "Unset $CVAR"
  unset $CVAR
done
# at this point $acx_blas_ok is either of: yes no disable
if test "$acx_blas_ok" = no ; then

extraf77=yes
echo "Calling F77 autoconf macros"
dnl For some strange reason these wrappers need to be toplevel.
dnl Otherwise they are executed even if the test above fails.
AC_PROG_F77([gfortran g77 fl32 f77 fort77 xlf f90 xlf90])
AC_F77_WRAPPERS
AC_F77_LIBRARY_LDFLAGS
dgemm=;AC_F77_FUNC(dgemm)
if test "$dgemm" = unknown ; then dgemm=dgemm_ ; fi
dgecon=;AC_F77_FUNC(dgecon)
if test "$dgecon" = unknown ; then dgecon=dgecon_ ; fi

echo "Checking BLAS with proper F77 support"
ACX_BLAS([$dgemm],
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
for CVAR in `set | awk -F= "/^ac_cv_.*${dgecon}/ { print \\$1 }"` ; do
  # echo "Unset $CVAR"
  unset $CVAR
done
# at this point $acx_lapack_ok is either of: yes no disable
if test "$acx_lapack_ok" = no ; then

extraf77=yes
echo "Calling F77 autoconf macros"
dnl For some strange reason these wrappers need to be toplevel.
dnl Otherwise they are executed even if the test above fails.
AC_PROG_F77([gfortran g77 fl32 f77 fort77 xlf f90 xlf90])
AC_F77_WRAPPERS
AC_F77_LIBRARY_LDFLAGS
dgecon=;AC_F77_FUNC(dgecon)
if test "$dgecon" = unknown ; then dgecon=dgecon_ ; fi
fi
else acx_lapack_ok=no
fi
if test "$acx_lapack_ok" = no ; then

echo "Checking LAPACK with proper F77 support"
ACX_LAPACK([$dgecon],
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

AC_DEFINE_UNQUOTED(CC,          ["${CC}"],          [C compiler])
AC_DEFINE_UNQUOTED(C_VERSION,   ["${C_VERSION}"],   [C compiler version])
AC_DEFINE_UNQUOTED(CFLAGS,      ["${CFLAGS}"],      [C compiler flags])
AC_DEFINE_UNQUOTED(CPP,         ["${CPP}"],         [C preprocessor])
AC_DEFINE_UNQUOTED(CPPFLAGS,    ["${CPPFLAGS}"],    [C preprocessor flags])
AC_DEFINE_UNQUOTED(F77,         ["${F77}"],         [F77 compiler])
AC_DEFINE_UNQUOTED(FFLAGS,      ["${FFLAGS}"],      [F77 compiler flags])
AC_DEFINE_UNQUOTED(LINK,        ["${LINK}"],        [Linker])
AC_DEFINE_UNQUOTED(LDFLAGS,     ["${LDFLAGS}"],     [Linker flags])
AC_DEFINE_UNQUOTED(BLAS_LIBS,   ["${BLAS_LIBS}"],   [BLAS libraries])
AC_DEFINE_UNQUOTED(LAPACK_LIBS, ["${LAPACK_LIBS}"], [LAPACK libraries])
AC_DEFINE_UNQUOTED(LIBS,        ["${LIBS}"],        [Libraries])
AC_DEFINE_UNQUOTED(FLIBS,       ["${FLIBS}"],       [Fortran libraries])

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
echo "| LINK:        $LINK"
echo "| LDFLAGS:     $LDFLAGS"
echo "| BLAS_LIBS:   $BLAS_LIBS"
echo "| LAPACK_LIBS: $LAPACK_LIBS"
echo "| LIBS:        $LIBS"
echo "| FLIBS:       $FLIBS"
echo "o----------------------------------"

# Create output files.
SC_ARG_OVERRIDE_SAVE([SC])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
