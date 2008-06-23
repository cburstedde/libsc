
# This file is part of the SC Library
# Makefile.am in toplevel directory

ACLOCAL_AMFLAGS = -I config

# initialize empty variables
AM_CPPFLAGS =
DISTCLEANFILES =
EXTRA_DIST =
LDADD =
LINT_CSOURCES =
MAINTAINERCLEANFILES =
TESTS =
bin_PROGRAMS =
check_PROGRAMS =
include_HEADERS =
lib_LIBRARIES =
nodist_include_HEADERS =
noinst_HEADERS =
noinst_PROGRAMS =

# handle toplevel directory
EXTRA_DIST += bootstrap scindent
DISTCLEANFILES += _configs.sed src/sc_config.h
MAINTAINERCLEANFILES += ChangeLog
.PHONY: lint ChangeLog

# non-recursive build
include src/Makefile.am
include iniparser/Makefile.am
include libb64/Makefile.am
include zlib/Makefile.am
include test/Makefile.am
include example/logging/Makefile.am
include example/dmatrix/Makefile.am
include example/options/Makefile.am

if SC_BLAS
if SC_LAPACK
LDADD += @LAPACK_LIBS@
endif
LDADD += @BLAS_LIBS@ @FLIBS@
endif

ALL_LINT_FLAGS = $(LINT_FLAGS) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
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
	@(GIT_DIR=@top_srcdir@/.git git-log | @top_srcdir@/git2cl \
		> .ChangeLog.tmp && \
		mv .ChangeLog.tmp ChangeLog; rm -f .ChangeLog.tmp) || \
	(touch ChangeLog; \
		echo 'Warning not in git dir; ChangeLog could be wrong.' >&2)

dist-hook: ChangeLog
