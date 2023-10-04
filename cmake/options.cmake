
include(GNUInstallDirs)

option(mpi "use MPI library" off)
option(openmp "use OpenMP" off)
option(zlib "build ZLIB" on)
option(BUILD_TESTING "build libsc self-tests" on)
option(BUILD_SHARED_LIBS "build shared libsc")

# --- default install directory under build/local
# users can specify like "cmake -B build -DCMAKE_INSTALL_PREFIX=~/mydir"
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  # will not take effect without FORCE
  set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/local" CACHE PATH "Install top-level directory" FORCE)
endif()


# Necessary for shared library with Visual Studio / Windows oneAPI
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS true)

# --- auto-ignore build directory
if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  file(WRITE ${PROJECT_BINARY_DIR}/.gitignore "*")
endif()
