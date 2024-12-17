cmake_minimum_required(VERSION 3.15...3.28)

enable_testing()
include(cmake/git.cmake)
include(cmake/init_build_type.cmake)
include(cmake/options.cmake)

project(SC
LANGUAGES C
HOMEPAGE_URL https://www.p4est.org/
DESCRIPTION "The SC library supports parallel scientific applications."
VERSION ${PROJECT_VERSION})

include(GNUInstallDirs)
include(cmake/config.cmake) # options.cmake before config.cmake to determine user intent
include(cmake/compilers.cmake) # compilers.cmake must be AFTER find_package() calls in config.cmake in general

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
  $<$<BOOL:${SC_HAVE_JSON}>:jansson::jansson>
  $<$<BOOL:${SC_NEED_M}>:m>
  $<$<BOOL:${WIN32}>:${WINSOCK_LIBRARIES}>
)

if ( SC_ENABLE_MPI )
  target_link_libraries(sc PUBLIC MPI::MPI_C)
endif()

if ( SC_HAVE_ZLIB )
  target_link_libraries(sc PUBLIC ZLIB::ZLIB)
endif()

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
  include(cmake/valgrind.cmake)
  add_subdirectory(test)
endif()

if(SC_BUILD_EXAMPLES AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.22)
  add_subdirectory(example)
endif()

# --- packaging

include(cmake/pkgconf.cmake)
include(cmake/install.cmake)

include(FeatureSummary)

add_feature_info(MPI SC_ENABLE_MPI "MPI features of libsc")
add_feature_info(ZLIB SC_HAVE_ZLIB "ZLIB features of libsc")
add_feature_info(shared SC_BUILD_SHARED_LIBS "shared libsc library")
add_feature_info(json SC_HAVE_JSON "JSON features of libsc")

feature_summary(WHAT ENABLED_FEATURES DISABLED_FEATURES)
