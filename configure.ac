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

echo "o---------------------------------------"
echo "| Checking MPI and related programs"
echo "o---------------------------------------"

dnl A nonempty second/third argument causes to enable F77/CXX, respectively.
SC_MPI_CONFIG([SC], [yes], [yes])
SC_MPI_ENGAGE([SC])
# This is needed for compatibility with automake >= 1.12
m4_ifdef([AM_PROG_AR],[AM_PROG_AR])
SC_PROG_LINT
dnl SC_C_VERSION
LT_INIT

echo "o---------------------------------------"
echo "| Checking libraries"
echo "o---------------------------------------"

SC_REQUIRE_LIB([m], [fabs])
SC_CHECK_LIB([z], [adler32_combine], [ZLIB], [SC])
SC_CHECK_LIB([lua52 lua5.2 lua51 lua5.1 lua lua5], [lua_createtable],
	     [LUA], [SC])

echo "o---------------------------------------"
echo "| Checking headers"
echo "o---------------------------------------"

AC_CHECK_HEADERS([execinfo.h signal.h sys/time.h sys/types.h time.h])
AC_CHECK_HEADERS([lua.h lua5.1/lua.h lua5.2/lua.h])

echo "o---------------------------------------"
echo "| Checking functions"
echo "o---------------------------------------"

AC_CHECK_FUNCS([backtrace backtrace_symbols])

echo "o---------------------------------------"
echo "| Checking BLAS, LAPACK and F77 support"
echo "o---------------------------------------"

SC_CHECK_BLAS_LAPACK([SC])

echo "o---------------------------------------"
echo "| Checking third-party software"
echo "o---------------------------------------"

dnl SC_CUDA([SC])
SC_BUILTIN_ALL

echo "o---------------------------------------"
echo "| Checking keywords and types"
echo "o---------------------------------------"

AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT
AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([long long])
AC_CHECK_SIZEOF([unsigned long])
AC_CHECK_SIZEOF([unsigned long long])
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
echo "| F77:         $F77"
echo "| FFLAGS:      $FFLAGS"
echo "| CPP:         $CPP"
echo "| CPPFLAGS:    $CPPFLAGS"
echo "| CC:          $CC"
dnl echo "| C_VERSION:   $C_VERSION"
echo "| CFLAGS:      $CFLAGS"
echo "| CXX:         $CXX"
echo "| CXXFLAGS:    $CXXFLAGS"
echo "| LDFLAGS:     $LDFLAGS"
echo "| LIBS:        $LIBS"
echo "| FLIBS:       $FLIBS"
echo "| BLAS_LIBS:   $BLAS_LIBS"
echo "| LAPACK_LIBS: $LAPACK_LIBS"
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
