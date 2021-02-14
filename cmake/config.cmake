include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckTypeSize)

message(STATUS "${PROJECT_NAME} CMake ${CMAKE_VERSION}")

# --- keep library finds in here so we don't forget to do them first

if(mpi)
  find_package(MPI COMPONENTS C REQUIRED)
endif()
if(openmp)
  find_package(OpenMP COMPONENTS C REQUIRED)
endif()
find_package(ZLIB)
find_package(Threads)

# --- generate sc_config.h

set(CMAKE_REQUIRED_INCLUDES)
set(CMAKE_REQUIRED_LIBRARIES)

if(MPI_FOUND)
  set(CMAKE_REQUIRED_LIBRARIES MPI::MPI_C)
  set(SC_CC \"${MPI_C_COMPILER}\")
  set(SC_CPP ${MPI_C_COMPILER})
else()
  set(SC_CC \"${CMAKE_C_COMPILER}\")
  set(SC_CPP ${CMAKE_C_COMPILER})
endif()

check_symbol_exists(sqrt math.h SC_NONEED_M)

if(NOT SC_NONEED_M)
  set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} m)
  check_symbol_exists(sqrt math.h SC_NEED_M)
endif()

string(APPEND SC_CPP " -E")
set(SC_CPP \"${SC_CPP}\")

set(SC_CFLAGS "${CMAKE_C_FLAGS} ${MPI_C_COMPILE_OPTIONS}")
set(SC_CFLAGS \"${SC_CFLAGS}\")

set(SC_CPPFLAGS \"\")

set(SC_LDFLAGS \"${MPI_C_LINK_FLAGS}\")
set(SC_LIBS \"${ZLIB_LIBRARIES} m\")

set(SC_ENABLE_PTHREAD ${CMAKE_USE_PTHREADS_INIT})
set(SC_ENABLE_MEMALIGN 1)

if(MPI_FOUND)
  set(SC_ENABLE_MPI ${MPI_FOUND})
  check_symbol_exists(MPI_COMM_TYPE_SHARED mpi.h SC_ENABLE_MPICOMMSHARED)
  set(SC_ENABLE_MPIIO 1)
  check_symbol_exists(MPI_Init_thread mpi.h SC_ENABLE_MPITHREAD)
  check_symbol_exists(MPI_Win_allocate_shared mpi.h SC_ENABLE_MPIWINSHARED)
endif(MPI_FOUND)


check_symbol_exists(realloc stdlib.h SC_ENABLE_USE_REALLOC)

check_symbol_exists(aligned_alloc stdlib.h SC_HAVE_ALIGNED_ALLOC)
if(NOT SC_HAVE_ALIGNED_ALLOC)
  check_symbol_exists(_aligned_malloc malloc.h SC_HAVE_ALIGNED_MALLOC)
endif()

check_symbol_exists(backtrace execinfo.h SC_HAVE_BACKTRACE)
check_symbol_exists(backtrace_symbols execinfo.h SC_HAVE_BACKTRACE_SYMBOLS)

# check_include_file(dlfcn.h SC_HAVE_DLFCN_H)
check_include_file(execinfo.h SC_HAVE_EXECINFO_H)
check_symbol_exists(fsync unistd.h SC_HAVE_FSYNC)
# check_include_file(inttypes.h SC_HAVE_INTTYPES_H)
# check_include_file(memory.h SC_HAVE_MEMORY_H)

check_symbol_exists(posix_memalign stdlib.h SC_HAVE_POSIX_MEMALIGN)

# requires -D_GNU_SOURCE, missing on MinGW
# https://www.gnu.org/software/gnulib/manual/html_node/qsort_005fr.html
set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_symbol_exists(qsort_r stdlib.h SC_HAVE_QSORT_R)
set(CMAKE_REQUIRED_DEFINITIONS)

check_include_file(signal.h SC_HAVE_SIGNAL_H)
check_include_file(stdint.h SC_HAVE_STDINT_H)
check_include_file(stdlib.h SC_HAVE_STDLIB_H)

check_include_file(string.h SC_HAVE_STRING_H)
if(SC_HAVE_STDLIB_H)
  check_symbol_exists(random stdlib.h SC_HAVE_RANDOM)
  check_symbol_exists(srandom stdlib.h SC_HAVE_SRANDOM)
endif()

check_symbol_exists(strtoll stdlib.h SC_HAVE_STRTOLL)

check_include_file(sys/types.h SC_HAVE_SYS_TYPES_H)

check_include_file(sys/time.h SC_HAVE_SYS_TIME_H)
check_include_file(time.h SC_HAVE_TIME_H)
if(WIN32)
  # even though Windows has time.h, struct timeval is in Winsock2.h
  check_include_file(Winsock2.h SC_HAVE_WINSOCK2_H)
  set(WINSOCK_LIBRARIES wsock32 ws2_32 Iphlpapi)
endif()

check_include_file(unistd.h SC_HAVE_UNISTD_H)
if(SC_HAVE_UNISTD_H)
  check_include_file(getopt.h SC_HAVE_GETOPT_H)
endif()

if(ZLIB_FOUND)
  set(CMAKE_REQUIRED_LIBRARIES ZLIB::ZLIB)
  check_symbol_exists(adler32_combine zlib.h SC_HAVE_ZLIB)
endif()

# check_type_size(int SC_SIZEOF_INT BUILTIN_TYPES_ONLY)
check_type_size(long SC_SIZEOF_LONG BUILTIN_TYPES_ONLY)
# check_type_size("long long" SC_SIZEOF_LONG_LONG BUILTIN_TYPES_ONLY)
# check_type_size("unsigned long" SC_SIZEOF_UNSIGNED_LONG BUILTIN_TYPES_ONLY)
# check_type_size("unsigned long long" SC_SIZEOF_UNSIGNED_LONG_LONG BUILTIN_TYPES_ONLY)
set(SC_SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P})


configure_file(src/sc_config_cmake.h.in ${PROJECT_BINARY_DIR}/include/sc_config.h)

# --- sanity check of MPI sc_config.h

# check if libsc was configured properly
include(CheckSymbolExists)
set(CMAKE_REQUIRED_FLAGS)
set(CMAKE_REQUIRED_INCLUDES)
set(CMAKE_REQUIRED_LIBRARIES)

# libsc and current project must both be compiled with/without MPI
check_symbol_exists(SC_ENABLE_MPI ${PROJECT_BINARY_DIR}/include/sc_config.h SC_has_mpi)
check_symbol_exists(SC_ENABLE_MPIIO ${PROJECT_BINARY_DIR}/include/sc_config.h SC_has_mpi_io)

if(MPI_C_FOUND)
  # a sign the current project is using MPI
  if(NOT (SC_has_mpi AND SC_has_mpi_io))
    message(FATAL_ERROR "MPI used, but sc_config.h is not configured for MPI")
  endif()
else()
  if(SC_has_mpi OR SC_has_mpi_io)
    message(FATAL_ERROR "MPI not used, but sc_config.h is configured for MPI")
  endif()
endif()
