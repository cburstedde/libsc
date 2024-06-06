set(VALGRIND_COMMAND)

find_program(VALGRIND "valgrind")
if( VALGRIND AND SC_TEST_WITH_VALGRIND )
    set(VALGRIND_COMMAND ${VALGRIND} --error-exitcode=1)
elseif( SC_TEST_WITH_VALGRIND )
    message(FATAL_ERROR "TEST_WITH_VALGRIND was set, but valgrind was not found")
endif()
