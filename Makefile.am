
# This file is part of the SC Library
# Makefile.am in toplevel directory

ACLOCAL_AMFLAGS = -I config

# initialize empty variables
AM_CPPFLAGS =
AM_LDFLAGS =
BUILT_SOURCES =
CLEANFILES =
DISTCLEANFILES =
EXTRA_DIST =
LDADD =
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
sysconf_DATA += config/Makefile.sc.mk
CLEANFILES += config/Makefile.sc.mk
config/Makefile.sc.mk : config/Makefile.sc.pre
	cat $< | \
        sed -e 's,{\(.*prefix\)},{sc_\1},g' \
            -e 's,^\(.*prefix *=\),sc_\1,g' > $@

# install libsc m4 macros in a dedicated directory
scaclocaldir = $(datadir)/aclocal
dist_scaclocal_DATA = \
        config/sc_builtin.m4 config/sc_memalign.m4 config/sc_include.m4 \
        config/ax_prefix_config_h.m4 config/ax_split_version.m4 \
        config/sc_package.m4 config/sc_mpi.m4 \
        config/sc_pthread.m4 config/sc_openmp.m4 config/sc_v4l2.m4 \
        config/sc_qsort.m4

# install example .ini files in a dedicated directory
scinidir = $(datadir)/ini
dist_scini_DATA =

# handle toplevel directory
EXTRA_DIST += \
        bootstrap scindent build-aux/git-version-gen build-aux/git2cl doc \
        cmake CMakeLists.txt src/CMakeLists.txt \
        example/CMakeLists.txt test/CMakeLists.txt CMakePresets.json \
        example/linksc/README example/linksc/bootstrap \
        example/linksc/configure.ac example/linksc/Makefile.am \
        example/linksc/liblinksc.h example/linksc/liblinksc.c \
        example/linksc/linksc.c

DISTCLEANFILES += \
        _configs.sed config/sc_config.h config/sc_autotools.pc

.PHONY: ChangeLog

# setup test environment
LOG_COMPILER = @SC_MPIRUN@ @SC_MPI_TEST_FLAGS@ \
               @SC_VALGRIND@ @SC_VALGRIND_FLAGS@

# non-recursive build
include src/Makefile.am
include iniparser/Makefile.am
include libb64/Makefile.am
include test/Makefile.am
## include example/bspline/Makefile.am
## include example/cuda/Makefile.am
## include example/dmatrix/Makefile.am
include example/function/Makefile.am
include example/logging/Makefile.am
include example/options/Makefile.am
include example/pthread/Makefile.am
## include example/openmp/Makefile.am
include example/v4l2/Makefile.am
## include example/warp/Makefile.am
include example/testing/Makefile.am

# revision control and ChangeLog
ChangeLog:
	(GIT_DIR=@top_srcdir@/.git git log > .ChangeLog.tmp && \
         cat .ChangeLog.tmp | @top_srcdir@/build-aux/git2cl > ChangeLog) ; \
        rm -f .ChangeLog.tmp

dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version
	(GITGEN_VERSION=`(cd @top_srcdir@ && ./build-aux/git-version-gen\
                .tarball-version)` ; \
	 test "x$(VERSION)" = "x$${GITGEN_VERSION}" || \
         ((echo "Stale version;"; echo $(VERSION); echo "$${GITGEN_VERSION}"; \
           echo "Please run:" ; \
           echo "     (cd @top_srcdir@ && ./bootstrap)" ; \
           echo "before make dist") 1>&2 ; rm -r $(distdir) ; exit 1))

install-data-hook:
	cd $(DESTDIR)$(pkgconfigdir) && \
              mv -f sc_autotools.pc "libsc-$(VERSION).pc"
	cd $(DESTDIR)$(pkgconfigdir) && ln -sf "libsc-$(VERSION).pc" libsc.pc

uninstall-hook:
	cd $(DESTDIR)$(pkgconfigdir) && rm -f "libsc-$(VERSION).pc"
	cd $(DESTDIR)$(pkgconfigdir) && rm -f "libsc.pc"

## From the GNU automake documentation "What gets Cleaned"
## -------------------------------------------------------

## If make built it, and it is commonly something that one would want to rebuild
## (for instance, a .o file), then mostlyclean should delete it.
## Otherwise, if make built it, then clean should delete it.
clean-local:
	rm -f ChangeLog sc_test_io_file.*

## If configure built it, then distclean should delete it.
distclean-local:

## If the maintainer built it (for instance, a .info file), then
## maintainer-clean should delete it. However maintainer-clean should not delete
## anything that needs to exist in order to run ‘./configure && make’.
maintainer-clean-local:
	rm -rf doc/{html,latex,man,*.doxygen.tags}

doxygen-local: doc/Doxyfile
	cd doc && doxygen Doxyfile

.PHONY: doxygen
