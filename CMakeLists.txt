cmake_minimum_required(VERSION 3.15...3.28)

enable_testing()



# --- extract version from Git

set(PROJECT_MAJOR 0)
set(PROJECT_MINOR 0)
set(PROJECT_PATCH 0)
set(PROJECT_VERSION 0.0.0)
find_program(GIT_VERSION_GEN NAMES git-version-gen
             PATHS ${PROJECT_SOURCE_DIR}/build-aux NO_DEFAULT_PATH)
if(GIT_VERSION_GEN)
  execute_process(COMMAND ${GIT_VERSION_GEN} .tarball-version
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE _err
    OUTPUT_VARIABLE git_version
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
if(_err EQUAL 0)
  if(git_version MATCHES
                 "^(0|[1-9][0-9]*)[.](0|[1-9][0-9]*)[.](0|[1-9][0-9]*)[.].*")
    set(PROJECT_MAJOR "${CMAKE_MATCH_1}")
    set(PROJECT_MINOR "${CMAKE_MATCH_2}")
    set(PROJECT_PATCH "${CMAKE_MATCH_3}")
    set(PROJECT_VERSION ${PROJECT_MAJOR}.${PROJECT_MINOR}.${PROJECT_PATCH}.999)
  elseif(git_version MATCHES
                 "^(0|[1-9][0-9]*)[.](0|[1-9][0-9]*)[.](0|[1-9][0-9]*)")
    set(PROJECT_MAJOR "${CMAKE_MATCH_1}")
    set(PROJECT_MINOR "${CMAKE_MATCH_2}")
    set(PROJECT_PATCH "${CMAKE_MATCH_3}")
    set(PROJECT_VERSION ${PROJECT_MAJOR}.${PROJECT_MINOR}.${PROJECT_PATCH})
  elseif(git_version MATCHES
                 "^(0|[1-9][0-9]*)[.](0|[1-9][0-9]*)")
    set(PROJECT_MAJOR "${CMAKE_MATCH_1}")
    set(PROJECT_MINOR "${CMAKE_MATCH_2}")
    set(PROJECT_VERSION ${PROJECT_MAJOR}.${PROJECT_MINOR})
  elseif(git_version MATCHES
                 "^(0|[1-9][0-9]*)")
    set(PROJECT_MAJOR "${CMAKE_MATCH_1}")
    set(PROJECT_VERSION ${PROJECT_MAJOR})
  endif()
endif()




set(sc_default_build_type "RelWithDebInfo")

if(NOT CMAKE_BUILD_TYPE)
  if(DEFINED ENV{CMAKE_BUILD_TYPE})
    set(CMAKE_BUILD_TYPE $ENV{CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build." FORCE)
  endif()
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "${sc_default_build_type}" CACHE STRING "Choose the type of build." FORCE)
  message(STATUS "Set build type to '${CMAKE_BUILD_TYPE}' as none was specified.")
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()


project(SC
LANGUAGES C
HOMEPAGE_URL https://www.p4est.org/
DESCRIPTION "The SC library supports parallel scientific applications."
VERSION ${PROJECT_VERSION})

include(cmake/config.cmake)
include(cmake/pkgconf.cmake)
include(cmake/install.cmake)

include(GNUInstallDirs)
include(FeatureSummary)

set(CMAKE_C_STANDARD 99)
message(STATUS "libsc ${PROJECT_VERSION} "
               "install prefix: ${CMAKE_INSTALL_PREFIX}")

# --- iniparser
add_library(iniparser OBJECT iniparser/inistring.c iniparser/dictionary.c iniparser/iniparser.c)
target_include_directories(iniparser PRIVATE iniparser src ${PROJECT_BINARY_DIR}/include)
target_link_libraries(iniparser PRIVATE $<$<BOOL:${SC_ENABLE_MPI}>:MPI::MPI_C>)

# --- libb64
add_library(libb64 OBJECT libb64/cencode.c libb64/cdecode.c)
target_include_directories(libb64 PRIVATE libb64)

# --- sc
add_library(sc $<TARGET_OBJECTS:iniparser> $<TARGET_OBJECTS:libb64>)
set_property(TARGET sc PROPERTY EXPORT_NAME SC)
set_property(TARGET sc PROPERTY SOVERSION ${SC_SOVERSION})
target_include_directories(sc
  PRIVATE iniparser libb64
  PUBLIC
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(sc PUBLIC
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
