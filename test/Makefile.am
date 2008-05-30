
# This file is part of the SC Library
# Makefile.am test
# included non-recursively from toplevel directory

sc_test_programs = \
        test/sc_test_pqueue

check_PROGRAMS += $(sc_test_programs)

test_sc_test_pqueue_SOURCES = test/test_pqueue.c

TESTS += $(sc_test_programs)

LINT_CSOURCES += \
        $(test_sc_test_pqueue_SOURCES)
