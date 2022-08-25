cmake_minimum_required(VERSION 3.13...3.24)
project(libscExamples LANGUAGES C)

enable_testing()

include(CheckSymbolExists)
include(CheckIncludeFile)

option(mpi "use MPI" off)

set_property(DIRECTORY PROPERTY LABELS "example")

if(CMAKE_VERSION VERSION_LESS 3.21)
  get_property(not_top DIRECTORY PROPERTY PARENT_DIRECTORY)
  if(NOT not_top)
    set(PROJECT_IS_TOP_LEVEL true)
  endif()
endif()

if(PROJECT_IS_TOP_LEVEL)
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules)

  # --- auto-ignore build directory
  if(NOT EXISTS ${PROJECT_BINARY_DIR}/.gitignore)
    file(WRITE ${PROJECT_BINARY_DIR}/.gitignore "*")
  endif()


  if(mpi)
    find_package(MPI COMPONENTS C REQUIRED)
    if(NOT MPIEXEC_EXECUTABLE)
      message(FATAL_ERROR "MPIEXEC not found")
    endif()
  endif(mpi)

  find_package(ZLIB REQUIRED)
  find_package(Threads)

  # --- find our library
  find_package(SC REQUIRED)
  set(cfg_h ${SC_INCLUDE_DIRS}/sc_config.h)

  # --- get system capabilities

  check_symbol_exists(sqrt math.h SC_NONEED_M)
  if(NOT SC_NONEED_M)
    set(CMAKE_REQUIRED_LIBRARIES m)
    check_symbol_exists(sqrt math.h SC_NEED_M)
  endif()

  check_symbol_exists(SC_HAVE_UNISTD_H ${cfg_h} SC_HAVE_UNISTD_H)
  check_symbol_exists(SC_HAVE_GETOPT_H ${cfg_h} SC_HAVE_GETOPT_H)
  check_symbol_exists(SC_ENABLE_V4L2 ${cfg_h} SC_ENABLE_V4L2)

  target_link_libraries(SC::SC INTERFACE
  $<$<BOOL:${MPI_C_FOUND}>:MPI::MPI_C>
  $<$<BOOL:${ZLIB_FOUND}>:ZLIB::ZLIB>
  $<$<BOOL:${SC_NEED_M}>:m>
  )
endif()

# --- build examples

function(test_sc_example name files)

add_executable(sc_${name} ${files})

target_link_libraries(sc_${name} PRIVATE SC::SC)

if(MPI_C_FOUND)
  add_test(NAME sc:example:${name} COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} $<TARGET_FILE:sc_${name}>)
else()
  add_test(NAME sc:example:${name} COMMAND sc_${name})
endif()

set_property(TEST sc:example:${name} PROPERTY TIMEOUT 10)
if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.22)
  set_property(TEST sc:example:${name} PROPERTY ENVIRONMENT_MODIFICATION "PATH=path_list_append:${SC_INCLUDE_DIR}/../bin")
endif()

endfunction(test_sc_example)


test_sc_example(function function/function.c)
test_sc_example(logging logging/logging.c)
test_sc_example(test_shmem testing/sc_test_shmem.c)

if(SC_HAVE_GETOPT_H)
  test_sc_example(options options/options.c)
endif()

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

if(SC_ENABLE_V4L2)
  test_sc_example(v4l2 v4l2/v4l2.c)
endif()
