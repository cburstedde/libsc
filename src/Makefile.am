
# Makefile.am in sc/src
# included non-recursively from toplevel directory

libsc_installed_headers = src/sc_config.h \
	src/sc.h src/sc_memory.h src/sc_options.h
libsc_compiled_sources  = \
	src/sc.c src/sc_memory.c src/sc_options.c

lib_LIBRARIES += src/libsc.a
src_libsc_a_SOURCES = $(libsc_installed_headers) $(libsc_compiled_sources)
src_libsc_a_CPPFLAGS = $(AM_CPPFLAGS)

include_HEADERS += $(libsc_installed_headers)

AM_CPPFLAGS += -I@top_srcdir@/src

LDADD += @top_builddir@/src/libsc.a

if LINT
LINT_CSOURCES += $(src_libsc_a_SOURCES)
endif LINT
