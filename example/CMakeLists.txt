cmake_minimum_required(VERSION 3.22)
project(libscExamples LANGUAGES C)

enable_testing()

include(CheckSymbolExists)
include(CheckIncludeFile)

if(PROJECT_IS_TOP_LEVEL)
  find_package(SC CONFIG REQUIRED)
endif()

# --- build examples

function(sc_example name files)

add_executable(sc_${name} ${files})

target_link_libraries(sc_${name} PRIVATE SC::SC)
set_target_properties(sc_${name}
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
  LABELS sc
)

endfunction(sc_example)

sc_example(function function/function.c)
sc_example(logging logging/logging.c)
sc_example(test_shmem testing/sc_test_shmem.c)

configure_file(options/sc_options_example.ini sc_options_example.ini COPYONLY)
configure_file(options/sc_options_example.json sc_options_example.json COPYONLY)
configure_file(options/sc_options_preload.ini sc_options_preload.ini COPYONLY)
sc_example(options options/options.c)

if(CMAKE_USE_PTHREADS_INIT)
  sc_example(pthread pthread/pthread.c)
  target_link_libraries(sc_pthread PRIVATE Threads::Threads)
endif()

if(SC_ENABLE_V4L2)
  sc_example(v4l2 v4l2/v4l2.c)
endif()

if(NOT PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  file(GENERATE OUTPUT .gitignore CONTENT "*")
endif()
