dnl
dnl This file is part of the SC Library.
dnl

AC_INIT([libsc],
        [m4_esyscmd([config/git-version-gen .tarball-version])],
        [info@p4est.org])
AC_PREREQ(2.61)
AC_CONFIG_HEADERS([src/pre_config.h])
AC_CONFIG_SRCDIR([src/sc.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([config])
AC_PREFIX_DEFAULT([$PWD/local])
AX_PREFIX_CONFIG_H([src/sc_config.h], [SC])
AM_INIT_AUTOMAKE([parallel-tests subdir-objects])
AM_SILENT_RULES
SC_VERSION([SC])

# Process command line options.
echo "o---------------------------------------"
echo "| Checking options"
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
SC_ARG_WITH([papi], [enable Flop counting with papi], [PAPI])

# Checks for MPI C compiler and other programs (no F77 at this point!).
echo "o---------------------------------------"
echo "| Checking MPI and other programs"
echo "o---------------------------------------"

dnl A nonempty second/third argument causes to enable F77/CXX, respectively.
SC_MPI_CONFIG([SC], [yes], [])
SC_MPI_ENGAGE([SC])
# This is needed for compatibility with automake >= 1.12
m4_ifdef([AM_PROG_AR],[AM_PROG_AR])
SC_PROG_LINT
SC_C_VERSION
LT_INIT

# Checks for libraries.
echo "o---------------------------------------"
echo "| Checking libraries"
echo "o---------------------------------------"

SC_REQUIRE_LIB([m], [fabs])
SC_CHECK_LIB([z], [adler32_combine], [ZLIB], [SC])
SC_CHECK_LIB([lua52 lua5.2 lua51 lua5.1 lua lua5], [lua_createtable],
	     [LUA], [SC])

# Checks for header files.
echo "o---------------------------------------"
echo "| Checking headers"
echo "o---------------------------------------"

AC_CHECK_HEADERS([execinfo.h signal.h sys/time.h sys/types.h time.h])
AC_CHECK_HEADERS([lua.h lua5.1/lua.h lua5.2/lua.h])

# Checks for functions.
echo "o---------------------------------------"
echo "| Checking functions"
echo "o---------------------------------------"

AC_CHECK_FUNCS([backtrace backtrace_symbols])

# Checks for BLAS (and F77 environment only if necessary).
echo "o---------------------------------------"
echo "| Checking BLAS, LAPACK and F77 support"
echo "o---------------------------------------"

m4_ifset([SC_CHECK_MPI_F77], [
AC_F77_LIBRARY_LDFLAGS
AC_F77_WRAPPERS
]) 

m4_ifset([SC_BLA_BOGUS], [
dnl Try a default name mangling scheme without using F77.
dgemm=dgemm_;dgecon=dgecon_
extraf77=no

AC_MSG_NOTICE([Checking BLAS without F77 support first])
SC_BLAS([SC], [$dgemm],
        [AC_DEFINE([BLAS], 1, [Define to 1 if BLAS is used])],
        [echo "Could not find BLAS without F77 support"])
for CVAR in `set | awk -F= "/^ac_cv_.*${dgemm}/ { print \\$1 }"` ; do
  # echo "Unset $CVAR"
  unset $CVAR
done
# at this point $sc_blas_ok is either of: yes no disable
if test "$sc_blas_ok" = no ; then

extraf77=yes
AC_MSG_NOTICE([Calling F77 autoconf macros])
dnl For some strange reason these wrappers need to be toplevel.
dnl Otherwise they are executed even if the test above fails.
AC_PROG_F77([gfortran g77 fl32 f77 fort77 xlf f90 xlf90])
AC_F77_WRAPPERS
AC_F77_LIBRARY_LDFLAGS
dgemm=;AC_F77_FUNC(dgemm)
if test "$dgemm" = unknown ; then dgemm=dgemm_ ; fi
dgecon=;AC_F77_FUNC(dgecon)
if test "$dgecon" = unknown ; then dgecon=dgecon_ ; fi

AC_MSG_NOTICE([Checking BLAS with proper F77 support])
SC_BLAS([SC], [$dgemm],
        [AC_DEFINE([BLAS], 1, [Define to 1 if BLAS is used])],
        [AC_MSG_ERROR([[\
Cannot find BLAS library, specify a path using LIBS=-L<DIR> (ex.\
 LIBS=-L/usr/path/lib) or a specific library using BLAS_LIBS=DIR/LIB\
 (for example BLAS_LIBS=/usr/path/lib/libcxml.a)]])])
fi
# at this point $sc_blas_ok is either of: yes disable
if test "$sc_blas_ok" = disable ; then
        AC_MSG_NOTICE([Not using BLAS])
fi
AM_CONDITIONAL([SC_BLAS], [test "$sc_blas_ok" = yes])

if test "$extraf77" = no ; then
AC_MSG_NOTICE([Checking LAPACK without F77 support first])
SC_LAPACK([SC], [$dgecon],
          [AC_DEFINE([LAPACK], 1, [Define to 1 if LAPACK is used])],
          [echo "Could not find LAPACK without F77 support"])
for CVAR in `set | awk -F= "/^ac_cv_.*${dgecon}/ { print \\$1 }"` ; do
  # echo "Unset $CVAR"
  unset $CVAR
done
# at this point $sc_lapack_ok is either of: yes no disable
if test "$sc_lapack_ok" = no ; then

extraf77=yes
AC_MSG_NOTICE([Calling F77 autoconf macros])
dnl For some strange reason these wrappers need to be toplevel.
dnl Otherwise they are executed even if the test above fails.
AC_PROG_F77([gfortran g77 fl32 f77 fort77 xlf f90 xlf90])
AC_F77_WRAPPERS
AC_F77_LIBRARY_LDFLAGS
dgecon=;AC_F77_FUNC(dgecon)
if test "$dgecon" = unknown ; then dgecon=dgecon_ ; fi
fi
else sc_lapack_ok=no
fi
if test "$sc_lapack_ok" = no ; then

AC_MSG_NOTICE([Checking LAPACK with proper F77 support])
SC_LAPACK([SC], [$dgecon],
          [AC_DEFINE([LAPACK], 1, [Define to 1 if LAPACK is used])],
          [AC_MSG_ERROR([[\
Cannot find LAPACK library, specify a path using LIBS=-L<DIR> (ex.\
 LIBS=-L/usr/path/lib) or a specific library using LAPACK_LIBS=DIR/LIB\
 (for example LAPACK_LIBS=/usr/path/lib/libcxml.a)]])])
fi
# at this point $sc_lapack_ok is either of: yes disable
if test "$sc_lapack_ok" = disable ; then
        AC_MSG_NOTICE([Not using LAPACK])
fi
AM_CONDITIONAL([SC_LAPACK], [test "$sc_lapack_ok" = yes])
])

# Check for builtin third-party software.
echo "o---------------------------------------"
echo "| Checking third-party software"
echo "o---------------------------------------"

SC_CUDA([SC])
SC_BUILTIN_ALL

# Checks for typedefs, structures, and compiler characteristics.
echo "o---------------------------------------"
echo "| Checking keywords and types"
echo "o---------------------------------------"

AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT
AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([unsigned long])
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

AC_DEFINE_UNQUOTED(CC,          ["${CC}"],          [C compiler])
AC_DEFINE_UNQUOTED(C_VERSION,   ["${C_VERSION}"],   [C compiler version])
AC_DEFINE_UNQUOTED(CFLAGS,      ["${CFLAGS}"],      [C compiler flags])
AC_DEFINE_UNQUOTED(CPP,         ["${CPP}"],         [C preprocessor])
AC_DEFINE_UNQUOTED(CPPFLAGS,    ["${CPPFLAGS}"],    [C preprocessor flags])
AC_DEFINE_UNQUOTED(F77,         ["${F77}"],         [F77 compiler])
AC_DEFINE_UNQUOTED(FFLAGS,      ["${FFLAGS}"],      [F77 compiler flags])
AC_DEFINE_UNQUOTED(LDFLAGS,     ["${LDFLAGS}"],     [Linker flags])
AC_DEFINE_UNQUOTED(BLAS_LIBS,   ["${BLAS_LIBS}"],   [BLAS libraries])
AC_DEFINE_UNQUOTED(LAPACK_LIBS, ["${LAPACK_LIBS}"], [LAPACK libraries])
AC_DEFINE_UNQUOTED(LIBS,        ["${LIBS}"],        [Libraries])
AC_DEFINE_UNQUOTED(FLIBS,       ["${FLIBS}"],       [Fortran libraries])

echo "o----------------------------------"
echo "| Results for libsc are"
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
AC_CONFIG_FILES([Makefile Makefile.sc.pre])
AC_OUTPUT

# Final notifications.
if test x$SC_HAVE_ZLIB = x; then
AC_MSG_NOTICE([
----------------------------------------------------------------------
We did not find a recent zlib containing the function adler32_combine.
Calling any functions using zlib functionality will abort the program.
You can fix this by compiling a working zlib and pointing LIBS to it.
])
fi
if test x$SC_HAVE_LUA = x; then
AC_MSG_NOTICE([
---------------------------------------------------------------------
We did not find a recent lua containing the function lua_createtable.
Including sc_lua.h will abort the compilation.
You can fix this by compiling a working lua and pointing LIBS to it.
])
fi
