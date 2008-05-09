
# Makefile.am in sc/zlib
# included non-recursively from toplevel directory
#
# This Makefile.am is part of SC and NOT part of zlib.
# All other files in this directory are unchanged as in zlib 1.2.3.

EXTRA_DIST += zlib/ChangeLog zlib/FAQ zlib/README zlib/algorithm.txt

if SC_ZLIB
zlib_internal_headers = \
	zlib/zconf.h zlib/zlib.h \
	zlib/crc32.h zlib/inffast.h zlib/inflate.h zlib/trees.h \
	zlib/zutil.h zlib/deflate.h zlib/inffixed.h zlib/inftrees.h
zlib_installed_headers =
zlib_compiled_sources = \
	zlib/adler32.c zlib/compress.c zlib/crc32.c zlib/gzio.c \
	zlib/uncompr.c zlib/deflate.c zlib/trees.c zlib/zutil.c \
	zlib/inflate.c zlib/infback.c zlib/inftrees.c zlib/inffast.c

src_libsc_a_SOURCES += \
	$(zlib_internal_headers) \
	$(zlib_installed_headers) \
	$(zlib_compiled_sources)
src_libsc_a_CPPFLAGS += -I@top_srcdir@/zlib
endif
