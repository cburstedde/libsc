set(tests allgather arrays keyvalue notify reduce search sortb version)

if(SC_HAVE_RANDOM AND SC_HAVE_SRANDOM)
  list(APPEND tests node_comm)
endif()

if(SC_SIZEOF_LONG GREATER_EQUAL 8)
  list(APPEND tests helpers)
endif()

if(SC_HAVE_UNISTD_H)
  list(APPEND tests sort)
endif()

if(SC_HAVE_GETOPT_H)
  list(APPEND tests builtin io_sink)
endif()

# ---

foreach(t ${tests})

  add_executable(test_${t} test_${t}.c)
  target_link_libraries(test_${t} PRIVATE SC::SC)
  if(ZLIB_FOUND)
    target_link_libraries(test_${t} PRIVATE ZLIB::ZLIB)
  endif()
  if(MPI_FOUND)
    target_link_libraries(test_${t} PRIVATE MPI::MPI_C)
  endif()
  if(SC_NEED_M)
    target_link_libraries(test_${t} PRIVATE m)
  endif()

  if(MPIEXEC_EXECUTABLE)
    add_test(NAME sc:${t} COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} $<TARGET_FILE:test_${t}>)
  else()
    add_test(NAME sc:${t} COMMAND $<TARGET_FILE:test_${t}>)
  endif()
  set_tests_properties(sc:${t} PROPERTIES
    TIMEOUT 300
    RESOURCE_LOCK cpu_mpi)

endforeach()
