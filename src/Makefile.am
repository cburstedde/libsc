
# Makefile.am in sc/src
# included non-recursively from toplevel directory

libsc_installed_headers = src/sc.h src/sc_memory.h
libsc_compiled_sources  = src/sc.c src/sc_memory.c

lib_LIBRARIES = src/libsc.a

src_libsc_a_SOURCES =  $(libsc_installed_headers) $(libsc_compiled_sources)

include_HEADERS = $(libsc_installed_headers)

AM_CPPFLAGS += -I@top_srcdir@/src
