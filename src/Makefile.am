
# This file is part of the SC Library
# Makefile.am in src
# included non-recursively from toplevel directory

# append to these variables in subdirectories as necessary
libsc_generated_headers = src/sc_config.h
libsc_installed_headers = \
        src/sc.h src/sc_mpi.h src/sc_containers.h src/sc_avl.h \
        src/sc_string.h src/sc_unique_counter.h src/sc_private.h \
        src/sc_options.h src/sc_functions.h src/sc_statistics.h \
        src/sc_ranges.h src/sc_io.h \
        src/sc_amr.h src/sc_search.h src/sc_sort.h \
        src/sc_dmatrix.h src/sc_blas.h src/sc_lapack.h \
        src/sc_bspline.h src/sc_flops.h src/sc_random.h \
        src/sc_getopt.h src/sc_obstack.h src/sc_lua.h src/sc_polynom.h \
        src/sc_keyvalue.h src/sc_refcount.h src/sc_warp.h src/sc_shmem.h \
        src/sc_allgather.h src/sc_reduce.h src/sc_notify.h \
        src/sc_uint128.h src/sc_v4l2.h
libsc_internal_headers =
libsc_compiled_sources = \
        src/sc.c src/sc_mpi.c src/sc_containers.c src/sc_avl.c \
        src/sc_string.c src/sc_unique_counter.c \
        src/sc_options.c src/sc_functions.c src/sc_statistics.c \
        src/sc_ranges.c src/sc_io.c \
        src/sc_amr.c src/sc_search.c src/sc_sort.c \
        src/sc_dmatrix.c src/sc_blas.c src/sc_lapack.c \
        src/sc_bspline.c src/sc_flops.c src/sc_random.c \
        src/sc_getopt.c src/sc_obstack.c src/sc_getopt1.c \
        src/sc_keyvalue.c src/sc_refcount.c src/sc_warp.c src/sc_polynom.c \
        src/sc_shmem.c src/sc_allgather.c src/sc_reduce.c src/sc_notify.c \
        src/sc_uint128.c src/sc_v4l2.c
libsc_original_headers = \
        src/sc_builtin/getopt.h src/sc_builtin/getopt_int.h \
        src/sc_builtin/obstack.h

# this variable is used for headers that are not publicly installed
LIBSC_CPPFLAGS =

lib_LTLIBRARIES += src/libsc.la
src_libsc_la_SOURCES = \
        $(libsc_internal_headers) \
        $(libsc_compiled_sources)
src_libsc_la_CPPFLAGS = $(AM_CPPFLAGS) $(LIBSC_CPPFLAGS)
src_libsc_la_LDFLAGS = -release $(VERSION)
LDADD += @top_builddir@/src/libsc.la

nodist_include_HEADERS += $(libsc_generated_headers)
include_HEADERS += $(libsc_installed_headers)

scbuiltindir = $(includedir)/sc_builtin
scbuiltin_HEADERS = $(libsc_original_headers)

AM_CPPFLAGS += -I@top_srcdir@/src

LINT_CSOURCES += $(libsc_compiled_sources)
