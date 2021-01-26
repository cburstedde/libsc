set(exs function)
set(tests function pthread)
if(NOT MSVC)
  list(APPEND exs pthread)
endif()

foreach(e ${exs})

  add_executable(sc_${e} ${e}/${e}.c)
  target_include_directories(sc_${e} PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/include)
  target_link_libraries(sc_${e} PRIVATE sc)
  if(ZLIB_FOUND)
    target_link_libraries(sc_${e} PRIVATE ZLIB::ZLIB)
  endif()
  if(MPI_FOUND)
    target_link_libraries(sc_${e} PRIVATE MPI::MPI_C)
  endif()

  if(${e} IN_LIST tests)
  add_test(NAME sc:example:${e} COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} $<TARGET_FILE:sc_${e}>)
  endif()

endforeach()
