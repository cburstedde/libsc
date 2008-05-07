
## Makefile.am in sc

ACLOCAL_AMFLAGS = -I config -I gnulib/m4

EXTRA_DIST = bootstrap scgnulib scindent

MAINTAINERCLEANFILES = ChangeLog

# gnulib is built recursively
SUBDIRS = gnulib
AM_CPPFLAGS = -I@top_builddir@/gnulib -I@top_srcdir@/gnulib

# non-recursive build
include src/Makefile.am

# non-recursively splint
SPLINT_FLAGS = \
	-weak -badflag -preproc -posix-lib \
	-unrecogcomments -formatcode
SPLINT_INCLUDES = \
	-I@top_builddir@/src -I@top_srcdir@/src @MPI_INCLUDES@
splint:
if HAVE_SPLINT
	for CFILE in @top_srcdir@/src/*.c ; do \
	  echo "Checking $$CFILE" ; \
	  splint -DSC_SPLINT $(SPLINT_FLAGS) $(SPLINT_INCLUDES) \
	     "$$CFILE" || exit 1 ; \
	done
endif # HAVE_SPLINT

.PHONY: splint

ChangeLog:
	(GIT_DIR=@top_srcdir@/.git git-log | @top_srcdir@/git2cl \
		> .ChangeLog.tmp && \
		mv .ChangeLog.tmp ChangeLog; rm -f .ChangeLog.tmp) || \
	(touch ChangeLog; \
		echo 'Warning not in git dir; ChangeLog could be wrong.' >&2)

dist-hook: ChangeLog
