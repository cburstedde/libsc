option(mpi "use MPI library" off)
option(openmp "use OpenMP" off)
option(zlib "build ZLIB" off)
option(BUILD_TESTING "build libsc self-tests" on)
option(TEST_WITH_VALGRIND "run self-tests with valgrind" OFF)
option(BUILD_SHARED_LIBS "build shared libsc")

# --- default install directory under build/local
# users can specify like "cmake -B build -DCMAKE_INSTALL_PREFIX=~/mydir"

if(CMAKE_VERSION VERSION_LESS 3.21)
  get_property(_not_top DIRECTORY PROPERTY PARENT_DIRECTORY)
  if(NOT _not_top)
    set(SC_IS_TOP_LEVEL true)
  endif()
endif()

if(SC_IS_TOP_LEVEL AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  # will not take effect without FORCE
  set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/local" CACHE PATH "Install top-level directory" FORCE)
endif()

set_property(DIRECTORY PROPERTY EP_UPDATE_DISCONNECTED true)

# Necessary for shared library with Visual Studio / Windows oneAPI
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS true)

# --- auto-ignore build directory
if(NOT PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  file(GENERATE OUTPUT .gitignore CONTENT "*")
endif()
