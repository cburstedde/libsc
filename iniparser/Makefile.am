
# Makefile.am in iniparser
# included non-recursively from toplevel directory

noinst_PROGRAMS += iniparser/sc_iniexample
iniparser_sc_iniexample_SOURCES = iniparser/iniexample.c
iniparser_sc_iniexample_CPPFLAGS = $(AM_CPPFLAGS) -I@top_srcdir@/iniparser

src_libsc_a_SOURCES += \
	iniparser/dictionary.h iniparser/dictionary.c \
	iniparser/iniparser.h iniparser/iniparser.c
src_libsc_a_CPPFLAGS += -I@top_srcdir@/iniparser

EXTRA_DIST += iniparser/AUTHORS iniparser/LICENSE iniparser/twisted.ini
