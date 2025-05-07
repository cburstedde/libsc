include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckTypeSize)
include(CheckPrototypeDefinition)
include(CheckCSourceCompiles)

# --- retrieve library interface version from configuration file

file(STRINGS config/sc_soversion.in SC_SOVERSION_READ
             REGEX "^[ \t]*SC_SOVERSION *= *[0-9:]+")
string(REGEX REPLACE ".*([0-9]+):([0-9]+):([0-9]+)" "\\1.\\2.\\3"
             SC_SOVERSION ${SC_SOVERSION_READ})
message(STATUS "libsc SOVERSION configured as ${SC_SOVERSION}")

# --- keep library finds in here so we don't forget to do them first

if( SC_ENABLE_MPI )
  find_package(MPI COMPONENTS C REQUIRED)
endif()

if( SC_USE_INTERNAL_ZLIB )
  message( STATUS "Using internal zlib" )
  include( ${CMAKE_CURRENT_LIST_DIR}/zlib.cmake )
else()
  find_package( ZLIB )

  if( NOT ZLIB_FOUND )
    set( SC_USE_INTERNAL_ZLIB ON )
    message( STATUS "Using internal zlib" )
    include( ${CMAKE_CURRENT_LIST_DIR}/zlib.cmake )
  else()
    set(CMAKE_REQUIRED_LIBRARIES ZLIB::ZLIB)

    check_c_source_compiles(
      "#include <zlib.h>
      int main(){
        z_off_t len = 3000; uLong a = 1, b = 2;
        a == adler32_combine (a, b, len);
        return 0;
      }"
      SC_HAVE_ZLIB
    )
  endif()
endif()

find_package(Threads)

if( SC_USE_INTERNAL_JSON )
  message(STATUS "Using builtin jansson")
  include(${CMAKE_CURRENT_LIST_DIR}/jansson.cmake)
else()
  find_package(jansson CONFIG)
  if(TARGET jansson::jansson)
    set(SC_HAVE_JSON ON CACHE BOOL "JSON features enabled")
  else()
    set(SC_HAVE_JSON OFF CACHE BOOL "JSON features disabled")
  endif()
endif()

# --- set global compile environment

# Build all targets with -fPIC so that libsc itself can be linked as a
# shared library, or linked into a shared library.
include(CheckPIESupported)
check_pie_supported()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --- generate sc_config.h

set(CMAKE_REQUIRED_INCLUDES)
set(CMAKE_REQUIRED_LIBRARIES)

check_symbol_exists(sqrt math.h SC_NONEED_M)

if(NOT SC_NONEED_M)
  set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} m)
  check_symbol_exists(sqrt math.h SC_NEED_M)
endif()

set(SC_ENABLE_PTHREAD ${CMAKE_USE_PTHREADS_INIT})
set(SC_ENABLE_MEMALIGN 1)

# user has requested a different MPI setting, so we need to clear these cache variables to recheck
if(NOT "${SC_ENABLE_MPI}" STREQUAL "${CACHED_SC_ENABLE_MPI}")
  unset(SC_ENABLE_MPICOMMSHARED CACHE)
  unset(SC_ENABLE_MPITHREAD CACHE)
  unset(SC_ENABLE_MPIWINSHARED CACHE)
  unset(SC_HAVE_AINT_DIFF CACHE)
  unset(SC_HAVE_MPI_UNSIGNED_LONG_LONG CACHE)
  unset(SC_HAVE_MPI_SIGNED_CHAR CACHE)
  unset(SC_HAVE_MPI_INT8_T CACHE)
  unset(SC_ENABLE_MPIIO CACHE)
  # Update cached variable
  set(CACHED_SC_ENABLE_MPI "${SC_ENABLE_MPI}" CACHE STRING "Cached value of SC_ENABLE_MPI")
endif()

if( SC_ENABLE_MPI )
  # perform check to set SC_ENABLE_MPICOMMSHARED
  include(cmake/check_mpicommshared.cmake)
  # perform check to set SC_ENABLE_MPIIO
  include(cmake/check_mpiio.cmake)
  # perform check to set SC_ENABLE_MPITHREAD
  include(cmake/check_mpithread.cmake)
  # perform check to set SC_ENABLE_MPIWINSHARED
  include(cmake/check_mpiwinshared.cmake)
  # perform check to set SC_HAVE_AINT_DIFF
  include(cmake/check_mpiaintdiff.cmake)
  # perform check of newer MPI data types
  include(cmake/check_mpitype.cmake)

  # Note: Rewrite this test in the form of the above.
  check_c_source_compiles("
  #include <mpi.h>
  int main (void) {
    MPI_Comm subcomm;
    MPI_Init ((int *) 0, (char ***) 0);
    MPI_Comm_split_type(MPI_COMM_WORLD,OMPI_COMM_TYPE_SOCKET,0,MPI_INFO_NULL,&subcomm);
    MPI_Finalize ();
    return 0;
  }" SC_ENABLE_OMPICOMMSOCKET)
endif()

check_symbol_exists(realloc stdlib.h SC_ENABLE_USE_REALLOC)

check_symbol_exists(aligned_alloc stdlib.h SC_HAVE_ALIGNED_ALLOC)
if(NOT SC_HAVE_ALIGNED_ALLOC)
  check_symbol_exists(_aligned_malloc malloc.h SC_HAVE_ALIGNED_MALLOC)
endif()

check_symbol_exists(backtrace execinfo.h SC_HAVE_BACKTRACE)
check_symbol_exists(backtrace_symbols execinfo.h SC_HAVE_BACKTRACE_SYMBOLS)

check_include_file(execinfo.h SC_HAVE_EXECINFO_H)
check_symbol_exists(fsync unistd.h SC_HAVE_FSYNC)
check_include_file(inttypes.h SC_HAVE_INTTYPES_H)
check_include_file(memory.h SC_HAVE_MEMORY_H)

check_symbol_exists(posix_memalign stdlib.h SC_HAVE_POSIX_MEMALIGN)
check_symbol_exists(basename libgen.h SC_HAVE_BASENAME)

# requires -D_GNU_SOURCE, missing on MinGW
# https://www.gnu.org/software/gnulib/manual/html_node/qsort_005fr.html
set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_symbol_exists(qsort_r stdlib.h SC_HAVE_QSORT_R)
if(SC_HAVE_QSORT_R)
  # check for GNU version of qsort_r
  check_prototype_definition(qsort_r
	"void qsort_r(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *, void *), void *arg)"
	"" "stdlib.h" SC_HAVE_GNU_QSORT_R)
  # check for BSD version of qsort_r
  check_prototype_definition(qsort_r
	"void qsort_r(void *base, size_t nmemb, size_t size, void *thunk, int (*compar)(void *, const void *, const void *))"
	"" "stdlib.h" SC_HAVE_BSD_QSORT_R)
endif()
set(CMAKE_REQUIRED_DEFINITIONS)

check_symbol_exists(fabs math.h SC_HAVE_FABS)

check_include_file(signal.h SC_HAVE_SIGNAL_H)
check_include_file(stdint.h SC_HAVE_STDINT_H)
check_include_file(stdlib.h SC_HAVE_STDLIB_H)

check_include_file(string.h SC_HAVE_STRING_H)
if(SC_HAVE_STDLIB_H)
  check_symbol_exists(random stdlib.h SC_HAVE_RANDOM)
  check_symbol_exists(srandom stdlib.h SC_HAVE_SRANDOM)
endif()

check_symbol_exists(strtoll stdlib.h SC_HAVE_STRTOLL)
check_symbol_exists(strtok_r string.h SC_HAVE_STRTOK_R)

check_include_file(sys/types.h SC_HAVE_SYS_TYPES_H)

check_include_file(sys/time.h SC_HAVE_SYS_TIME_H)
check_include_file(time.h SC_HAVE_TIME_H)

check_symbol_exists(gettimeofday sys/time.h SC_HAVE_GETTIMEOFDAY)

if(WIN32)
  set(WINSOCK_LIBRARIES wsock32 ws2_32 Iphlpapi)
endif()

check_include_file(libgen.h SC_HAVE_LIBGEN_H)
check_include_file(unistd.h SC_HAVE_UNISTD_H)
if(SC_HAVE_UNISTD_H)
  check_include_file(getopt.h SC_HAVE_GETOPT_H)
endif()

if(NOT SC_HAVE_GETOPT_H)
  set(SC_PROVIDE_GETOPT True)
endif()

check_include_file(sys/ioctl.h SC_HAVE_SYS_IOCTL_H)
check_include_file(sys/select.h SC_HAVE_SYS_SELECT_H)
check_include_file(sys/stat.h SC_HAVE_SYS_STAT_H)
check_include_file(fcntl.h SC_HAVE_FCNTL_H)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  check_include_file(linux/videodev2.h SC_HAVE_LINUX_VIDEODEV2_H)
  check_include_file(linux/version.h SC_HAVE_LINUX_VERSION_H)

  if(SC_HAVE_LINUX_VIDEODEV2_H AND SC_HAVE_LINUX_VERSION_H)
    file(READ ${CMAKE_CURRENT_LIST_DIR}/check_v4l2.c check_v4l2_src)
    check_c_source_compiles([=[${check_v4l2_src}]=] SC_ENABLE_V4L2)
  endif()
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  set(SC_ENABLE_DEBUG 1)
else()
  set(SC_ENABLE_DEBUG 0)
endif()

configure_file(${CMAKE_CURRENT_LIST_DIR}/sc_config.h.in ${PROJECT_BINARY_DIR}/include/sc_config.h)

file(TIMESTAMP ${PROJECT_BINARY_DIR}/include/sc_config.h _t)
message(VERBOSE "sc_config.h was last generated ${_t}")

# --- sanity check of MPI sc_config.h

# check if libsc was configured properly
set(CMAKE_REQUIRED_FLAGS)
set(CMAKE_REQUIRED_INCLUDES)
set(CMAKE_REQUIRED_LIBRARIES)
set(CMAKE_REQUIRED_DEFINITIONS)

# libsc and current project must both be compiled with/without MPI
check_symbol_exists("SC_ENABLE_MPI" ${PROJECT_BINARY_DIR}/include/sc_config.h SC_ENABLE_MPI)
check_symbol_exists("SC_ENABLE_MPIIO" ${PROJECT_BINARY_DIR}/include/sc_config.h SC_ENABLE_MPIIO)

# Check for deprecated MPI and MPI I/O configuration.
# This is done at the end of config.cmake to ensure that the warning is visible
# without scrolling.
if (SC_ENABLE_MPI AND NOT SC_ENABLE_MPIIO)
  message(WARNING "libsc MPI configured but MPI I/O is not configured/found: DEPRECATED")
  message(NOTICE "This configuration is DEPRECATED and will be disallowed in the future.")
  message(NOTICE "If the MPI File API is not available, please disable MPI altogether.")
endif()
