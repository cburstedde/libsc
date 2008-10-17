/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2008 Carsten Burstedde, Lucas Wilcox.

  The SC Library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with the SC Library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SC_H
#define SC_H

/* include the sc_config header first */

#include <sc_config.h>

/* provide extern C defines */

#ifdef __cplusplus              /* enforce semicolon */
#define SC_EXTERN_C_BEGIN       extern "C" {    void sc_extern_c_begin (void)
#define SC_EXTERN_C_END         }               void sc_extern_cc_end (void)
#else
#define SC_EXTERN_C_BEGIN                       void sc_extern_c_begin (void)
#define SC_EXTERN_C_END                         void sc_extern_c_end (void)
#endif

/* include MPI before stdio.h */

#ifdef SC_MPI
#include <mpi.h>
#ifndef MPI_UNSIGNED_LONG_LONG
#define MPI_UNSIGNED_LONG_LONG MPI_LONG_LONG_INT
#endif
#else
#ifdef MPI_SUCCESS
#error "mpi.h is included.  Use --enable-mpi."
#endif
#endif

/* include system headers */

#include <math.h>
#include <ctype.h>
#include <float.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef SC_PROVIDE_GETOPT
#ifdef _GETOPT_H
#error "getopt.h is included.  Include sc.h first or use --without-getopt".
#endif
#include <sc_getopt.h>
#else
#include <getopt.h>
#endif

#ifdef SC_PROVIDE_OBSTACK
#ifdef _OBSTACK_H
#error "obstack.h is included.  Include sc.h first or use --without-obstack".
#endif
#include <sc_obstack.h>
#else
#include <obstack.h>
#endif

#ifdef SC_PROVIDE_ZLIB
#ifdef ZLIB_H
#error "zlib.h is included.  Include sc.h first or use --without-zlib".
#endif
#include <sc_zlib.h>
#else
#include <zlib.h>
#endif

#include <sc_c99_functions.h>
#include <sc_mpi.h>

#if 0
/*@ignore@*/
#define index   DONT_USE_NAME_CONFLICT_1 ---
#define rindex  DONT_USE_NAME_CONFLICT_2 ---
#define link    DONT_USE_NAME_CONFLICT_3 ---
#define NO_DEFINE_DONT_USE_CONFLICT SPLINT_IS_STUPID_ALSO
/*@end@*/
#endif /* 0 */

/* check macros, always enabled */

#define SC_NOOP() do { ; } while (0)
#define SC_CHECK_ABORT(c,s)                        \
  do {                                             \
    if (!(c)) {                                    \
      sleep (1);                                   \
      fprintf (stderr, "Abort: %s\n   in %s:%d\n", \
               (s), __FILE__, __LINE__);           \
      sc_abort ();                                 \
    }                                              \
  } while (0)
/* Only include the following define in C, not C++, since C++98
   does not allow variadic macros. */
#ifndef __cplusplus
#define SC_CHECK_ABORTF(c,fmt,...)                      \
  do {                                                  \
    if (!(c)) {                                         \
      sleep (1);                                        \
      fprintf (stderr, "Abort: " fmt "\n   in %s:%d\n", \
               __VA_ARGS__, __FILE__, __LINE__);        \
      sc_abort ();                                      \
    }                                                   \
  } while (0)
#endif
#define SC_CHECK_MPI(r) SC_CHECK_ABORT ((r) == MPI_SUCCESS, "MPI operation")
#define SC_CHECK_NOT_REACHED() SC_CHECK_ABORT (0, "Unreachable code")

/* assertions, only enabled in debug mode */

#ifdef SC_DEBUG
#define SC_ASSERT(c) SC_CHECK_ABORT ((c), "Assertion '" #c "'")
#else
#define SC_ASSERT(c) SC_NOOP ()
#endif

/* macros for memory allocation, will abort if out of memory */

#define SC_ALLOC(t,n)         (t *) sc_malloc (sc_package_id, (n) * sizeof(t))
#define SC_ALLOC_ZERO(t,n)    (t *) sc_calloc (sc_package_id, (n), sizeof(t))
#define SC_REALLOC(p,t,n)     (t *) sc_realloc (sc_package_id,          \
                                             (p), (n) * sizeof(t))
#define SC_STRDUP(s)                sc_strdup (sc_package_id, (s))
#define SC_FREE(p)                  sc_free (sc_package_id, (p))

/**
 * Sets n elements of a memory range to zero.
 * Assumes the pointer p is of the correct type.
 */
#define SC_BZERO(p,n) do { memset ((p), 0, (n) * sizeof (*(p))); } while (0)

/* min and max helper macros */

#define SC_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define SC_MAX(a,b) (((a) > (b)) ? (a) : (b))

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
  (((x) <= 0) ? 0 : (1 << (SC_LOG2_64 ((x) - 1LL) + 1)))

/* log categories */

#define SC_LC_GLOBAL      1     /* log only for master process */
#define SC_LC_NORMAL      2     /* log for every process */

/* log priorities */

#define SC_LP_DEFAULT   (-1)    /* this selects the SC default threshold */
#define SC_LP_NONE        0
#define SC_LP_TRACE       1     /* this will prefix file and line number */
#define SC_LP_DEBUG       2     /* any information on the internal state */
#define SC_LP_VERBOSE     3     /* information on conditions, decisions */
#define SC_LP_INFO        4     /* the main things a function is doing */
#define SC_LP_STATISTICS  5     /* important for consistency or performance */
#define SC_LP_PRODUCTION  6     /* a few lines for a major api function */
#define SC_LP_SILENT      7     /* this will never log anything */
#ifdef SC_LOG_PRIORITY
#define SC_LP_THRESHOLD SC_LOG_PRIORITY
#else
#ifdef SC_DEBUG
#define SC_LP_THRESHOLD SC_LP_DEBUG
#else
#define SC_LP_THRESHOLD SC_LP_INFO
#endif
#endif

/* generic log macros */

/* Only include these logging macros in C, not C++, since C++99
   does not allow variadic macros. */
#ifndef __cplusplus

#define SC_LOGF(package,category,priority,fmt,...)                      \
  do {                                                                  \
    if ((priority) >= SC_LP_THRESHOLD) {                                \
      sc_logf (__FILE__, __LINE__, (package), (category), (priority),   \
               (fmt), __VA_ARGS__);                                     \
    }                                                                   \
  } while (0)
#define SC_LOG(g,c,p,s) SC_LOGF((g), (c), (p), "%s", (s))
#define SC_GLOBAL_LOG(p,s) SC_LOG (sc_package_id, SC_LC_GLOBAL, (p), (s))
#define SC_GLOBAL_LOGF(p,f,...) \
  SC_LOGF (sc_package_id, SC_LC_GLOBAL, (p), (f), __VA_ARGS__)
#define SC_NORMAL_LOG(p,s) SC_LOG (sc_package_id, SC_LC_NORMAL, (p), (s))
#define SC_NORMAL_LOGF(p,f,...) \
  SC_LOGF (sc_package_id, SC_LC_NORMAL, (p), (f), __VA_ARGS__)

/* convenience global log macros will only output if identifier <= 0 */

#define SC_GLOBAL_TRACE(s) SC_GLOBAL_LOG (SC_LP_TRACE, (s))
#define SC_GLOBAL_TRACEF(f,...) \
  SC_GLOBAL_LOGF (SC_LP_TRACE, (f), __VA_ARGS__)
#define SC_GLOBAL_LDEBUG(s) SC_GLOBAL_LOG (SC_LP_DEBUG, (s))
#define SC_GLOBAL_LDEBUGF(f,...) \
  SC_GLOBAL_LOGF (SC_LP_DEBUG, (f), __VA_ARGS__)
#define SC_GLOBAL_VERBOSE(s) SC_GLOBAL_LOG (SC_LP_VERBOSE, (s))
#define SC_GLOBAL_VERBOSEF(f,...) \
  SC_GLOBAL_LOGF (SC_LP_VERBOSE, (f), __VA_ARGS__)
#define SC_GLOBAL_INFO(s) SC_GLOBAL_LOG (SC_LP_INFO, (s))
#define SC_GLOBAL_INFOF(f,...) \
  SC_GLOBAL_LOGF (SC_LP_INFO, (f), __VA_ARGS__)
#define SC_GLOBAL_STATISTICS(s) SC_GLOBAL_LOG (SC_LP_STATISTICS, (s))
#define SC_GLOBAL_STATISTICSF(f,...) \
  SC_GLOBAL_LOGF (SC_LP_STATISTICS, (f), __VA_ARGS__)
#define SC_GLOBAL_PRODUCTION(s) SC_GLOBAL_LOG (SC_LP_PRODUCTION, (s))
#define SC_GLOBAL_PRODUCTIONF(f,...) \
  SC_GLOBAL_LOGF (SC_LP_PRODUCTION, (f), __VA_ARGS__)

/* convenience log macros that output regardless of identifier */

#define SC_TRACE(s) SC_NORMAL_LOG (SC_LP_TRACE, (s))
#define SC_TRACEF(f,...) \
  SC_NORMAL_LOGF (SC_LP_TRACE, (f), __VA_ARGS__)
#define SC_LDEBUG(s) SC_NORMAL_LOG (SC_LP_DEBUG, (s))
#define SC_LDEBUGF(f,...) \
  SC_NORMAL_LOGF (SC_LP_DEBUG, (f), __VA_ARGS__)
#define SC_VERBOSE(s) SC_NORMAL_LOG (SC_LP_VERBOSE, (s))
#define SC_VERBOSEF(f,...) \
  SC_NORMAL_LOGF (SC_LP_VERBOSE, (f), __VA_ARGS__)
#define SC_INFO(s) SC_NORMAL_LOG (SC_LP_INFO, (s))
#define SC_INFOF(f,...) \
  SC_NORMAL_LOGF (SC_LP_INFO, (f), __VA_ARGS__)
#define SC_STATISTICS(s) SC_NORMAL_LOG (SC_LP_STATISTICS, (s))
#define SC_STATISTICSF(f,...) \
  SC_NORMAL_LOGF (SC_LP_STATISTICS, (f), __VA_ARGS__)
#define SC_PRODUCTION(s) SC_NORMAL_LOG (SC_LP_PRODUCTION, (s))
#define SC_PRODUCTIONF(f,...) \
  SC_NORMAL_LOGF (SC_LP_PRODUCTION, (f), __VA_ARGS__)

#endif /* !__cplusplus */

SC_EXTERN_C_BEGIN;

/* callback typedefs */

typedef void        (*sc_handler_t) (void *data);
typedef void        (*sc_log_handler_t) (const char *filename, int lineno,
                                         int package, int category,
                                         int priority, const char *fmt,
                                         va_list ap);

/* extern declarations */

extern const int    sc_log2_lookup_table[256];
extern void        *SC_VP_DEFAULT;      /* a unique void pointer value */
extern FILE        *SC_FP_KEEP; /* a unique FILE pointer value */
extern FILE        *sc_root_stdout;
extern FILE        *sc_root_stderr;
extern int          sc_package_id;

/* memory allocation functions, handle NULL pointers gracefully */

void               *sc_malloc (int package, size_t size);
void               *sc_calloc (int package, size_t nmemb, size_t size);
void               *sc_realloc (int package, void *ptr, size_t size);
char               *sc_strdup (int package, const char *s);
void                sc_free (int package, void *ptr);
void                sc_memory_check (int package);

/* comparison functions for various integer sizes */

int                 sc_int_compare (const void *v1, const void *v2);
int                 sc_int16_compare (const void *v1, const void *v2);
int                 sc_int32_compare (const void *v1, const void *v2);
int                 sc_int64_compare (const void *v1, const void *v2);

/** Controls the default SC log behavior.
 * \param [in] log_handler   Set default SC log handler (NULL selects builtin).
 * \param [in] log_threshold Set default SC log threshold (or SC_LP_DEFAULT).
 * \param [in] log_stream    Set stream to use by the builtin log handler.
 *                           This can be SC_FP_KEEP to keep the status quo
 *                           or NULL which silences the builtin log handler.
 */
void                sc_set_log_defaults (sc_log_handler_t log_handler,
                                         int log_thresold, FILE * log_stream);

/** The central log function to be called by all packages.
 * Dispatches the log calls by package and filters by category and priority.
 * \param [in] package   Must be a registered package id or -1.
 * \param [in] category  Must be SC_LC_NORMAL or SC_LC_GLOBAL.
 * \param [in] priority  Must be >= SC_LP_NONE and < SC_LP_SILENT.
 */
void                sc_logf (const char *filename, int lineno,
                             int package, int category, int priority,
                             const char *fmt, ...)
  __attribute__ ((format (printf, 6, 7)));

/** Generic MPI abort handler. The data must point to a MPI_Comm object. */
void                sc_generic_abort (void *data);

/** Installs an abort handler and catches signals INT SEGV USR2. */
void                sc_set_abort_handler (sc_handler_t handler, void *data);

/** Prints a stack trace, calls the abort handler and terminates. */
void                sc_abort (void)
  __attribute__ ((noreturn));

/** Register a software package with SC.
 * \param [in] log_handler   Custom log function, or NULL to select the
 *                           default SC log handler.
 * \param [in] log_threshold Minimal log priority required for output, or
 *                           SC_LP_DEFAULT to select the SC default threshold.
 * \return                   Returns a unique package id.
 */
int                 sc_package_register (sc_log_handler_t log_handler,
                                         int log_threshold,
                                         const char *name, const char *full);
bool                sc_package_is_registered (int package_id);
void                sc_package_unregister (int package_id);

/** Print a summary of all packages registered with SC.
 * \param [in] stream  Stream to print to. If NULL, nothing happens.
 */
void                sc_package_summary (FILE * stream);

/** Sets the global program identifier (e.g. the MPI rank) and abort handler.
 * This function is optional.
 * If this function is not called or called with log_handler == NULL,
 * the default SC log handler will be used.
 * If this function is not called or called with log_threshold == SC_LP_DEFAULT,
 * the default SC log threshold will be used.
 * The default SC log settings can be changed with sc_set_log_defaults ().
 */
void                sc_init (int identifier,
                             sc_handler_t abort_handler, void *abort_data,
                             sc_log_handler_t log_handler, int log_threshold);

/** Unregisters all packages, runs the memory check, removes the
 * abort and signal handlers and resets sc_identifier and sc_root_*.
 * This function is optional.
 * This function does not require sc_init to be called first.
 */
void                sc_finalize (void);

SC_EXTERN_C_END;

#endif /* SC_H */
