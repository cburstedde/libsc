set(sc_tests allgather arrays keyvalue notify reduce search sortb version)

if(SC_HAVE_RANDOM AND SC_HAVE_SRANDOM)
  list(APPEND sc_tests node_comm)
endif()

if(SC_SIZEOF_LONG GREATER_EQUAL 8)
  list(APPEND sc_tests helpers)
endif()

if(SC_HAVE_UNISTD_H)
  list(APPEND sc_tests sort)
endif()

if(SC_HAVE_GETOPT_H)
  list(APPEND sc_tests builtin io_sink)
endif()

# ---
set(MPIEXEC_MAX_NUMPROCS 2)
foreach(t IN LISTS sc_tests)

  add_executable(test_${t} test_${t}.c)
  target_link_libraries(test_${t} PRIVATE SC::SC)

  if(MPIEXEC_EXECUTABLE)
    add_test(NAME ${t} COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} $<TARGET_FILE:test_${t}>)
  else()
    add_test(NAME ${t} COMMAND test_${t})
  endif()

endforeach()

set_tests_properties(${sc_tests} PROPERTIES
TIMEOUT 300
LABELS "unit;libsc"
)

if(MPIEXEC_EXECUTABLE)
  set_tests_properties(${sc_tests} PROPERTIES RESOURCE_LOCK cpu_mpi)
endif()

if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.22)
  set_tests_properties(${sc_tests} PROPERTIES ENVIRONMENT_MODIFICATION PATH=path_list_append:$<TARGET_FILE_DIR:sc>)
endif()
