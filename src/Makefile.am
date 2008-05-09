
# Makefile.am in sc/src
# included non-recursively from toplevel directory

libsc_internal_headers =
libsc_generated_headers = src/sc_config.h
libsc_installed_headers = src/sc.h src/sc_containers.h src/sc_options.h
libsc_compiled_sources =  src/sc.c src/sc_containers.c src/sc_options.c
if SC_BLAS
  libsc_installed_headers += src/sc_blas.h src/sc_dmatrix.h
  libsc_compiled_sources +=  src/sc_blas.c src/sc_dmatrix.c
endif
if SC_GETOPT
  libsc_internal_headers +=  src/sc_getopt_int.h
  libsc_installed_headers += src/sc_getopt.h
  libsc_compiled_sources +=  src/sc_getopt.c
endif
if SC_OBSTACK
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
