
dnl This macro attempts to insert cuda rules into the Makefiles.
dnl This happens only when --with-nvcc[=NAME] is used on the configure line.
dnl
dnl SC_CUDA(PREFIX)
dnl Figure out if cuda should be used.
dnl Defines automake conditional SC_CUDA and activates Makefile rule.
dnl
AC_DEFUN([SC_CUDA],
[
# check --with-nvcc[=NAME] command line option
SC_ARG_WITH_PREFIX([nvcc], [enable CUDA and specify compiler (default: nvcc)],
                   [CUDA], [$1], [[[=NAME]]])
if test "$withval" != "no" ; then
  # determine name of CUDA compiler in variable PREFIX_NVCC_NAME
  $1_NVCC_NAME="nvcc"
  if test "$withval" != "yes" ; then
    $1_NVCC_NAME="$withval"
  fi

  # find location of CUDA compiler in PATH
  AC_PATH_PROG([$1_NVCC], [nvcc], [no])
  if test "$$1_NVCC" = "no" ; then
    AC_MSG_ERROR([CUDA compiler $1_NVCC_NAME not found])
  fi

  # cuda does not yet work with libtool
  if test -n "$$1_ENABLE_SHARED" -a "$$1_ENABLE_SHARED" != "no" ; then
    AC_MSG_ERROR([--with-nvcc does not yet work with --enable-shared])
  fi

  # print some variables
  AC_MSG_NOTICE([$1_NVCC_NAME is $$1_NVCC_NAME])
  AC_MSG_NOTICE([$1_NVCC is $$1_NVCC])
  AC_MSG_NOTICE([NVCCFLAGS is $NVCCFLAGS])
  AC_MSG_NOTICE([NVCCLIBS is $NVCCLIBS])
fi

AC_SUBST([$1_NVCC])
AC_SUBST([NVCCFLAGS])
AC_SUBST([NVCCLIBS])
])
