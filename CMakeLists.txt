cmake_minimum_required(VERSION 3.15...3.28)
enable_testing()

find_package( Git REQUIRED )
execute_process( 
  COMMAND ${GIT_EXECUTABLE} describe --tags 
  COMMAND cut -c 2- 
  COMMAND cut -d- -f1
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE SC_VERSION 
  OUTPUT_STRIP_TRAILING_WHITESPACE 
)

find_program( GIT_VERSION_GEN NAMES git-version-gen PATHS ${CMAKE_CURRENT_SOURCE_DIR}/build-aux NO_DEFAULT_PATH )
if( GIT_VERSION_GEN )
  execute_process(
    COMMAND ${GIT_VERSION_GEN} .tarball-version
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake
    RESULT_VARIABLE _err
    OUTPUT_VARIABLE git_version
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()


project( 
  SC
  LANGUAGES C
  HOMEPAGE_URL https://www.p4est.org/
  DESCRIPTION "The SC library supports parallel scientific applications."
  VERSION ${SC_VERSION} 
)


if( NOT CMAKE_BUILD_TYPE )
  set( CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE )
  message( STATUS "Set build type to '${CMAKE_BUILD_TYPE}' as none was specified." )
  set_property( CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo" )
endif()

set( CMAKE_C_STANDARD 99 )

include( cmake/config.cmake )
include( cmake/pkgconf.cmake )
include( cmake/install.cmake )

include( GNUInstallDirs )
include( FeatureSummary )

message( STATUS "libsc ${SC_VERSION} install prefix: ${CMAKE_INSTALL_PREFIX}" )

# --- iniparser
add_library(iniparser OBJECT iniparser/inistring.c iniparser/dictionary.c iniparser/iniparser.c)
target_include_directories(iniparser PRIVATE iniparser src ${PROJECT_BINARY_DIR}/include)
target_link_libraries(iniparser PRIVATE $<$<BOOL:${SC_ENABLE_MPI}>:MPI::MPI_C>)

# --- libb64
add_library(libb64 OBJECT libb64/cencode.c libb64/cdecode.c)
target_include_directories(libb64 PRIVATE libb64)

# --- sc
add_library( sc $<TARGET_OBJECTS:iniparser> $<TARGET_OBJECTS:libb64> )
set_property( TARGET sc PROPERTY EXPORT_NAME SC )
set_property( TARGET sc PROPERTY SOVERSION ${SC_SOVERSION} )
target_include_directories(
  sc
  PRIVATE 
    iniparser libb64
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries( 
  sc 
  PUBLIC
    $<$<BOOL:${SC_ENABLE_MPI}>:MPI::MPI_C>
    $<$<BOOL:${SC_HAVE_ZLIB}>:ZLIB::ZLIB>
    $<$<BOOL:${SC_HAVE_JSON}>:jansson::jansson>
    $<$<BOOL:${SC_NEED_M}>:m>
    $<$<BOOL:${WIN32}>:${WINSOCK_LIBRARIES}>
)

# imported target, for use from parent project
add_library(SC::SC INTERFACE IMPORTED GLOBAL)
target_link_libraries(SC::SC INTERFACE sc)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/ ${PROJECT_BINARY_DIR}/include/
TYPE INCLUDE
FILES_MATCHING PATTERN "*.h"
)

install(TARGETS sc EXPORT ${PROJECT_NAME}-targets)

add_subdirectory(src)

if(SC_BUILD_TESTING)
  set(VALGRIND_COMMAND)

  find_program(VALGRIND "valgrind")
  if(VALGRIND AND TEST_WITH_VALGRIND)
      set(VALGRIND_COMMAND ${VALGRIND} --error-exitcode=1)
  elseif(TEST_WITH_VALGRIND)
      message(FATAL_ERROR "TEST_WITH_VALGRIND was set, but valgrind was not found")
  endif()

  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.22)
    add_subdirectory(example)
  endif()

  add_subdirectory(test)
endif()

# --- packaging

add_feature_info(MPI SC_ENABLE_MPI "MPI features of libsc")
add_feature_info(OpenMP SC_ENABLE_OPENMP "OpenMP features of libsc")
add_feature_info(ZLIB SC_HAVE_ZLIB "ZLIB features of libsc")
add_feature_info(shared SC_BUILD_SHARED_LIBS "shared libsc library")
add_feature_info(json SC_HAVE_JSON "JSON features of libsc")

feature_summary(WHAT ENABLED_FEATURES DISABLED_FEATURES)
