
## Makefile.am in sc

ACLOCAL_AMFLAGS = -I config -I gnulib/m4

SUBDIRS = gnulib src

EXTRA_DIST = bootstrap scgnulib scindent ChangeLog

MAINTAINERCLEANFILES=ChangeLog

.PHONY: ChangeLog

ChangeLog:
	(GIT_DIR=$(top_srcdir)/.git git-log > .ChangeLog.tmp && \
		mv .ChangeLog.tmp ChangeLog; rm -f .ChangeLog.tmp) || \
	(touch ChangeLog; \
		echo 'Warning not in git dir; ChangeLog could be wrong.' >&2)

dist-hook: ChangeLog

if HAVE_SPLINT # splint is used non-recursively

SPLINT_FLAGS = \
	-weak -badflag -preproc -posix-lib \
	-unrecogcomments -formatcode
SPLINT_INCLUDES = \
	-I@top_builddir@/src -I@top_srcdir@/src @MPI_INCLUDES@

splint:
	for CFILE in @top_srcdir@/src/*.c ; do \
	  echo "Checking $$CFILE" ; \
	  splint -DSC_SPLINT $(SPLINT_FLAGS) $(SPLINT_INCLUDES) \
	     "$$CFILE" || exit 1 ; \
	done

.PHONY: splint

endif # HAVE_SPLINT
