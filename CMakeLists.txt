cmake_minimum_required(VERSION 3.15)
project(libsc
  LANGUAGES C)

include(CTest)

include(cmake/options.cmake)
include(cmake/compilers.cmake)

if(mpi)
  find_package(MPI COMPONENTS C REQUIRED)
endif()

find_package(ZLIB)
find_package(Threads)

# --- iniparser
add_library(iniparser iniparser/dictionary.c iniparser/iniparser.c)
target_include_directories(iniparser
  INTERFACE iniparser
  PRIVATE src ${PROJECT_BINARY_DIR}/include)
if(MPI_FOUND)
  target_link_libraries(iniparser PRIVATE MPI::MPI_C)
endif()
# --- libb64
add_library(libb64 libb64/cencode.c libb64/cdecode.c libb64/b64dec.c libb64/b64enc.c)
target_include_directories(libb64 PUBLIC libb64)

# --- sc
add_library(sc)
target_compile_features(sc PRIVATE c_std_99)
target_include_directories(sc
  PUBLIC src src/sc_builtin ${PROJECT_BINARY_DIR}/include)
target_link_libraries(sc PRIVATE iniparser libb64)
if(MPI_FOUND)
  target_link_libraries(sc PRIVATE MPI::MPI_C)
endif()
if(ZLIB_FOUND)
  target_link_libraries(sc PRIVATE ZLIB::ZLIB)
endif()
if(NOT WIN32)
  target_link_libraries(sc PRIVATE m)
endif()

add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(example)
  add_subdirectory(test)
endif(BUILD_TESTING)
