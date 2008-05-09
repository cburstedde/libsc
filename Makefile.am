
# Makefile.am in sc

ACLOCAL_AMFLAGS = -I config

EXTRA_DIST = bootstrap scindent

MAINTAINERCLEANFILES = ChangeLog

.PHONY: lint ChangeLog

# initialize empty variables
bin_PROGRAMS =
noinst_PROGRAMS =
lib_LIBRARIES =
noinst_HEADERS =
include_HEADERS =
nodist_include_HEADERS =
AM_CPPFLAGS =
LDADD =
LINT_CSOURCES =

# non-recursive build
include src/Makefile.am
include iniparser/Makefile.am
include libb64/Makefile.am
include zlib/Makefile.am
include example/dmatrix/Makefile.am
include example/options/Makefile.am

if SC_BLAS
LDADD += @LAPACK_LIBS@ @BLAS_LIBS@ @FLIBS@
endif

ALL_LINT_FLAGS=$(LINT_FLAGS) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
               $(MPI_INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) \
               $(src_libsc_a_CPPFLAGS)

lint:
if LINT
	for f in $(LINT_CSOURCES) ; do \
		$(LINT) $(ALL_LINT_FLAGS) $(top_srcdir)/$$f || \
		echo "Lint check failed for $$f" ; \
	done
endif LINT

ChangeLog:
	(GIT_DIR=@top_srcdir@/.git git-log | @top_srcdir@/git2cl \
		> .ChangeLog.tmp && \
		mv .ChangeLog.tmp ChangeLog; rm -f .ChangeLog.tmp) || \
	(touch ChangeLog; \
		echo 'Warning not in git dir; ChangeLog could be wrong.' >&2)

dist-hook: ChangeLog
