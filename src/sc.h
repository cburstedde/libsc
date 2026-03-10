/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System
  Additional copyright (C) 2011 individual authors

  The SC Library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with the SC Library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

/** \file sc.h
 *
 * Support for process management (memory allocation, logging, etc.)
 */

/** \defgroup sc The sc Library
 *
 * The SC Library provides support for parallel scientific applications.
 */

#ifndef SC_H
#define SC_H

/* we set the GNU feature test macro before including anything */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* include the libsc config header first */
#include <sc_config.h>
#ifndef _sc_const
/** Portable way to work with really old compilers without const. */
#define _sc_const const
#endif
#ifndef _sc_restrict
/** Portable way to work with really old compilers without restrict. */
#define _sc_restrict restrict
#endif

/** Test for gcc version without features.h. */
#define SC_CALC_VERSION(major,minor,patchlevel) \
                       (((major) * 1000 + (minor)) * 1000 + (patchlevel))
#ifdef __GNUC__
#define SC_GCC_VERSION \
        SC_CALC_VERSION(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#else
/** Assemble GCC version using a 1000-per-digit scheme. */
#define SC_GCC_VERSION \
        SC_CALC_VERSION (0, 0, 0)
#endif

/* use this feature macro, be minimally invasive */
#ifdef SC_ENABLE_MEMALIGN
/* we disable the system-provided functions for the time being */
#ifdef SC_HAVE_ANY_MEMALIGN
#undef SC_HAVE_ANY_MEMALIGN
#endif
/* if system-provided functions are needed, give them the prototype */
#ifdef SC_HAVE_ANY_MEMALIGN
#ifndef SC_HAVE_POSIX_MEMALIGN
#ifdef SC_HAVE_ALIGNED_ALLOC
#define _ISOC11_SOURCE
#endif
#endif
#endif
/* done with memalign macros */
#endif

/* disable global counters that are not thread-safe (say when using TBB) */
#ifndef SC_ENABLE_USE_COUNTERS
#define SC_NOCOUNT_MALLOC
#define SC_NOCOUNT_REFCOUNT
#define SC_NOCOUNT_LOGINDENT
#endif

/* implement the default visibility attribute */

#if defined _WIN32 || defined __CYGWIN__
#if 0
  /* this is currently not properly tested */
  #ifdef BUILDING_DLL
    #ifdef __GNUC__
      #define SC_DLL_PUBLIC __attribute__ ((dllexport))
    #else
      #define SC_DLL_PUBLIC __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
      #define SC_DLL_PUBLIC __attribute__ ((dllimport))
    #else
      #define SC_DLL_PUBLIC __declspec(dllimport)
    #endif
  #endif
#else
  /* while disabling the above definitions */
  #define SC_DLL_PUBLIC
#endif
#else
  #if __GNUC__ >= 4
    #define SC_DLL_PUBLIC __attribute__ ((visibility ("default")))
  #else
    #define SC_DLL_PUBLIC
  #endif
#endif

/* use this in case mpi.h includes stdint.h */

#ifndef __STDC_LIMIT_MACROS
/** Activate C99 limit macros for older C++ compilers. */
#define __STDC_LIMIT_MACROS
#endif
#ifndef __STDC_CONSTANT_MACROS
/** Activate C99 constant macros for older C++ compilers. */
#define __STDC_CONSTANT_MACROS
#endif

/** ​Platform-specific noreturn and printf format string checks​​ */
#ifdef _MSC_VER
#include <sal.h> /* _Printf_format_string_ */
#define SC_NORETURN __declspec(noreturn)
#define SC_PRINTF_LIKE(fmt_idx, first_arg)
#define SC_PRINTF_FMT _Printf_format_string_
#else
#define SC_NORETURN __attribute__((noreturn))
#define SC_PRINTF_LIKE(fmt_idx, first_arg) __attribute__((format(printf, fmt_idx, first_arg)))
#define SC_PRINTF_FMT
#endif

/* include MPI before stdio.h */

#ifdef SC_ENABLE_MPI
#include <mpi.h>
#else
#ifdef MPI_SUCCESS
#error "mpi.h is included.  Use --enable-mpi."
#endif
#endif

/* include system headers */

#define _USE_MATH_DEFINES
#include <math.h>
#ifndef M_E
#define M_E 2.71828182845904523536
#endif
#ifndef M_LOG2E
#define M_LOG2E 1.44269504088896340736
#endif
#ifndef M_LOG10E
#define M_LOG10E 0.434294481903251827651
#endif
#ifndef M_LN2
#define M_LN2 0.693147180559945309417
#endif
#ifndef M_LN10
#define M_LN10 2.30258509299404568402
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#ifndef M_PI_4
#define M_PI_4 0.785398163397448309616
#endif
#ifndef M_1_PI
#define M_1_PI 0.318309886183790671538
#endif
#ifndef M_2_PI
#define M_2_PI 0.636619772367581343076
#endif
#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.12837916709551257390
#endif
#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2 0.707106781186547524401
#endif
#include <ctype.h>
#include <float.h>
#ifdef SC_HAVE_LIBGEN_H
#include <libgen.h>
#endif
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#ifdef SC_HAVE_STDINT_H
#include <stdint.h>
#endif
#include <stdio.h>
#ifdef SC_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef SC_HAVE_STRING_H
#include <string.h>
#endif
#ifdef SC_HAVE_SYS_TIME_H
#include <sys/time.h>
#elif defined(_MSC_VER) && !defined(SC_HAVE_GETTIMEOFDAY)
#define WIN32_LEAN_AND_MEAN
#include <Winsock2.h>
struct timezone
{
  int                 tz_minuteswest;
  int                 tz_dsttime;
};
int gettimeofday (struct timeval*, struct timezone*);
#endif
#ifdef SC_HAVE_UNISTD_H
#include <unistd.h>
#elif defined _WIN32
#include <BaseTsd.h>
typedef SSIZE_T     ssize_t;
#endif

/* definitions to allow user code to query the sc library */
/** Indicate that we do not modify the communicator in sc_init. */
#define SC_INIT_COMM_CLEAN

/* provide extern C defines */

/* The hacks below enable semicolons after the SC_EXTERN_C_ macros
 * and also take care of the different semantics of () / (...) */
#ifdef __cplusplus
#define SC_EXTERN_C_BEGIN       extern "C" { void sc_extern_c_hack_1 (void)
#define SC_EXTERN_C_END                    } extern "C" void sc_extern_c_hack_2 (void)
#define SC_NOARGS               ...
#else
#define SC_EXTERN_C_BEGIN                    void sc_extern_c_hack_1 (void)
#define SC_EXTERN_C_END                      void sc_extern_c_hack_2 (void)
/** For compatibility of varargs with C++ */
#define SC_NOARGS
#endif

/* this header is always included */
#include <sc_mpi.h>

SC_EXTERN_C_BEGIN;

/* extern variables */

/** Lookup table to provide fast base-2 logarithm of integers. */
extern const int    sc_log2_lookup_table[256];

/** libsc allows for multiple packages to use their own log priorities etc.
 * Logging priorities, callbacks, and memory balance counters go by package.
 * This is the package id for core sc functions and is meant to be read only.
 * The variable starts out with a value of -1, which is fine by itself.
 * It is set to a non-negative value by the (optional) \ref sc_init.
 * Calling the (also optional) \ref sc_finalize resets it to -1.  There is
 * no need to access this variable directly; use \ref sc_get_package_id.
 */
extern SC_DLL_PUBLIC int sc_package_id;

/** Optional trace file for logging (see \ref sc_init).
 * Initialized to NULL. */
extern SC_DLL_PUBLIC FILE *sc_trace_file;

/** Optional minimum log priority for messages that go into the trace file. */
extern SC_DLL_PUBLIC int sc_trace_prio;

/** Define machine epsilon for the double type. */
#define SC_EPS               2.220446049250313e-16

/** Define 1000 times the machine epsilon for the double type. */
#define SC_1000_EPS (1000. * 2.220446049250313e-16)

/* check macros, always enabled */

/** A macro to do and return nothing as an expression. */
#define SC_NOOP() ((void) (0))
#define SC_ABORT(s)                             \
  sc_abort_verbose (__FILE__, __LINE__, (s))
#define SC_ABORT_NOT_REACHED() SC_ABORT ("Unreachable code")
#define SC_CHECK_ABORT(q,s)                     \
  ((q) ? (void) 0 : SC_ABORT (s))
#define SC_CHECK_MPI(r) SC_CHECK_ABORT ((r) == sc_MPI_SUCCESS, "MPI error")

/*
 * C++98 does not allow variadic macros
 * 1. Declare a default variadic function for C and C++
 * 2. Use macros in C instead of the function
 * This loses __FILE__ and __LINE__ in the C++ ..F log functions
 */
SC_NORETURN
void                SC_ABORTF (SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
void                SC_CHECK_ABORTF (int success, SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(2, 3);
#ifndef __cplusplus
#define SC_ABORTF(fmt,...)                                      \
  sc_abort_verbosef (__FILE__, __LINE__, (fmt), __VA_ARGS__)
#define SC_CHECK_ABORTF(q,fmt,...)                      \
  ((q) ? (void) 0 : SC_ABORTF (fmt, __VA_ARGS__))
#endif
#define SC_ABORT1(fmt,a)                                \
  sc_abort_verbosef (__FILE__, __LINE__, (fmt), (a))
#define SC_ABORT2(fmt,a,b)                                      \
  sc_abort_verbosef (__FILE__, __LINE__, (fmt), (a), (b))
#define SC_ABORT3(fmt,a,b,c)                                    \
  sc_abort_verbosef (__FILE__, __LINE__, (fmt), (a), (b), (c))
#define SC_ABORT4(fmt,a,b,c,d)                                          \
  sc_abort_verbosef (__FILE__, __LINE__, (fmt), (a), (b), (c), (d))
#define SC_ABORT5(fmt,a,b,c,d,e)                                        \
  sc_abort_verbosef (__FILE__, __LINE__, (fmt), (a), (b), (c), (d), (e))
#define SC_ABORT6(fmt,a,b,c,d,e,f)                                      \
  sc_abort_verbosef (__FILE__, __LINE__, (fmt), (a), (b), (c), (d), (e), (f))
#define SC_CHECK_ABORT1(q,fmt,a)                \
  ((q) ? (void) 0 : SC_ABORT1 ((fmt), (a)))
#define SC_CHECK_ABORT2(q,fmt,a,b)                      \
  ((q) ? (void) 0 : SC_ABORT2 ((fmt), (a), (b)))
#define SC_CHECK_ABORT3(q,fmt,a,b,c)                    \
  ((q) ? (void) 0 : SC_ABORT3 ((fmt), (a), (b), (c)))
#define SC_CHECK_ABORT4(q,fmt,a,b,c,d)                          \
  ((q) ? (void) 0 : SC_ABORT4 ((fmt), (a), (b), (c), (d)))
#define SC_CHECK_ABORT5(q,fmt,a,b,c,d,e)                        \
  ((q) ? (void) 0 : SC_ABORT5 ((fmt), (a), (b), (c), (d), (e)))
#define SC_CHECK_ABORT6(q,fmt,a,b,c,d,e,f)                              \
  ((q) ? (void) 0 : SC_ABORT6 ((fmt), (a), (b), (c), (d), (e), (f)))

/* assertions, only enabled in debug mode */

#ifdef SC_ENABLE_DEBUG
#define SC_ASSERT(c) SC_CHECK_ABORT ((c), "Assertion '" #c "'")
#define SC_EXECUTE_ASSERT_FALSE(expression)                             \
  do { int _sc_i = (int) (expression);                                  \
       SC_CHECK_ABORT (!_sc_i, "Expected false: '" #expression "'");    \
  } while (0)
#define SC_EXECUTE_ASSERT_TRUE(expression)                              \
  do { int _sc_i = (int) (expression);                                  \
       SC_CHECK_ABORT (_sc_i, "Expected true: '" #expression "'");      \
  } while (0)
#else
#define SC_ASSERT(c) SC_NOOP ()
#define SC_EXECUTE_ASSERT_FALSE(expression) \
  do { (void) (expression); } while (0)
#define SC_EXECUTE_ASSERT_TRUE(expression) \
  do { (void) (expression); } while (0)
#endif

/* macros for memory allocation, will abort if out of memory */

#define SC_ALLOC(t,n)         (t *) sc_malloc (sc_package_id, (n) * sizeof(t))
#define SC_ALLOC_ZERO(t,n)    (t *) sc_calloc (sc_package_id, \
                                               (size_t) (n), sizeof(t))
#define SC_REALLOC(p,t,n)     (t *) sc_realloc (sc_package_id,          \
                                             (p), (n) * sizeof(t))
#define SC_STRDUP(s)                sc_strdup (sc_package_id, (s))
#define SC_FREE(p)                  sc_free (sc_package_id, (p))

/* macros for memory alignment */
/* some copied from bfam: https://github.com/bfam/bfam */

#define SC_ALIGN_UP(x,n) ( ((n) <= 0) ? (x) : ((x) + (n) - 1) / (n) * (n) )

#if defined (__bgq__)
#define SC_ARG_ALIGN(p,t,n) __alignx((n), (p))
#elif defined (__ICC)
#define SC_ARG_ALIGN(p,t,n) __assume_aligned((p), (n))
#elif defined (__clang__)
#define SC_ARG_ALIGN(p,t,n) SC_NOOP ()
#elif defined (__GNUC__) || defined (__GNUG__)

#if SC_GCC_VERSION >= SC_CALC_VERSION (4, 7, 0)
#define SC_ARG_ALIGN(p,t,n) do {                              \
  (p) = (t) __builtin_assume_aligned((void *) (p), (n));      \
} while (0)
#else
#define SC_ARG_ALIGN(p,t,n) SC_NOOP ()
#endif

#else
#define SC_ARG_ALIGN(p,t,n) SC_NOOP ()
#endif

#if (defined __GNUC__) || (defined __PGI) || (defined __IBMC__)
#define SC_ATTR_ALIGN(n) __attribute__ ((aligned(n)))
#else
#define SC_ATTR_ALIGN(n)
#endif

/**
 * Sets n elements of a memory range to zero.
 * Assumes the pointer p is of the correct type.
 */
#define SC_BZERO(p,n) ((void) memset ((p), 0, (n) * sizeof (*(p))))

/* min, max and square helper macros */

#define SC_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define SC_MAX(a,b) (((a) > (b)) ? (a) : (b))
#define SC_SQR(a) ((a) * (a))

/* hopefully fast binary logarithms and binary round up */

#define SC_LOG2_8(x) (sc_log2_lookup_table[(x)])
#define SC_LOG2_16(x) (((x) > 0xff) ?                                   \
                       (SC_LOG2_8 ((x) >> 8) + 8) : SC_LOG2_8 (x))
#define SC_LOG2_32(x) (((x) > 0xffff) ?                                 \
                       (SC_LOG2_16 ((x) >> 16)) + 16 : SC_LOG2_16 (x))
#define SC_LOG2_64(x) (((x) > 0xffffffffLL) ?                           \
                       (SC_LOG2_32 ((x) >> 32)) + 32 : SC_LOG2_32 (x))
#define SC_ROUNDUP2_32(x)                               \
  (((x) <= 0) ? 0 : (1 << (SC_LOG2_32 ((x) - 1) + 1)))
#define SC_ROUNDUP2_64(x)                               \
  (((x) <= 0) ? 0 : (1LL << (SC_LOG2_64 ((x) - 1LL) + 1)))

/* log categories */

#define SC_LC_GLOBAL      1     /**< log only for master process */
#define SC_LC_NORMAL      2     /**< log for every process */

/** \defgroup sc_logprios Log Priorities
 *
 * The log level is a number designating the priority of a log action.
 *
 * Log levels serve both to indicate the priority of a message and as a
 * filter passed to functions to determine which priorities to log.
 *
 * The priorities \ref SC_LP_TRACE to \ref SC_LP_VERBOSE may be used
 * profusely and are appropriate when all parallel processes contribute
 * low-priority, informative log messages.
 *
 *  \ref SC_LP_INFO and above should not clutter the output of large
 *  parallel runs.  \ref SC_LP_STATISTICS is suggested for important
 *  measurements.  \ref SC_LP_PRODUCTION is meant for rudimentary
 *  information on the program flow.
 *
 * \ref SC_LP_ESSENTIAL can be used for one-time messages, say at program
 * startup to communicate program version or licence information.
 *
 * \ref SC_LP_ERROR shall never be used during clean operation.
 * Thus, it can be used to indicate abnormal behavior of the program
 * and will not disturb the silent appearance of a clean run.
 * Recommended for those who generally prefer to see absolutely no output
 * from the libraries they use.
 *
 * \ingroup sc
 *
 * @{ */
#define SC_LP_DEFAULT   (-1)    /**< Selects the SC default threshold. */
#define SC_LP_ALWAYS      0     /**< Log absolutely everything. */
#define SC_LP_TRACE       1     /**< Prefix file and line number. */
#define SC_LP_DEBUG       2     /**< Any information on the internal state. */
#define SC_LP_VERBOSE     3     /**< Information on conditions, decisions. */
#define SC_LP_INFO        4     /**< Most relevant things a function is doing. */
#define SC_LP_STATISTICS  5     /**< Important for consistency/performance. */
#define SC_LP_PRODUCTION  6     /**< A few lines at most for a major api function. */
#define SC_LP_ESSENTIAL   7     /**< Log a few lines max (version info) per program. */
#define SC_LP_ERROR       8     /**< Log errors only.  This is suggested over \ref SC_LP_SILENT. */
#define SC_LP_SILENT      9     /**< Never log anything.  Instead suggesting \ref SC_LP_ERROR. */
/** @} */

/* The default log priority may be overridded by this preprocessor define. */
#ifdef SC_LOG_PRIORITY
#define SC_LP_THRESHOLD SC_LOG_PRIORITY
#define SC_LP_APPLICATION SC_LOG_PRIORITY
#else
#ifdef SC_ENABLE_DEBUG
#define SC_LP_THRESHOLD SC_LP_TRACE
#define SC_LP_APPLICATION SC_LP_DEBUG
#else
/** The log threshold chosen by \ref SC_LP_DEFAULT. */
#define SC_LP_THRESHOLD SC_LP_INFO
/** This threshold is intended for applications with conservative logging. */
#define SC_LP_APPLICATION SC_LP_STATISTICS
#endif
#endif

/* generic log macros */
#define SC_GEN_LOG(package,category,priority,s)                         \
  ((priority) < SC_LP_THRESHOLD ? (void) 0 :                            \
   sc_log (__FILE__, __LINE__, (package), (category), (priority), (s)))
#define SC_GLOBAL_LOG(p,s) SC_GEN_LOG (sc_package_id, SC_LC_GLOBAL, (p), (s))
#define SC_LOG(p,s) SC_GEN_LOG (sc_package_id, SC_LC_NORMAL, (p), (s))
void                SC_GEN_LOGF (int package, int category, int priority,
                                 SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(4, 5);
void                SC_GLOBAL_LOGF (int priority, SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(2, 3);
void                SC_LOGF (int priority, SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(2, 3);
#ifndef __cplusplus
#define SC_GEN_LOGF(package,category,priority,fmt,...)                  \
  ((priority) < SC_LP_THRESHOLD ? (void) 0 :                            \
   sc_logf (__FILE__, __LINE__, (package), (category), (priority),      \
            (fmt), __VA_ARGS__))
#define SC_GLOBAL_LOGF(p,fmt,...)                                       \
  SC_GEN_LOGF (sc_package_id, SC_LC_GLOBAL, (p), (fmt), __VA_ARGS__)
#define SC_LOGF(p,fmt,...)                                              \
  SC_GEN_LOGF (sc_package_id, SC_LC_NORMAL, (p), (fmt), __VA_ARGS__)
#endif

/* convenience global log macros will only output if identifier <= 0 */
#define SC_GLOBAL_TRACE(s) SC_GLOBAL_LOG (SC_LP_TRACE, (s))
#define SC_GLOBAL_LDEBUG(s) SC_GLOBAL_LOG (SC_LP_DEBUG, (s))
#define SC_GLOBAL_VERBOSE(s) SC_GLOBAL_LOG (SC_LP_VERBOSE, (s))
#define SC_GLOBAL_INFO(s) SC_GLOBAL_LOG (SC_LP_INFO, (s))
#define SC_GLOBAL_STATISTICS(s) SC_GLOBAL_LOG (SC_LP_STATISTICS, (s))
#define SC_GLOBAL_PRODUCTION(s) SC_GLOBAL_LOG (SC_LP_PRODUCTION, (s))
#define SC_GLOBAL_ESSENTIAL(s) SC_GLOBAL_LOG (SC_LP_ESSENTIAL, (s))
#define SC_GLOBAL_LERROR(s) SC_GLOBAL_LOG (SC_LP_ERROR, (s))
void                SC_GLOBAL_TRACEF (SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
void                SC_GLOBAL_LDEBUGF (SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
void                SC_GLOBAL_VERBOSEF (SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
void                SC_GLOBAL_INFOF (SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
void                SC_GLOBAL_STATISTICSF (SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
void                SC_GLOBAL_PRODUCTIONF (const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
void                SC_GLOBAL_ESSENTIALF (SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
void                SC_GLOBAL_LERRORF (SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
#ifndef __cplusplus
#define SC_GLOBAL_TRACEF(fmt,...)                       \
  SC_GLOBAL_LOGF (SC_LP_TRACE, (fmt), __VA_ARGS__)
#define SC_GLOBAL_LDEBUGF(fmt,...)                      \
  SC_GLOBAL_LOGF (SC_LP_DEBUG, (fmt), __VA_ARGS__)
#define SC_GLOBAL_VERBOSEF(fmt,...)                     \
  SC_GLOBAL_LOGF (SC_LP_VERBOSE, (fmt), __VA_ARGS__)
#define SC_GLOBAL_INFOF(fmt,...)                        \
  SC_GLOBAL_LOGF (SC_LP_INFO, (fmt), __VA_ARGS__)
#define SC_GLOBAL_STATISTICSF(fmt,...)                  \
  SC_GLOBAL_LOGF (SC_LP_STATISTICS, (fmt), __VA_ARGS__)
#define SC_GLOBAL_PRODUCTIONF(fmt,...)                  \
  SC_GLOBAL_LOGF (SC_LP_PRODUCTION, (fmt), __VA_ARGS__)
#define SC_GLOBAL_ESSENTIALF(fmt,...)                   \
  SC_GLOBAL_LOGF (SC_LP_ESSENTIAL, (fmt), __VA_ARGS__)
#define SC_GLOBAL_LERRORF(fmt,...)                      \
  SC_GLOBAL_LOGF (SC_LP_ERROR, (fmt), __VA_ARGS__)
#endif

/* convenience log macros that output regardless of identifier */
#define SC_TRACE(s) SC_LOG (SC_LP_TRACE, (s))
#define SC_LDEBUG(s) SC_LOG (SC_LP_DEBUG, (s))
#define SC_VERBOSE(s) SC_LOG (SC_LP_VERBOSE, (s))
#define SC_INFO(s) SC_LOG (SC_LP_INFO, (s))
#define SC_STATISTICS(s) SC_LOG (SC_LP_STATISTICS, (s))
#define SC_PRODUCTION(s) SC_LOG (SC_LP_PRODUCTION, (s))
#define SC_ESSENTIAL(s) SC_LOG (SC_LP_ESSENTIAL, (s))
#define SC_LERROR(s) SC_LOG (SC_LP_ERROR, (s))
void                SC_TRACEF (SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
void                SC_LDEBUGF (SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
void                SC_VERBOSEF (SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
void                SC_INFOF (SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
void                SC_STATISTICSF (SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
void                SC_PRODUCTIONF (SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
void                SC_ESSENTIALF (SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
void                SC_LERRORF (SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(1, 2);
#ifndef __cplusplus
#define SC_TRACEF(fmt,...)                      \
  SC_LOGF (SC_LP_TRACE, (fmt), __VA_ARGS__)
#define SC_LDEBUGF(fmt,...)                     \
  SC_LOGF (SC_LP_DEBUG, (fmt), __VA_ARGS__)
#define SC_VERBOSEF(fmt,...)                    \
  SC_LOGF (SC_LP_VERBOSE, (fmt), __VA_ARGS__)
#define SC_INFOF(fmt,...)                       \
  SC_LOGF (SC_LP_INFO, (fmt), __VA_ARGS__)
#define SC_STATISTICSF(fmt,...)                         \
  SC_LOGF (SC_LP_STATISTICS, (fmt), __VA_ARGS__)
#define SC_PRODUCTIONF(fmt,...)                         \
  SC_LOGF (SC_LP_PRODUCTION, (fmt), __VA_ARGS__)
#define SC_ESSENTIALF(fmt,...)                  \
  SC_LOGF (SC_LP_ESSENTIAL, (fmt), __VA_ARGS__)
#define SC_LERRORF(fmt,...)                     \
  SC_LOGF (SC_LP_ERROR, (fmt), __VA_ARGS__)
#endif

/** Macros used to convert a macro definition such as the point version
 * or some other numerical literal to a string. */
#define _SC_TOSTRING(x) #x

/** Macros used to convert a macro definition such as the point version
 * or some other numerical literal to a string. */
#define SC_TOSTRING(x) _SC_TOSTRING(x)

/** Type of the log handler function. */
typedef void        (*sc_log_handler_t) (FILE * log_stream,
                                         const char *filename, int lineno,
                                         int package, int category,
                                         int priority, const char *msg);

/** Type of the abort handler function. */
typedef void        (*sc_abort_handler_t) (void);

/* memory allocation functions, will abort if out of memory */

void               *sc_malloc (int package, size_t size);
void               *sc_calloc (int package, size_t nmemb, size_t size);
void               *sc_realloc (int package, void *ptr, size_t size);
char               *sc_strdup (int package, const char *s);
void                sc_free (int package, void *ptr);
int                 sc_memory_status (int package);
void                sc_memory_check (int package);

/** Return error count or zero if all is ok. */
int                 sc_memory_check_noerr (int package);

/* comparison functions for various integer sizes */

int                 sc_int_compare (const void *v1, const void *v2);
int                 sc_int8_compare (const void *v1, const void *v2);
int                 sc_int16_compare (const void *v1, const void *v2);
int                 sc_int32_compare (const void *v1, const void *v2);
int                 sc_int64_compare (const void *v1, const void *v2);
int                 sc_double_compare (const void *v1, const void *v2);

/** Safe version of the standard library atoi (3) function.
 * \param [in] nptr     NUL-terminated string.
 * \return              Converted integer value.  0 if no valid number.
 *                      INT_MAX on overflow, INT_MIN on underflow.
 */
int                 sc_atoi (const char *nptr);

/** Safe version of the standard library atol (3) function.
 * \param [in] nptr     NUL-terminated string.
 * \return              Converted long value.  0 if no valid number.
 *                      LONG_MAX on overflow, LONG_MIN on underflow.
 */
long                sc_atol (const char *nptr);

/** Controls the default SC log behavior.
 * \param [in] log_stream    Set stream to use by sc_logf (or NULL for stdout).
 * \param [in] log_handler   Set default SC log handler (NULL selects builtin).
 * \param [in] log_threshold Set default SC log threshold (or SC_LP_DEFAULT).
 *                           May be SC_LP_ALWAYS or SC_LP_SILENT.
 */
void                sc_set_log_defaults (FILE * log_stream,
                                         sc_log_handler_t log_handler,
                                         int log_threshold);

/** Set the default SC abort behavior.
 * \param [in] abort_handler Set default SC above handler (NULL selects
 *                           builtin).  If it returns, we abort (2) then.
 */
void                sc_set_abort_handler (sc_abort_handler_t abort_handler);

/** The central log function to be called by all packages.
 * Dispatches the log calls by package and filters by category and priority.
 * \param [in] filename  Usually used with a __FILE__ argument.
 * \param [in] lineno    Usually used with a __LINE__ argument.
 * \param [in] package   Must be a registered package id or -1.
 * \param [in] category  Must be SC_LC_NORMAL or SC_LC_GLOBAL.
 * \param [in] priority  Must be > SC_LP_ALWAYS and < SC_LP_SILENT.
 * \param [in] msg       Nul-terminated string to print.
 */
void                sc_log (const char *filename, int lineno,
                            int package, int category, int priority,
                            const char *msg);

/** The printf-style log function to be called by all packages.
 * Dispatches the log calls by package and filters by category and priority.
 * \param [in] filename  Usually used with a __FILE__ argument.
 * \param [in] lineno    Usually used with a __LINE__ argument.
 * \param [in] package   Must be a registered package id or -1.
 * \param [in] category  Must be SC_LC_NORMAL or SC_LC_GLOBAL.
 * \param [in] priority  Must be > SC_LP_ALWAYS and < SC_LP_SILENT.
 * \param [in] fmt       String of printf convention to log.
 */
void                sc_logf (const char *filename, int lineno,
                             int package, int category, int priority,
                             SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(6, 7);

/** The vprintf-style log function to be called by all packages.
 * Dispatches the log calls by package and filters by category and priority.
 * \param [in] filename  Usually used with a __FILE__ argument.
 * \param [in] lineno    Usually used with a __LINE__ argument.
 * \param [in] package   Must be a registered package id or -1.
 * \param [in] category  Must be SC_LC_NORMAL or SC_LC_GLOBAL.
 * \param [in] priority  Must be > SC_LP_ALWAYS and < SC_LP_SILENT.
 * \param [in] fmt       String of vprintf convention to log.
 * \param [in] ap        Must be initialized by va_start.
 */
void                sc_logv (const char *filename, int lineno,
                             int package, int category, int priority,
                             const char *fmt, va_list ap);

/** Add spaces to the start of a package's default log format. */
void                sc_log_indent_push_count (int package, int count);

/** Remove spaces from the start of a package's default log format. */
void                sc_log_indent_pop_count (int package, int count);

/** Add one space to the start of sc's default log format. */
void                sc_log_indent_push (void);

/** Remove one space from the start of a sc's default log format. */
void                sc_log_indent_pop (void);

/** Print a stack trace, call the abort handler and then call abort (). */
SC_NORETURN
void                sc_abort (void);

/** Print a message to stderr and then call sc_abort (). */
SC_NORETURN
void                sc_abort_verbose (const char *filename, int lineno,
                                      const char *msg);

/** Print a message to stderr and then call sc_abort (). */
SC_NORETURN
void                sc_abort_verbosef (const char *filename, int lineno,
                                       SC_PRINTF_FMT const char *fmt, ...)
  SC_PRINTF_LIKE(3, 4);

/** Print a message to stderr and then call sc_abort (). */
SC_NORETURN
void                sc_abort_verbosev (const char *filename, int lineno,
                                       const char *fmt, va_list ap);

/** Collective abort where only root prints a message */
SC_NORETURN
void                sc_abort_collective (const char *msg);

/** Register a software package with SC.
 * This function must only be called before additional threads are created.
 * The logging parameters are as in sc_set_log_defaults.
 * \return                   Returns a unique package id.
 */
int                 sc_package_register (sc_log_handler_t log_handler,
                                         int log_threshold,
                                         const char *name, const char *full);

/** Query whether an identifier matches a registered package.
 * \param [in] package_id       Only a non-negative id can be registered.
 * \return                      True if and only if the package id is
 *                              non-negative and package is registered.
 */
int                 sc_package_is_registered (int package_id);

/** Acquire a pthread mutex lock.
 * If configured without --enable-pthread, this function does nothing.
 * This function must be followed with a matching \ref sc_package_unlock.
 * \param [in] package_id       Either -1 for an undefined package or
 *                              an id returned from \ref sc_package_register.
 *                              Depending on the value, the appropriate mutex
 *                              is chosen.  Thus, we may overlap locking calls
 *                              with distinct package_id.
 */
void                sc_package_lock (int package_id);

/** Release a pthread mutex lock.
 * If configured without --enable-pthread, this function does nothing.
 * This function must be follow a matching \ref sc_package_lock.
 * \param [in] package_id       Either -1 for an undefined package or
 *                              an id returned from \ref sc_package_register.
 *                              Depending on the value, the appropriate mutex
 *                              is chosen.  Thus, we may overlap locking calls
 *                              with distinct package_id.
 */
void                sc_package_unlock (int package_id);

/** Set the logging verbosity of a registered package.
 * This can be called at any point in the program, any number of times.
 * It can only lower the verbosity at and below the value of SC_LP_THRESHOLD.
 * \param [in] package_id       Must be a registered package identifier.
 * \param [in] log_priority     The minimum priority required to output.
 */
void                sc_package_set_verbosity (int package_id,
                                              int log_priority);

/** Set the unregister behavior of sc_package_unregister().
 *
 * \param[in] package_id    Must be -1 for the default package or
 *                          the identifier of a registered package.
 * \param[in] set_abort     True if sc_package_unregister() should abort if the
 *                          number of allocs does not match the number of
 *                          frees; false otherwise.
 */
void                sc_package_set_abort_alloc_mismatch (int package_id,
                                                         int set_abort);

/** Unregister a software package with SC.
 * This function must only be called after additional threads are finished.
 */
void                sc_package_unregister (int package_id);

/** Print a summary of all packages registered with SC.
 * Uses the SC_LC_GLOBAL log category which by default only prints on rank 0.
 * \param [in] log_priority     Priority passed to sc log functions.
 */
void                sc_package_print_summary (int log_priority);

/** Sets the global program identifier (e.g. the MPI rank) and some flags.
 * This function is optional.
 * This function must only be called before additional threads are created.
 * If this function is not called or called with log_handler == NULL,
 * the default SC log handler will be used.
 * If this function is not called or called with log_threshold == SC_LP_DEFAULT,
 * the default SC log threshold will be used.
 * The default SC log settings can be changed with sc_set_log_defaults ().
 * \param [in] mpicomm          MPI communicator, can be sc_MPI_COMM_NULL.
 *                              If sc_MPI_COMM_NULL, the identifier is set to -1.
 *                              Otherwise, sc_MPI_Init must have been called.
 *                              Effectively, we just query size and rank.
 * \param [in] catch_signals    If true, signals INT and SEGV are caught.
 * \param [in] print_backtrace  If true, sc_abort prints a backtrace.
 */
void                sc_init (sc_MPI_Comm mpicomm,
                             int catch_signals, int print_backtrace,
                             sc_log_handler_t log_handler, int log_threshold);

/** Return whether SC has been initialized or not.
 * \return          True if libsc has been initialized with a call to
 *                  \ref sc_init and false otherwise.
 *                  After \ref sc_finalize the result resets to false.
 * \note            This routine is not thread-safe.
 */
int                 sc_is_initialized (void);

/** Query SC's own package identity.
 * \return          This is -1 before \ref sc_init has been called
 *                  and a proper package identifier (>= 0) afterwards.
 *                  After \ref sc_finalize the identifier resets to -1.
 * \note            This routine is not thread-safe.
 */
int                 sc_get_package_id (void);

/** Unregisters all packages, runs the memory check, removes the
 * signal handlers and resets sc_identifier and sc_root_*.
 * This function aborts on any inconsistency found unless
 * the global variable default_abort_mismatch is false.
 * Function is optional if memory cleanliness is no concern.
 * This function does not require sc_init to be called first.
 * In any case it makes \ref sc_is_initialized return false.
 */
void                sc_finalize (void);

/** Unregisters all packages, runs the memory check, removes the
 * signal handlers and resets sc_identifier and sc_root_*.
 * This function never aborts but returns the number of errors encountered.
 * Function is optional if memory cleanliness is no concern.
 * This function does not require sc_init to be called first.
 * In any case it makes \ref sc_is_initialized return false.
 * \return          0 when everything is consistent, nonzero otherwise.
 */
int                 sc_finalize_noabort (void);

/** Identify the root process.
 * Only meaningful between sc_init and sc_finalize and
 * with a communicator that is not sc_MPI_COMM_NULL (otherwise always true).
 *
 * \return          Return true for the root process and false otherwise.
 */
int                 sc_is_root (void);

/** Provide a string copy function.
 * \param [out] dest    Buffer of length at least \a size.
 *                      On output, not touched if NULL or \a size == 0.
 *                      Otherwise, \a src is copied to \a dest and
 *                      \a dest is padded with '\0' from the right
 *                      if strlen (src) < size - 1.
 * \param [in] size     Allocation length of \a dest.
 * \param [in] src      Null-terminated string.
 * \return              Equivalent to \ref
 *                      sc_snprintf (dest, size, "%s", src).
 */
void                sc_strcopy (char *dest, size_t size, const char *src);

/** Wrap the system snprintf function, allowing for truncation.
 * The snprintf function may truncate the string written to the specified length.
 * In some cases, compilers warn when this may occur.
 * Here this is permitted behavior and we avoid the warning.
 * \param [out] str     Buffer of length at least \a size.
 *                      On output, not touched if NULL or \a size == 0.
 *                      Otherwise, "" on snprintf error or the proper result.
 *                      The proper result is padded on the right with '\0'
 *                      if the allocation length of the string that is
 *                      defined by \a format is shorter than \a size.
 * \param [in] size     Allocation length of \a str.
 * \param [in] format   Format string as in man (3) snprintf.
 */
void                sc_snprintf (char *str, size_t size,
                                 SC_PRINTF_FMT const char *format, ...)
  SC_PRINTF_LIKE(3, 4);

/** Return the full version of libsc.
 *
 * \return          Return the version of libsc using the format
 *                  `VERSION_MAJOR.VERSION_MINOR.VERSION_POINT`,
 *                  where `VERSION_POINT` can contain dots and
 *                  characters, e.g. to indicate the additional
 *                  number of commits and a git commit hash.
 */
const char         *sc_version (void);

/** Return the major version of libsc.
 *
 * \return          Return the major version of libsc.
 */
int                 sc_version_major (void);

/** Return the minor version of libsc.
 *
 * \return          Return the minor version of libsc.
 */
int                 sc_version_minor (void);

/** Perform a runtime check for the integer endian convention.
 * \return          True if byte order is little endian, false otherwise.
 */
int                 sc_is_littleendian (void);

#if 0
/* Sadly, the point version macro by autoconf doesn't work with vX and vX.Y.
   The remaining option is to use sc_version and parse its return string. */
/** Return the point version of libsc.
 *
 * \return          Return the (first part of the) point version of libsc,
 *                  without information about the additional number of commits
 *                  and commit hash.
 */
int                 sc_version_point (void);
#endif /* 0 */

/** Return a boolean indicating whether zlib has been configured.
 * \return          True if zlib including adler32_combine (3)
 *                  has been found on running configure
 *                  or respectively on calling cmake.
 */
int                 sc_have_zlib (void);

/** Return whether we have found a JSON library at configure time.
 * \return          True if and only if SC_HAVE_JSON is defined.
 */
int                 sc_have_json (void);

/** Portable function to sleep a prescribed amount of milliseconds.
 *
 * \param [in] milliseconds The number of milliseconds to sleep.
 */
void                sc_sleep (unsigned milliseconds);

SC_EXTERN_C_END;

#endif /* SC_H */
