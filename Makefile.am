
## Makefile.am in sc

ACLOCAL_AMFLAGS = -I config -I gnulib/m4

SUBDIRS = gnulib src

EXTRA_DIST = bootstrap scgnulib scindent

if HAVE_SPLINT # splint is used non-recursively

SPLINT_FLAGS = \
	-weak -badflag -preproc -posixlib \
	-unrecogcomments -formatcode

splint:
	for CFILE in @top_srcdir@/src/*.c ; do \
	  echo "Checking $$CFILE" ; \
	  splint -DP4EST_SPLINT $(SPLINT_FLAGS) \
	    -I@top_srcdir@/src @MPI_INCLUDES@ "$$CFILE" || exit 1 ; \
	done

.PHONY: splint

endif # HAVE_SPLINT
