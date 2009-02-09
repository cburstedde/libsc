
# This file is part of the SC Library
# Makefile.am in toplevel directory

ACLOCAL_AMFLAGS = -I config

# initialize empty variables
AM_CPPFLAGS =
CLEANFILES =
DISTCLEANFILES =
EXTRA_DIST =
LDADD =
LINT_CSOURCES =
TESTS =
bin_PROGRAMS =
check_PROGRAMS =
include_HEADERS =
lib_LIBRARIES =
nodist_include_HEADERS =
noinst_HEADERS =
noinst_PROGRAMS =
sysconf_DATA =

# use this if you want to link in libsc without autotools
sysconf_DATA += Makefile.sc.mk
CLEANFILES += Makefile.sc.mk
Makefile.sc.mk : Makefile.sc.pre
	cat $< | \
        sed -e 's,{\(.*prefix\)},{sc_\1},g' \
            -e 's,^\(.*prefix *=\),sc_\1,g' > $@

# install libsc m4 macros in the correct directory
scaclocaldir = $(datadir)/aclocal
scaclocal_DATA = \
        config/sc_builtin.m4 config/sc_include.m4 config/sc_mpi.m4 \
        config/sc_c_check_flag.m4 config/sc_c_version.m4 \
        config/sc_prefix_config_h.m4 config/sc_lint.m4

# set test environment
TESTS_ENVIRONMENT = @SC_MPI_TESTS_ENVIRONMENT@

# handle toplevel directory
EXTRA_DIST += \
        bootstrap scindent config/git-version-gen config/git2cl bugs doc
DISTCLEANFILES += \
        _configs.sed src/sc_config.h SC.override.pre SC.override
.PHONY: lint ChangeLog

# non-recursive build
include src/Makefile.am
include iniparser/Makefile.am
include libb64/Makefile.am
include lua/Makefile.am
include zlib/Makefile.am
include test/Makefile.am
include example/bspline/Makefile.am
include example/dmatrix/Makefile.am
include example/logging/Makefile.am
include example/matrixmultiply/Makefile.am
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
else
	@echo "Static source code check disabled by configure"
endif

ChangeLog:
	(GIT_DIR=@top_srcdir@/.git git log > .ChangeLog.tmp && \
         cat .ChangeLog.tmp | @top_srcdir@/config/git2cl > ChangeLog) ; \
        rm -f .ChangeLog.tmp

dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version
	test "x$(VERSION)" = "x`@top_srcdir@/config/git-version-gen\
              @top_srcdir@/.tarball-version`" || \
        ((echo "Stale version;" ; echo "Please run:" ; \
          echo "     cd @top_srcdir@ && ./bootstrap" ; \
          echo "before make dist") 1>&2 ; exit 1)
