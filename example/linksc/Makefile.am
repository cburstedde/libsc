
## Makefile source file for sc link example

AM_CPPFLAGS = -I@LSC_SC_INC@
AM_LDFLAGS =
ACLOCAL_AMFLAGS = -Iconfig
bin_PROGRAMS =
lib_LTLIBRARIES =

## The library depends on selected make installed libsc code.
lib_LTLIBRARIES += liblinksc.la
liblinksc_la_SOURCES = liblinksc.h liblinksc.c
liblinksc_la_LDFLAGS = -version-info 1:0:0
liblinksc_la_LIBADD = -L@LSC_SC_LIB@ -lsc

## The program depends on more make installed libsc code than
## is being pulled in by liblinksc.la, thus it is specified again.
bin_PROGRAMS += linksc
linksc_SOURCES = linksc.c
linksc_LDADD = liblinksc.la -L@LSC_SC_LIB@ -lsc

## We add -rpath to all executables to find the installed libsc code.
AM_LDFLAGS += -rpath @LSC_SC_LIB@

## LDADD shall contain no linker flags except -L, -l and -dl*open.
## Use LDFLAGS for other linker flags (such as -rpath).
## Use LIBADD to add extra object files to a library.
##
## It is fine to add libtool libraries (.la files) to both LDADD and LIBADD.

## (EXTRA_)target_DEPENDENCIES is derived automatically from _LDADD.
## If the contents of _LDADD are sufficient, _DEPENDENCIES is not needed.

## If make built it, and it is commonly something that one would want to rebuild
## (for instance, a .o file), then mostlyclean should delete it.
## Otherwise, if make built it, then clean should delete it.
clean-local:

## If configure built it, then distclean should delete it.
distclean-local:

## If the maintainer built it (for instance, a .info file), then
## maintainer-clean should delete it. However maintainer-clean should not delete
## anything that needs to exist in order to run ‘./configure && make’.
maintainer-clean-local:
