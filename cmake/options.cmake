option( SC_ENABLE_MPI "use MPI library" OFF )
option( SC_ENABLE_OPENMP "use OpenMP" OFF )

option( SC_USE_INTERNAL_ZLIB "build ZLIB" OFF )
option( SC_USE_INTERNAL_JSON "build Jansson" OFF )

option( SC_BUILD_SHARED_LIBS "build shared libsc" OFF )
option( SC_BUILD_EXAMPLES "build libsc examples" ON )
option( SC_BUILD_TESTING "build libsc self-tests" ON )
option( SC_TEST_WITH_VALGRIND "run self-tests with valgrind" OFF )

set_property(DIRECTORY PROPERTY EP_UPDATE_DISCONNECTED true)

# Necessary for shared library with Visual Studio / Windows oneAPI
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS true)

# --- auto-ignore build directory
if(NOT PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  file(GENERATE OUTPUT .gitignore CONTENT "*")
endif()

# We are enabling this shortcut even though we recommend to
# use -DSC_ENABLE_MPI:BOOL=... on the cmake command line.
if (DEFINED mpi)
  set_property(CACHE SC_ENABLE_MPI PROPERTY VALUE "${mpi}")
endif()

# Store the MPI setting for the case the user changes the value
set(CACHED_SC_ENABLE_MPI "${SC_ENABLE_MPI}" CACHE STRING "Cached value of SC_ENABLE_MPI")
