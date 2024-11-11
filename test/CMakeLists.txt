include(CTest)

set(sc_tests allgather arrays keyvalue notify reduce search sortb version scda)

if(SC_HAVE_RANDOM AND SC_HAVE_SRANDOM)
  list(APPEND sc_tests node_comm)
endif()

if(SC_SIZEOF_LONG GREATER_EQUAL 8)
  list(APPEND sc_tests helpers)
endif()

if(SC_HAVE_UNISTD_H)
  list(APPEND sc_tests sort)
endif()

list(APPEND sc_tests builtin io_sink)

set(MPI_WRAPPER)
if(MPIEXEC_EXECUTABLE)
  set(MPIEXEC_MAX_NUMPROCS 2)
  set(MPI_WRAPPER ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS})
endif()

# ---
foreach(t IN LISTS sc_tests)

  add_executable(sc_test_${t} test_${t}.c)
  target_link_libraries(sc_test_${t} PRIVATE SC::SC)
  target_compile_definitions(sc_test_${t} PRIVATE
    $<$<BOOL:${SC_TEST_WITH_VALGRIND}>:SC_ENABLE_VALGRIND=1>
    $<$<BOOL:${SC_HAVE_ZLIB}>:SC_HAVE_ZLIB>
    )

  add_test(NAME ${t} COMMAND ${MPI_WRAPPER} ${VALGRIND_COMMAND} $<TARGET_FILE:sc_test_${t}>)

endforeach()

set_tests_properties(${sc_tests}
PROPERTIES
  LABELS "unit;libsc"
  TIMEOUT 60
)

if(MPIEXEC_EXECUTABLE)
  set_tests_properties(${sc_tests} PROPERTIES RESOURCE_LOCK cpu_mpi)
endif()

if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.22)
  set_tests_properties(${sc_tests} PROPERTIES ENVIRONMENT_MODIFICATION PATH=path_list_append:$<TARGET_FILE_DIR:sc>)
endif()
