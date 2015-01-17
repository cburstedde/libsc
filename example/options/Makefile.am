
# This file is part of the SC Library
# Makefile.am in example/options
# included non-recursively from toplevel directory

bin_PROGRAMS += example/options/sc_options example/options/sc_logging
example_options_sc_options_SOURCES = example/options/options.c
example_options_sc_logging_SOURCES = example/options/logging.c

dist_scini_DATA += example/options/options.ini example/options/preload.ini

LINT_CSOURCES += $(example_options_sc_options_SOURCES)
