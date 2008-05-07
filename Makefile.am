
## Makefile.am in sc

ACLOCAL_AMFLAGS = -I config

EXTRA_DIST = bootstrap scindent

MAINTAINERCLEANFILES = ChangeLog

# initialize empty variables
lib_LIBRARIES =
include_HEADERS =
noinst_PROGRAMS =
AM_CPPFLAGS =
LDADD =

# non-recursive build
include src/Makefile.am
include iniparser/Makefile.am
include example/options/Makefile.am

# non-recursive splint
SPLINT_FLAGS = \
	-weak -badflag -preproc -posix-lib \
	-unrecogcomments -formatcode
SPLINT_INCLUDES = \
	-I@top_builddir@/src -I@top_srcdir@/src -I@top_srcdir@/iniparser \
	@MPI_INCLUDES@
splint:
if HAVE_SPLINT
	for CFILE in @top_srcdir@/src/*.c @top_srcdir@/example/**/*.c ; do \
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
