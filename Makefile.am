
# This file is part of the SC Library
# Makefile.am in toplevel directory

ACLOCAL_AMFLAGS = -I config

# initialize empty variables
AM_CPPFLAGS =
BUILT_SOURCES =
CLEANFILES =
DISTCLEANFILES =
EXTRA_DIST =
LDADD =
LINT_CSOURCES =
TESTS =
bin_PROGRAMS =
check_PROGRAMS =
include_HEADERS =
lib_LTLIBRARIES =
nodist_include_HEADERS =
noinst_HEADERS =
noinst_PROGRAMS =
sysconf_DATA =

# pkg-config configuration file
pkgconfig_DATA = config/sc_autotools.pc

# use this if you want to link in libsc without autotools
sysconf_DATA += Makefile.sc.mk
CLEANFILES += Makefile.sc.mk
Makefile.sc.mk : Makefile.sc.pre
	cat $< | \
        sed -e 's,{\(.*prefix\)},{sc_\1},g' \
            -e 's,^\(.*prefix *=\),sc_\1,g' > $@

# install libsc m4 macros in a dedicated directory
scaclocaldir = $(datadir)/aclocal
dist_scaclocal_DATA = \
        config/sc_blas.m4 config/sc_builtin.m4 config/sc_c_check_flag.m4 \
        config/sc_c_version.m4 config/sc_cuda.m4 config/sc_include.m4 \
        config/sc_lapack.m4 config/sc_lint.m4 config/sc_mpi.m4 \
        config/ax_prefix_config_h.m4 config/ax_split_version.m4 \
        config/sc_package.m4 config/sc_trilinos.m4 \
        config/sc_pthread.m4 config/sc_openmp.m4 config/sc_memalign.m4

# install example .ini files in a dedicated directory
scinidir = $(datadir)/ini
dist_scini_DATA =

# handle toplevel directory
EXTRA_DIST += \
        bootstrap scindent build-aux/git-version-gen build-aux/git2cl bugs doc
DISTCLEANFILES += \
        _configs.sed src/sc_config.h config/sc_autotools.pc
.PHONY: lint ChangeLog

# setup test environment
if SC_MPIRUN
LOG_COMPILER = @SC_MPIRUN@
AM_LOG_FLAGS = @SC_MPI_TEST_FLAGS@
endif

# non-recursive build
include src/Makefile.am
include iniparser/Makefile.am
include libb64/Makefile.am
include test/Makefile.am
include example/bspline/Makefile.am
## include example/cuda/Makefile.am
include example/dmatrix/Makefile.am
include example/function/Makefile.am
include example/logging/Makefile.am
include example/options/Makefile.am
include example/pthread/Makefile.am
include example/openmp/Makefile.am
include example/v4l2/Makefile.am
include example/warp/Makefile.am
include example/testing/Makefile.am

# lint static syntax checker
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

# revision control and ChangeLog
ChangeLog:
	(GIT_DIR=@top_srcdir@/.git git log > .ChangeLog.tmp && \
         cat .ChangeLog.tmp | @top_srcdir@/build-aux/git2cl > ChangeLog) ; \
        rm -f .ChangeLog.tmp

dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version
	test "x$(VERSION)" = "x`@top_srcdir@/build-aux/git-version-gen\
              $(distdir)/.tarball-version`" || \
        ((echo "Stale version;" ; echo $(VERSION); \
          echo `@top_srcdir@/build-aux/git-version-gen\
              $(distdir)/.tarball-version`; echo "Please run:" ; \
          echo "     cd @top_srcdir@ && ./bootstrap" ; \
          echo "before make dist") 1>&2 ; rm -r $(distdir) ; exit 1)

install-data-hook:
	cd $(DESTDIR)$(pkgconfigdir) && \
              mv -f sc_autotools.pc "libsc-$(VERSION).pc"

uninstall-hook:
	cd $(DESTDIR)$(pkgconfigdir) && rm -f "libsc-$(VERSION).pc"

clean-local:
	rm -f ChangeLog

maintainer-clean-local:
	rm -rf doxygen

doxygen: Doxyfile
	doxygen

.PHONY: doxygen

## # CUDA support
## if SC_WITH_CUDA
## .cu.o:
## 	$(SC_NVCC) -o $@ -c $< @NVCCFLAGS@
## endif
