cmake_minimum_required(VERSION 3.13...3.20)

include(cmake/git.cmake)

project(SC
LANGUAGES C
HOMEPAGE_URL https://github.com/cburstedde/libsc
DESCRIPTION "The 'sc' auxiliary library"
VERSION ${PROJECT_VERSION})

message(STATUS "${PROJECT_NAME} ${PROJECT_VERSION}")

include(CTest)

include(cmake/options.cmake)

# options.cmake before config.cmake to determine user intent
include(cmake/config.cmake)

# compilers.cmake must be AFTER find_package() calls in config.cmake in general
include(cmake/compilers.cmake)

# --- iniparser
add_library(iniparser OBJECT
            iniparser/inistring.c iniparser/dictionary.c iniparser/iniparser.c)
target_include_directories(iniparser PRIVATE iniparser src ${PROJECT_BINARY_DIR}/include)
if(MPI_FOUND)
  target_link_libraries(iniparser PRIVATE MPI::MPI_C)
endif()

# --- libb64
add_library(libb64 libb64/cencode.c libb64/cdecode.c libb64/b64dec.c libb64/b64enc.c)
target_include_directories(libb64
  PRIVATE libb64
  INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/libb64>
  $<INSTALL_INTERFACE:include>)

install(FILES libb64/libb64.h
  DESTINATION include)

install(TARGETS libb64
  EXPORT ${PROJECT_NAME}Targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)

# --- sc
add_library(sc $<TARGET_OBJECTS:iniparser>)
target_compile_features(sc PRIVATE c_std_99)
target_include_directories(sc
  PRIVATE src src/sc_builtin iniparser ${PROJECT_BINARY_DIR}/include
  INTERFACE
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/sc_builtin>
  $<INSTALL_INTERFACE:include>)
target_link_libraries(sc PRIVATE libb64)
if(MPI_FOUND)
  target_link_libraries(sc PRIVATE MPI::MPI_C)
endif()
if(ZLIB_FOUND)
  target_link_libraries(sc PRIVATE ZLIB::ZLIB)
endif()
if(SC_NEED_M)
  target_link_libraries(sc PRIVATE m)
endif()

# imported target, for use from FetchContent
add_library(SC::SC INTERFACE IMPORTED GLOBAL)
target_link_libraries(SC::SC INTERFACE sc)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/ ${PROJECT_SOURCE_DIR}/src/sc_builtin/ ${PROJECT_BINARY_DIR}/include/
  DESTINATION include
  FILES_MATCHING PATTERN "*.h")

install(TARGETS sc
  EXPORT ${PROJECT_NAME}Targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)

add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(test)
endif(BUILD_TESTING)

# --- packaging

include(cmake/pkgconf.cmake)
include(cmake/install.cmake)

include(FeatureSummary)

add_feature_info(MPI mpi "MPI features of libsc")
add_feature_info(OpenMP openmp "OpenMP features of libsc")

feature_summary(WHAT ENABLED_FEATURES DISABLED_FEATURES)
