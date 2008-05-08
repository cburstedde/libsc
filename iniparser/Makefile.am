
# Makefile.am in sc/iniparser
# included non-recursively from toplevel directory

noinst_PROGRAMS += iniparser/sc_iniexample
iniparser_sc_iniexample_SOURCES = iniparser/iniexample.c
iniparser_sc_iniexample_CPPFLAGS = $(AM_CPPFLAGS) -I@top_srcdir@/iniparser

iniparser_internal_headers = iniparser/dictionary.h iniparser/iniparser.h
iniparser_compiled_sources = iniparser/dictionary.c iniparser/iniparser.c
src_libsc_a_SOURCES += \
	$(iniparser_internal_headers) $(iniparser_compiled_sources)
src_libsc_a_CPPFLAGS += -I@top_srcdir@/iniparser

EXTRA_DIST += iniparser/AUTHORS iniparser/LICENSE iniparser/twisted.ini

if LINT
LINT_CSOURCES += \
	$(iniparser_compiled_sources) \
	$(iniparser_sc_iniexample_SOURCES)
endif LINT
