
# This file is part of the SC Library
# Makefile.am test
# included non-recursively from toplevel directory

sc_test_programs = \
        test/sc_test_allgather \
        test/sc_test_arrays \
        test/sc_test_builtin \
        test/sc_test_io_sink \
        test/sc_test_keyvalue \
        test/sc_test_node_comm \
        test/sc_test_notify \
        test/sc_test_reduce \
        test/sc_test_search \
        test/sc_test_sort \
        test/sc_test_sortb \
        test/sc_test_version \
        test/sc_test_helpers \
        test/sc3_test_containers
## Reenable and properly verify pqueue when it is actually used
##      test/sc_test_pqueue \

check_PROGRAMS += $(sc_test_programs)

test_sc_test_allgather_SOURCES = test/test_allgather.c
test_sc_test_arrays_SOURCES = test/test_arrays.c
test_sc_test_builtin_SOURCES = test/test_builtin.c
test_sc_test_io_sink_SOURCES = test/test_io_sink.c
test_sc_test_keyvalue_SOURCES = test/test_keyvalue.c
test_sc_test_notify_SOURCES = test/test_notify.c
test_sc_test_node_comm_SOURCES = test/test_node_comm.c
## Reenable and properly verify pqueue when it is actually used
## test_sc_test_pqueue_SOURCES = test/test_pqueue.c
test_sc_test_reduce_SOURCES = test/test_reduce.c
test_sc_test_search_SOURCES = test/test_search.c
test_sc_test_sort_SOURCES = test/test_sort.c
test_sc_test_sortb_SOURCES = test/test_sortb.c
test_sc_test_version_SOURCES = test/test_version.c
test_sc_test_helpers_SOURCES = test/test_helpers.c
test_sc3_test_containers_SOURCES = test/test_containers.c

TESTS += $(sc_test_programs)

LINT_CSOURCES += \
        $(test_sc_test_allgather_SOURCES) \
        $(test_sc_test_arrays_SOURCES) \
        $(test_sc_test_builtin_SOURCES) \
        $(test_sc_test_io_sink_SOURCES) \
        $(test_sc_test_keyvalue_SOURCES) \
        $(test_sc_test_notify_SOURCES) \
        $(test_sc_test_pqueue_SOURCES) \
        $(test_sc_test_reduce_SOURCES) \
        $(test_sc_test_search_SOURCES) \
        $(test_sc_test_sort_SOURCES) \
        $(test_sc_test_sortb_SOURCES) \
        $(test_sc_test_version_SOURCES) \
        $(test_sc_test_helpers_SOURCES) \
        $(test_sc3_test_containers_SOURCES)
