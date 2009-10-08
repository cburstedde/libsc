
# This file is part of the SC Library
# Makefile.am in src
# included non-recursively from toplevel directory

libsc_generated_headers = src/sc_config.h
libsc_installed_headers = \
        src/sc.h src/sc_mpi.h src/sc_containers.h src/sc_avl.h \
        src/sc_options.h src/sc_functions.h src/sc_statistics.h \
        src/sc_ranges.h src/sc_c99_functions.h src/sc_io.h \
        src/sc_amr.h src/sc_allgather.h src/sc_sort.h \
        src/sc_dmatrix.h src/sc_blas.h src/sc_lapack.h \
        src/sc_bspline.h src/sc_flops.h src/sc_object.h \
        src/sc_getopt.h src/sc_obstack.h src/sc_zlib.h \
        src/sc_lua.h src/sc_complex.h \
	src/sc_keyvalue.h
libsc_compiled_sources = \
        src/sc.c src/sc_mpi.c src/sc_containers.c src/sc_avl.c \
        src/sc_options.c src/sc_functions.c src/sc_statistics.c \
        src/sc_ranges.c src/sc_c99_functions.c src/sc_io.c \
        src/sc_amr.c src/sc_allgather.c src/sc_sort.c \
        src/sc_dmatrix.c src/sc_blas.c src/sc_lapack.c \
        src/sc_bspline.c src/sc_flops.c src/sc_object.c \
        src/sc_getopt.c src/sc_obstack.c src/sc_getopt1.c \
	src/sc_keyvalue.c
libsc_original_headers = \
        src/sc_builtin/getopt.h src/sc_builtin/getopt_int.h \
        src/sc_builtin/obstack.h \
        src/sc_builtin/lua.h src/sc_builtin/luaconf.h \
        src/sc_builtin/lauxlib.h src/sc_builtin/lualib.h \
        src/sc_builtin/zlib.h src/sc_builtin/zconf.h

lib_LIBRARIES += src/libsc.a
src_libsc_a_SOURCES = \
        $(libsc_installed_headers) \
        $(libsc_compiled_sources)
src_libsc_a_CPPFLAGS = $(AM_CPPFLAGS)

nodist_include_HEADERS += $(libsc_generated_headers)
include_HEADERS += $(libsc_installed_headers)

scbuiltindir = $(includedir)/sc_builtin
scbuiltin_HEADERS = $(libsc_original_headers)

AM_CPPFLAGS += -I@top_srcdir@/src

LDADD += @top_builddir@/src/libsc.a

LINT_CSOURCES += $(libsc_compiled_sources)
