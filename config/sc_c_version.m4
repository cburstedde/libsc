# ===========================================================================
#            From: http://autoconf-archive.cryp.to/ax_gcc_version.html
# and renamed by Carsten Burstedde <carsten@ices.utexas.edu>
# ===========================================================================
#
# SYNOPSIS
#
#   SC_C_VERSION  (Extension of AX_GCC_VERSION to more C compilers)
#
# DESCRIPTION
#
#   This macro retrieves the cc version and returns it in the C_VERSION
#   variable if available, an empty string otherwise.
#
# LAST MODIFICATION
#
#   2009-02-09
#
# COPYLEFT
#
#   Copyright (c) 2008 Lucas Wilcox <lucasw@ices.utexas.edu>
#   Copyright (c) 2008 Francesco Salvestrini <salvestrini@users.sourceforge.net>
#
#   This program is free software; you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation; either version 2 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <http://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Macro Archive. When you make and
#   distribute a modified version of the Autoconf Macro, you may extend this
#   special exception to the GPL to apply to your modified version as well.

AC_DEFUN([SC_C_VERSION], [
  C_VERSION=""
  AS_IF([test "x$C_VERSION" = "x"],[
    SC_C_CHECK_FLAG([-V],[],[],[
      sc_pgcc_version_option=yes
    ],[
      sc_pgcc_version_option=no
    ])
    AS_IF([test "x$sc_pgcc_version_option" != "xno"],[
      AC_CACHE_CHECK([pgcc version],[sc_cv_pgcc_version],[
        # The sed part removes all new lines
        sc_cv_pgcc_version="`$CC -V 2>/dev/null | sed -e :a -e '$!N; s/\n/ /; ta'`"
        AS_IF([test "x$sc_cv_pgcc_version" = "x"],[
          sc_cv_pgcc_version=""
          ])
        ])
      C_VERSION=$sc_cv_pgcc_version
    ])
  ])

  AS_IF([test "x$C_VERSION" = "x"],[
    SC_C_CHECK_FLAG([-dumpversion],[],[],[
      sc_gcc_version_option=yes
    ],[
      sc_gcc_version_option=no
    ])
    AS_IF([test "x$sc_gcc_version_option" != "xno"],[
      AC_CACHE_CHECK([gcc version],[sc_cv_gcc_version],[
        # The sed part removes all new lines
        sc_cv_gcc_version="`$CC -dumpversion | sed -e :a -e '$!N; s/\n/ /; ta'`"
        AS_IF([test "x$sc_cv_gcc_version" = "x"],[
          sc_cv_gcc_version=""
        ])
      ])
      C_VERSION=$sc_cv_gcc_version
    ])
  ])

  AC_SUBST([C_VERSION])
])
