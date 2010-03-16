
# This file is part of the SC Library
# Makefile.am in zlib
# included non-recursively from toplevel directory
#
# This Makefile.am is part of the SC Library and NOT part of zlib.
# All other files in this directory are unchanged as in zlib 1.2.3.

EXTRA_DIST += \
        zlib/ChangeLog zlib/FAQ zlib/README zlib/algorithm.txt

if SC_PROVIDE_ZLIB

bin_PROGRAMS += zlib/sc_zlib_example

zlib_sc_zlib_example_SOURCES = zlib/example.c

zlib_internal_headers = \
        zlib/zlib.h zlib/zconf.h \
        zlib/crc32.h zlib/deflate.h zlib/gzguts.h zlib/inffast.h \
        zlib/inffixed.h zlib/inflate.h zlib/inftrees.h zlib/trees.h \
        zlib/zutil.h
zlib_compiled_sources = \
        zlib/adler32.c zlib/compress.c zlib/crc32.c zlib/deflate.c \
        zlib/gzclose.c zlib/gzlib.c zlib/gzread.c zlib/gzwrite.c \
        zlib/infback.c zlib/inffast.c zlib/inflate.c zlib/inftrees.c \
        zlib/trees.c zlib/uncompr.c zlib/zutil.c

libsc_internal_headers += $(zlib_internal_headers)
libsc_compiled_sources += $(zlib_compiled_sources)

endif
