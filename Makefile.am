
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
LINT_CSOURCES =

# non-recursive build
include src/Makefile.am
include iniparser/Makefile.am
include example/options/Makefile.am

if LINT
ALL_LINT_FLAGS=$(LINT_FLAGS) -systemdirs /usr/include:$(MPI_INCLUDE_PATH) \
               $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(MPI_INCLUDES) \
               $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(src_libsc_a_CPPFLAGS)

lint:
	for f in $(LINT_CSOURCES) ; do \
		$(LINT) $(ALL_LINT_FLAGS) $(top_srcdir)/$$f ; \
	done
endif LINT

ChangeLog:
	(GIT_DIR=@top_srcdir@/.git git-log | @top_srcdir@/git2cl \
		> .ChangeLog.tmp && \
		mv .ChangeLog.tmp ChangeLog; rm -f .ChangeLog.tmp) || \
	(touch ChangeLog; \
		echo 'Warning not in git dir; ChangeLog could be wrong.' >&2)

dist-hook: ChangeLog
