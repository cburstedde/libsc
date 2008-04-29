
## Makefile.am in sc

ACLOCAL_AMFLAGS = -I config -I gnulib/m4

SUBDIRS = gnulib src

EXTRA_DIST = bootstrap scgnulib scindent

if HAVE_SPLINT # splint is used non-recursively
splint:
	for CFILE in @top_srcdir@/src/*.c ; do \
	  echo "Checking $$CFILE" ; \
	  splint -DP4EST_SPLINT -preproc -weak -unrecogcomments \
	    -I@top_srcdir@/src @MPI_INCLUDES@ +posixlib "$$CFILE" || exit 1 ; \
	done

.PHONY: splint
endif
