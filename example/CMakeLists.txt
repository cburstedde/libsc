cmake_minimum_required(VERSION 3.22)
project(libscExamples LANGUAGES C)

enable_testing()

include(CheckSymbolExists)
include(CheckIncludeFile)

if(PROJECT_IS_TOP_LEVEL)
  find_package(SC CONFIG REQUIRED)
endif()

# --- build examples

function(test_sc_example name files)

add_executable(sc_${name} ${files})

target_link_libraries(sc_${name} PRIVATE SC::SC)

if(SC_ENABLE_MPI)
  add_test(NAME sc:example:${name} COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} $<TARGET_FILE:sc_${name}>)
else()
  add_test(NAME sc:example:${name} COMMAND sc_${name})
endif()

if(WIN32)
  set_property(TEST sc:example:${name} PROPERTY ENVIRONMENT_MODIFICATION "PATH=path_list_append:${SC_INCLUDE_DIR}/../bin")
endif()

endfunction(test_sc_example)


test_sc_example(function function/function.c)
test_sc_example(logging logging/logging.c)
test_sc_example(test_shmem testing/sc_test_shmem.c)

configure_file(options/sc_options_example.ini sc_options_example.ini COPYONLY)
configure_file(options/sc_options_example.json sc_options_example.json COPYONLY)
configure_file(options/sc_options_preload.ini sc_options_preload.ini COPYONLY)
test_sc_example(options options/options.c)

# The OpenMP example is disabled
# We are likely removing the OpenMP configuration entirely
#
# if(OpenMP_FOUND)
#   test_sc_example(openmp openmp/openmp.c)
#   target_link_libraries(sc_openmp PRIVATE OpenMP::OpenMP_C)
# endif()

if(CMAKE_USE_PTHREADS_INIT)
  test_sc_example(pthread pthread/pthread.c)
  target_link_libraries(sc_pthread PRIVATE Threads::Threads)
endif()

if(SC_ENABLE_V4L2)
  test_sc_example(v4l2 v4l2/v4l2.c)
endif()

if(NOT PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  file(GENERATE OUTPUT .gitignore CONTENT "*")
endif()
