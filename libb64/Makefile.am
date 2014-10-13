
# This file is part of the SC Library
# Makefile.am in libb64
# included non-recursively from toplevel directory

bin_PROGRAMS += libb64/sc_b64enc libb64/sc_b64dec
libb64_sc_b64enc_SOURCES = libb64/b64enc.c
libb64_sc_b64enc_CPPFLAGS = $(AM_CPPFLAGS) -I@top_srcdir@/libb64
libb64_sc_b64dec_SOURCES = libb64/b64dec.c
libb64_sc_b64dec_CPPFLAGS = $(AM_CPPFLAGS) -I@top_srcdir@/libb64

libb64_internal_headers = libb64/libb64.h
libb64_compiled_sources = libb64/cencode.c libb64/cdecode.c

libsc_internal_headers += $(libb64_internal_headers)
libsc_compiled_sources += $(libb64_compiled_sources)
LIBSC_CPPFLAGS += -I@top_srcdir@/libb64

EXTRA_DIST += \
	libb64/AUTHORS libb64/CHANGELOG libb64/LICENSE libb64/README

LINT_CSOURCES += \
	$(libb64_sc_b64enc_SOURCES) \
	$(libb64_sc_b64dec_SOURCES)
