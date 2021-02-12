
dnl SC_CHECK_V4L2(PREFIX)
dnl Check for Video for Linux version 2 support and link a test program
dnl
dnl This macro tries to link various v4l2 access functions.
dnl If a working v4l2 development environment is found, define
dnl PREFIX_ENABLE_V4L2.  Set the automake conditional accordingly.
dnl
AC_DEFUN([SC_CHECK_V4L2], [
  AC_MSG_CHECKING([for V4L2])

  dnl Run link test for V4L2 device interaction
  AS_VAR_PUSHDEF([myresult], [ac_cv_link_v4l2])dnl
  AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#include <stdio.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_LINUX_VIDEODEV2_H
#include <linux/videodev2.h>
#endif
]],[[
  open (NULL, O_NONBLOCK | O_RDWR);
  ioctl (0, VIDIOC_QUERYCAP, NULL);
  ioctl (0, VIDIOC_ENUMOUTPUT, NULL);
  ioctl (0, VIDIOC_S_OUTPUT, NULL);
  ioctl (0, VIDIOC_S_FMT, NULL);
  select (0, NULL, NULL, NULL, NULL);
  write (0, NULL, 0);
]])], [AS_VAR_SET([myresult], [yes])], )

  dnl Act based on link test result
  AS_VAR_IF([myresult], [yes],
      [AC_DEFINE([ENABLE_V4L2], 1, [Development with V4L2 devices works])
       AC_MSG_RESULT([yes])],
      [AC_MSG_RESULT([no])])
  AM_CONDITIONAL([$1_ENABLE_V4L2],
                 [AS_VAR_TEST_SET([myresult])])
  AS_VAR_POPDEF([myresult])
])
