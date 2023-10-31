cmake_minimum_required(VERSION 3.14...3.28)

include(cmake/git.cmake)

include(cmake/init_build_type.cmake)

project(SC
LANGUAGES C
HOMEPAGE_URL https://www.p4est.org/
DESCRIPTION "The SC library supports parallel scientific applications."
VERSION ${PROJECT_VERSION})

enable_testing()

set(CMAKE_C_STANDARD 99)

include(cmake/options.cmake)
message(STATUS "libsc ${PROJECT_VERSION} "
               "install prefix: ${CMAKE_INSTALL_PREFIX}")
# options.cmake before config.cmake to determine user intent
include(cmake/config.cmake)

# compilers.cmake must be AFTER find_package() calls in config.cmake in general
include(cmake/compilers.cmake)

# --- iniparser
add_library(iniparser OBJECT
            iniparser/inistring.c iniparser/dictionary.c iniparser/iniparser.c)
target_include_directories(iniparser PRIVATE iniparser src ${PROJECT_BINARY_DIR}/include)
target_link_libraries(iniparser PRIVATE $<$<BOOL:${SC_ENABLE_MPI}>:MPI::MPI_C>)

# --- libb64
add_library(libb64 OBJECT libb64/cencode.c libb64/cdecode.c)
target_include_directories(libb64 PRIVATE libb64)

# --- sc
add_library(sc $<TARGET_OBJECTS:iniparser> $<TARGET_OBJECTS:libb64>)
set_property(TARGET sc PROPERTY EXPORT_NAME SC)
set_property(TARGET sc PROPERTY VERSION ${PROJECT_VERSION})
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
)

# imported target, for use from FetchContent
add_library(SC::SC INTERFACE IMPORTED GLOBAL)
target_link_libraries(SC::SC INTERFACE sc)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/ ${PROJECT_BINARY_DIR}/include/
TYPE INCLUDE
FILES_MATCHING PATTERN "*.h"
)

install(TARGETS sc
  EXPORT ${PROJECT_NAME}-targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)

add_subdirectory(src)

if(BUILD_TESTING)
  include(cmake/valgrind.cmake)

  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.22)
    add_subdirectory(example)
  endif()

  add_subdirectory(test)
endif(BUILD_TESTING)

# --- packaging

install(FILES
${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindSC.cmake
DESTINATION cmake)

include(cmake/pkgconf.cmake)
include(cmake/install.cmake)

include(FeatureSummary)

add_feature_info(MPI SC_ENABLE_MPI "MPI features of libsc")
add_feature_info(OpenMP openmp "OpenMP features of libsc")
add_feature_info(ZLIB SC_HAVE_ZLIB "ZLIB features of libsc")
add_feature_info(shared BUILD_SHARED_LIBS "shared libsc library")
add_feature_info(json SC_HAVE_JSON "JSON features of libsc")

feature_summary(WHAT ENABLED_FEATURES DISABLED_FEATURES)
