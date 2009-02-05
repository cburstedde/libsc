
# This file is part of the SC Library
# Makefile.am in src
# included non-recursively from toplevel directory

EXTRA_DIST += \
        src/sc_builtin

libsc_internal_headers =
libsc_generated_headers = src/sc_config.h
libsc_installed_headers = \
        src/sc.h src/sc_mpi.h src/sc_containers.h src/sc_avl.h \
        src/sc_options.h src/sc_functions.h src/sc_statistics.h \
        src/sc_ranges.h src/sc_c99_functions.h src/sc_io.h \
        src/sc_amr.h src/sc_allgather.h src/sc_sort.h \
        src/sc_dmatrix.h src/sc_blas.h src/sc_lapack.h \
        src/sc_bspline.h src/sc_zlib.h src/sc_lua.h
libsc_compiled_sources = \
        src/sc.c src/sc_mpi.c src/sc_containers.c src/sc_avl.c \
        src/sc_options.c src/sc_functions.c src/sc_statistics.c \
        src/sc_ranges.c src/sc_c99_functions.c src/sc_io.c \
        src/sc_amr.c src/sc_allgather.c src/sc_sort.c \
        src/sc_dmatrix.c src/sc_blas.c src/sc_lapack.c \
        src/sc_bspline.c

if SC_PROVIDE_GETOPT
  libsc_internal_headers +=  src/sc_getopt_int.h
  libsc_installed_headers += src/sc_getopt.h
  libsc_compiled_sources +=  src/sc_getopt.c src/sc_getopt1.c
endif
if SC_PROVIDE_OBSTACK
  libsc_installed_headers += src/sc_obstack.h
  libsc_compiled_sources +=  src/sc_obstack.c
endif

lib_LIBRARIES += src/libsc.a
src_libsc_a_SOURCES = \
        $(libsc_internal_headers) \
        $(libsc_installed_headers) \
        $(libsc_compiled_sources)
src_libsc_a_CPPFLAGS = $(AM_CPPFLAGS)

nodist_include_HEADERS += $(libsc_generated_headers)
include_HEADERS += $(libsc_installed_headers)

AM_CPPFLAGS += -I@top_srcdir@/src

LDADD += @top_builddir@/src/libsc.a

LINT_CSOURCES += $(libsc_compiled_sources)

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(includedir)/sc_builtin
	$(INSTALL_DATA) @top_srcdir@/src/sc_builtin/*.h \
                        $(DESTDIR)$(includedir)/sc_builtin

uninstall-local:
	rm -rf $(DESTDIR)$(includedir)/sc_builtin
