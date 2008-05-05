
## Makefile.am in sc/src

## SUBDIRS =

lib_LIBRARIES = libsc.a

libsc_installed_headers = sc.h sc_memory.h
libsc_compiled_sources  = sc.c sc_memory.c

libsc_a_SOURCES =  $(libsc_installed_headers) $(libsc_compiled_sources)

include_HEADERS = $(libsc_installed_headers)

AM_CPPFLAGS = -I@top_builddir@/gnulib -I@top_srcdir@/gnulib
