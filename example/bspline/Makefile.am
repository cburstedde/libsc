
# This file is part of the SC Library
# Makefile.am in example/bspline
# included non-recursively from toplevel directory

EXTRA_DIST += \
        example/bspline/example1.txt \
        example/bspline/example2.txt \
        example/bspline/example3.txt

bin_PROGRAMS += example/bspline/sc_bspline \
								example/bspline/sc_bspline_per
example_bspline_sc_bspline_SOURCES = example/bspline/bspline.c
example_bspline_sc_bspline_per_SOURCES = example/bspline/bspline_per.c

LINT_CSOURCES += $(example_bspline_sc_bspline_SOURCES)
