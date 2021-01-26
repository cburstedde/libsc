
function(test_sc_example name files)

add_executable(sc_${name} ${files})
target_include_directories(sc_${name} PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/include)
target_link_libraries(sc_${name} PRIVATE SC::SC)
if(MPI_FOUND)
  target_link_libraries(sc_${name} PRIVATE MPI::MPI_C)
endif()
if(ZLIB_FOUND)
  target_link_libraries(sc_${name} PRIVATE ZLIB::ZLIB)
endif()
if(SC_NEED_M)
  target_link_libraries(sc_${name} PRIVATE m)
endif()

add_test(NAME sc:example:${name} COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} $<TARGET_FILE:sc_${name}>)

endfunction(test_sc_example)


test_sc_example(function function/function.c)
test_sc_example(logging logging/logging.c)
test_sc_example(options options/options.c)
test_sc_example(test_shmem testing/sc_test_shmem.c)

if(OpenMP_FOUND)
  test_sc_example(openmp openmp/openmp.c)
  target_link_libraries(sc_openmp PRIVATE OpenMP::OpenMP_C)
endif()

if(Threads_FOUND)
  check_include_file(pthread.h HAS_PTHREAD)
  if(HAS_PTHREAD)
    test_sc_example(pthread pthread/pthread.c)
    target_link_libraries(sc_pthread PRIVATE Threads::Threads)
  endif()
endif()
