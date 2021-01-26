include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckTypeSize)

target_sources(sc PRIVATE sc.c sc_mpi.c sc_containers.c sc_avl.c sc_string.c sc_unique_counter.c sc_functions.c sc_statistics.c sc_ranges.c sc_io.c sc_amr.c sc_search.c sc_sort.c sc_flops.c sc_random.c sc_getopt.c sc_getopt1.c  sc_polynom.c sc_keyvalue.c sc_refcount.c sc_shmem.c sc_allgather.c sc_reduce.c sc_notify.c sc_uint128.c)
if(NOT MSVC AND NOT (WIN32 AND CMAKE_C_COMPILER_ID STREQUAL Clang))
  target_sources(sc PRIVATE sc_options.c)
endif()

# --- generate sc_config.h

set(CMAKE_REQUIRED_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_BINARY_DIR}/include)

if(ZLIB_FOUND)
  set(CMAKE_REQUIRED_LIBRARIES ZLIB::ZLIB)
endif()

if(MPI_FOUND)
  set(CMAKE_REQUIRED_LIBRARIES MPI::MPI_C ${CMAKE_REQUIRED_LIBRARIES})
  set(SC_CC \"${MPI_C_COMPILER}\")
  set(SC_CPP ${MPI_C_COMPILER})
else()
  set(SC_CC \"${CMAKE_C_COMPILER}\")
  set(SC_CPP ${CMAKE_C_COMPILER})
endif()

if(NOT WIN32)
  set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} m)
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
check_symbol_exists(backtrace execinfo.h SC_HAVE_BACKTRACE)
check_symbol_exists(backtrace_symbols execinfo.h SC_HAVE_BACKTRACE_SYMBOLS)

# check_include_file(dlfcn.h SC_HAVE_DLFCN_H)
check_include_file(execinfo.h SC_HAVE_EXECINFO_H)
check_symbol_exists(fsync unistd.h SC_HAVE_FSYNC)
# check_include_file(inttypes.h SC_HAVE_INTTYPES_H)
# check_include_file(memory.h SC_HAVE_MEMORY_H)

check_symbol_exists(posix_memalign stdlib.h SC_HAVE_POSIX_MEMALIGN)
check_symbol_exists(qsort_r stdlib.h SC_HAVE_QSORT_R)
check_include_file(signal.h SC_HAVE_SIGNAL_H)
check_include_file(stdint.h SC_HAVE_STDINT_H)
check_include_file(stdlib.h SC_HAVE_STDLIB_H)
# check_include_file(strings.h SC_HAVE_STRINGS_H)
check_include_file(string.h SC_HAVE_STRING_H)

# check_symbol_exists(strtol stdlib.h SC_HAVE_STRTOL)
check_symbol_exists(strtoll stdlib.h SC_HAVE_STRTOLL)

# check_include_file(sys/stat.h SC_HAVE_SYS_STAT_H)
check_include_file(sys/time.h SC_HAVE_SYS_TIME_H)
check_include_file(sys/types.h SC_HAVE_SYS_TYPES_H)
check_include_file(times.h SC_HAVE_TIME_H)

check_include_file(unistd.h SC_HAVE_UNISTD_H)
check_symbol_exists(adler32_combine zlib.h SC_HAVE_ZLIB)



# check_type_size(int SC_SIZEOF_INT BUILTIN_TYPES_ONLY)
check_type_size(long SC_SIZEOF_LONG BUILTIN_TYPES_ONLY)
# check_type_size("long long" SC_SIZEOF_LONG_LONG BUILTIN_TYPES_ONLY)
# check_type_size("unsigned long" SC_SIZEOF_UNSIGNED_LONG BUILTIN_TYPES_ONLY)
# check_type_size("unsigned long long" SC_SIZEOF_UNSIGNED_LONG_LONG BUILTIN_TYPES_ONLY)
set(SC_SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P})


configure_file(sc_config.h.in ${PROJECT_BINARY_DIR}/include/sc_config.h)
