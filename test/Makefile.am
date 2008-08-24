
# This file is part of the SC Library
# Makefile.am test
# included non-recursively from toplevel directory

sc_test_programs = \
        test/sc_test_dmatrix \
        test/sc_test_dmatrix_pool \
        test/sc_test_pqueue \
        test/sc_test_allgather

check_PROGRAMS += $(sc_test_programs)

test_sc_test_dmatrix_SOURCES = test/test_dmatrix.c
test_sc_test_dmatrix_pool_SOURCES = test/test_dmatrix_pool.c
test_sc_test_pqueue_SOURCES = test/test_pqueue.c
test_sc_test_allgather_SOURCES = test/test_allgather.c

TESTS += $(sc_test_programs)

LINT_CSOURCES += \
        $(test_sc_test_dmatrix_SOURCES) \
        $(test_sc_test_dmatrix_pool_SOURCES) \
        $(test_sc_test_pqueue_SOURCES) \
        $(test_sc_test_allgather_SOURCES)
