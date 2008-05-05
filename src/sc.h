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

/* this file must be named config.h due to gnulib */
/* make sure your package uses a different name for the config header */
#include <config.h>
#define SC_BUGREPORT PACKAGE_BUGREPORT
#define SC_STRING PACKAGE_STRING
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

/* header files safe to use via gnulib with above config.h included */
#include <getopt.h>
#include <obstack.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* using sqrt to compute hash statistics */
#include <math.h>

/* require zlib header for adler32 checksums */
#ifdef HAVE_ZLIB_H
#include <zlib.h>
#endif

/* check macros, always enabled */
#define SC_NOOP do { ; } while (0)
#define SC_CHECK_ABORT(c,s)                        \
  do {                                             \
    if (!(c)) {                                    \
      fprintf (stderr, "Abort: %s\n   in %s:%d\n", \
               (s), __FILE__, __LINE__);           \
      sc_abort ();                                 \
    }                                              \
  } while (0)
#define SC_CHECK_ABORTF(c,fmt,...)                      \
  do {                                                  \
    if (!(c)) {                                         \
      fprintf (stderr, "Abort: " fmt "\n   in %s:%d\n", \
               __VA_ARGS__, __FILE__, __LINE__);        \
      sc_abort ();                                      \
    }                                                   \
  } while (0)
#define SC_CHECK_MPI(r) SC_CHECK_ABORT ((r) == MPI_SUCCESS, "MPI operation")

/* assertions, only enabled in debug mode */
#ifdef SC_DEBUG
#define SC_ASSERT(c) SC_CHECK_ABORT ((c), "Assertion '" #c "'")
#else
#define SC_ASSERT(c) SC_NOOP
#endif
#define SC_ASSERT_NOT_REACHED() SC_CHECK_ABORT (0, "Unreachable code")

/* macros for memory allocation, will abort if out of memory */
#define SC_ALLOC(t,n) (t *) sc_malloc ((n) * sizeof(t))
#define SC_ALLOC_ZERO(t,n) (t *) sc_calloc ((n), sizeof(t))
#define SC_REALLOC(p,t,n) (t *) sc_realloc ((p), (n) * sizeof(t))
#define SC_STRDUP(s) sc_strdup (s)
#define SC_FREE(p) sc_free (p)

/* min and max helper macros */
#define SC_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define SC_MAX(a,b) (((a) > (b)) ? (a) : (b))

/* hopefully fast binary logarithms and binary round up */
#define SC_LOG2_8(x) (sc_log_lookup_table[(x)])
#define SC_LOG2_16(x) (((x) > 0xff) ?                                   \
                       (SC_LOG2_8 ((x) >> 8) + 8) : SC_LOG2_8 (x))
#define SC_LOG2_32(x) (((x) > 0xffff) ?                                 \
                       (SC_LOG2_16 ((x) >> 16)) + 16 : SC_LOG2_16 (x))
#define SC_LOG2_64(x) (((long long) (x) > 0xffffffffLL) ?               \
                       (SC_LOG2_32 ((x) >> 32)) + 32 : SC_LOG2_32 (x))
#define SC_ROUNDUP2_32(x)                               \
  (((x) <= 0) ? 0 : (1 << (SC_LOG2_32 ((x) - 1) + 1)))
#define SC_ROUNDUP2_64(x)                               \
  (((x) <= 0) ? 0 : (1 << (SC_LOG2_64 ((x) - 1) + 1)))

/* primitive logging macros for now */
#define SC_LP_NONE        0
#define SC_LP_TRACE       1     /* this will prefix file and line number */
#define SC_LP_DEBUG       2     /* any information on the internal state */
#define SC_LP_VERBOSE     3     /* information on conditions, decisions */
#define SC_LP_INFO        4     /* the main things a function is doing */
#define SC_LP_STATISTICS  5     /* important for consistency or performance */
#define SC_LP_PRODUCTION  6     /* a few lines for a major api function */
#define SC_LP_SILENT      7     /* this will never log anything */
#ifdef SC_DEBUG
#define SC_LP_THRESHOLD SC_LP_DEBUG
#else
#define SC_LP_THRESHOLD SC_LP_INFO
#endif
#define SC_LOGF(level,fmt,...)             \
  do {                                     \
    if ((level) >= SC_LP_THRESHOLD) {      \
      printf ((fmt), __VA_ARGS__);         \
    }                                      \
  } while (0)
#define SC_LOG(level,s) SC_LOGF((level), "%s", (s))

/* extern declarations */
extern const int    sc_log_lookup_table[256];

/* memory allocation functions, handle NULL pointers gracefully */
void               *sc_malloc (size_t size);
void               *sc_calloc (size_t nmemb, size_t size);
void               *sc_realloc (void *ptr, size_t size);
char               *sc_strdup (const char *s);
void                sc_free (void *ptr);
void                sc_memory_check (void);

/* prints a stack trace, calls the abort handler and terminates */
void                sc_abort (void)
  __attribute__ ((noreturn));

#endif /* SC_H */
