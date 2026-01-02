dnl
dnl This file is part of the SC Library.
dnl

AC_INIT([libsc],
        [m4_esyscmd([build-aux/git-version-gen .tarball-version])],
        [p4est@ins.uni-bonn.de])
AC_PREREQ(2.61)
AC_CONFIG_HEADERS([config/sc_config_autotools.h])
AC_CONFIG_SRCDIR([src/sc.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([config])
AC_PREFIX_DEFAULT([$PWD/local])
AX_PREFIX_CONFIG_H([config/sc_config.h], [SC])
AM_INIT_AUTOMAKE([parallel-tests subdir-objects])
AM_SILENT_RULES([yes])
AM_EXTRA_RECURSIVE_TARGETS([justlibs doxygen])
PKG_INSTALLDIR
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
AC_SUBST([SC_ENABLE_DEBUG])
SC_ARG_DISABLE([realloc], [resize arrays with malloc/copy/free (HISTORIC)],
               [USE_REALLOC])
SC_ARG_DISABLE([counters], [disable non-thread-safe internal debug counters],
               [USE_COUNTERS])
SC_ARG_DISABLE([file-checks], [disable tests that use file i/o functions],
               [FILE_CHECKS])
SC_ARG_WITH([papi], [enable Flop counting with papi], [PAPI])

echo "o---------------------------------------"
echo "| Checking MPI and related programs"
echo "o---------------------------------------"

dnl A nonempty second/third argument causes to enable F77+FC/CXX, respectively.
SC_MPI_CONFIG([SC], [], [])
SC_MPI_ENGAGE([SC])
# This is needed for compatibility with automake >= 1.12
m4_ifdef([AM_PROG_AR],[AM_PROG_AR])
LT_INIT

echo "o---------------------------------------"
echo "| Checking keywords and types"
echo "o---------------------------------------"

AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

echo "o---------------------------------------"
echo "| Checking headers"
echo "o---------------------------------------"

AC_CHECK_HEADERS([fcntl.h sys/ioctl.h sys/select.h sys/stat.h])
AC_CHECK_HEADERS([execinfo.h signal.h libgen.h time.h sys/time.h])
AC_CHECK_HEADERS([linux/version.h linux/videodev2.h])

echo "o---------------------------------------"
echo "| Checking functions"
echo "o---------------------------------------"

AC_CHECK_FUNCS([backtrace backtrace_symbols])
AC_CHECK_FUNCS([basename dirname])
AC_CHECK_FUNCS([strtol strtoll strtok_r])
AC_CHECK_FUNCS([fsync])
AC_CHECK_FUNCS([qsort_r])
AC_CHECK_FUNCS([gettimeofday])

echo "o---------------------------------------"
echo "| Checking libraries"
echo "o---------------------------------------"

dnl Some libsc example programs (e.g. the v4l2 example) use math functions
dnl such as sin().  On many systems these reside in libm, while on others
dnl they are provided by libc.  Detect and add the required math library
dnl explicitly to avoid relying on transitive or toolchain-specific linking.
AC_SEARCH_LIBS([sin], [m])

SC_CHECK_LIBRARIES([SC])

# Print summary.
echo "o----------------------------------"
echo "| Results for libsc are"
echo "o----------------------------------"
m4_ifset([SC_CHECK_MPI_F77], [
echo "| F77:         $F77"
echo "| FFLAGS:      $FFLAGS"
])
m4_ifset([SC_CHECK_MPI_FC], [
echo "| FC:          $FC"
echo "| FCFLAGS:     $FCFLAGS"
])
echo "| CPP:         $CPP"
echo "| CPPFLAGS:    $CPPFLAGS"
echo "| CC:          $CC"
dnl echo "| C_VERSION:   $C_VERSION"
echo "| CFLAGS:      $CFLAGS"
m4_ifset([SC_CHECK_MPI_CXX], [
echo "| CXX:         $CXX"
echo "| CXXFLAGS:    $CXXFLAGS"
])
echo "| LDFLAGS:     $LDFLAGS"
echo "| LIBS:        $LIBS"
dnl echo "| FLIBS:       $FLIBS"
dnl echo "| BLAS_LIBS:   $BLAS_LIBS"
dnl echo "| LAPACK_LIBS: $LAPACK_LIBS"
echo "| TESTS:       $SC_MPIRUN $SC_MPI_TEST_FLAGS"
echo "o----------------------------------"

# Create output files.
AC_CONFIG_FILES([Makefile doc/Doxyfile])
AC_CONFIG_FILES([config/Makefile.sc.pre config/sc_autotools.pc])
AC_OUTPUT

# Final notifications.
SC_FINAL_MESSAGES([SC])
