
# This file is part of the SC Library
# Makefile.am in iniparser
# included non-recursively from toplevel directory

noinst_PROGRAMS += iniparser/sc_iniexample
iniparser_sc_iniexample_SOURCES = iniparser/iniexample.c
iniparser_sc_iniexample_CPPFLAGS = $(AM_CPPFLAGS) -I@top_srcdir@/iniparser

iniparser_internal_headers = \
  iniparser/inistring.h iniparser/dictionary.h iniparser/iniparser.h
iniparser_compiled_sources = \
  iniparser/inistring.c iniparser/dictionary.c iniparser/iniparser.c

libsc_internal_headers += $(iniparser_internal_headers)
libsc_compiled_sources += $(iniparser_compiled_sources)
SC_CPPFLAGS += -I@top_srcdir@/iniparser

EXTRA_DIST += iniparser/AUTHORS iniparser/LICENSE
dist_scini_DATA += iniparser/sc_iniparser_twisted.ini
