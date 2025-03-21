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

#include <sc_private.h>

#ifdef SC_HAVE_SIGNAL_H
#include <signal.h>
#endif

typedef void        (*sc_sig_t) (int);

#ifdef SC_HAVE_BACKTRACE
#ifdef SC_HAVE_BACKTRACE_SYMBOLS
#ifdef SC_HAVE_EXECINFO_H
#include <execinfo.h>
#define SC_BACKTRACE
#define SC_STACK_SIZE 64
#endif
#endif
#endif

#ifdef SC_ENABLE_PTHREAD
#include <pthread.h>
#endif

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#if _POSIX_C_SOURCE >= 199309L
#include <time.h>
#else
#include <unistd.h>
#endif

typedef struct sc_package
{
  int                 is_registered;
  sc_log_handler_t    log_handler;
  int                 log_threshold;
  int                 log_indent;
  int                 malloc_count;
  int                 free_count;
  int                 rc_active;
  int                 abort_mismatch;
  const char         *name;
  const char         *full;
#ifdef SC_ENABLE_PTHREAD
  pthread_mutex_t     mutex;
#endif
}
sc_package_t;

/** The only log handler that comes with libsc. */
static void         sc_log_handler (FILE * log_stream,
                                    const char *filename, int lineno,
                                    int package, int category, int priority,
                                    const char *msg);

/* *INDENT-OFF* */
const int sc_log2_lookup_table[256] =
{ -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
   4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
   5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
   5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
   6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
   6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
   6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
   6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
};
/* *INDENT-ON* */

int                 sc_package_id = -1;
int                 sc_initialized = 0;

FILE               *sc_trace_file = NULL;
int                 sc_trace_prio = SC_LP_STATISTICS;

static int          default_malloc_count = 0;
static int          default_free_count = 0;
static int          default_rc_active = 0;
static int          default_abort_mismatch = 1;

static int          sc_identifier = -1;
static sc_MPI_Comm  sc_mpicomm = sc_MPI_COMM_NULL;

static FILE        *sc_log_stream = NULL;
static sc_log_handler_t sc_default_log_handler = sc_log_handler;
static int          sc_default_log_threshold = SC_LP_THRESHOLD;

static void         sc_abort_handler (void);
static sc_abort_handler_t sc_default_abort_handler = sc_abort_handler;

static int          sc_signals_caught = 0;
static sc_sig_t     system_int_handler = NULL;
static sc_sig_t     system_segv_handler = NULL;

static int          sc_print_backtrace = 0;

static int          sc_num_packages = 0;
static int          sc_num_packages_alloc = 0;
static sc_package_t *sc_packages = NULL;

void
sc_extern_c_hack_1 (void)
{
  /* Completing the hack in sc.h on providing the prototype.
     We use the macro SC_EXTER_C_BEGIN; after including all headers
     and before declaring functions to ensure C linkage. */
}

void
sc_extern_c_hack_2 (void)
{
  /* Completing the hack in sc.h on providing the prototype.
     We use the macro SC_EXTER_C_END; after declaring all functions,
     just before the final include-once check, to ensure C linkage. */
}

int
sc_mpi_is_enabled (void)
{
#ifdef SC_ENABLE_MPI
  return 1;
#else
  return 0;
#endif
}

int
sc_mpi_is_shared (void)
{
#ifdef SC_ENABLE_MPISHARED
  return 1;
#else
  return 0;
#endif
}

#ifdef SC_ENABLE_PTHREAD

static pthread_mutex_t sc_default_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sc_error_mutex = PTHREAD_MUTEX_INITIALIZER;

int
sc_get_package_id (void)
{
  return sc_package_id;
}

static void
sc_check_abort_thread (int condition, int package, const char *message)
{
  if (!condition) {
    pthread_mutex_lock (&sc_error_mutex);
    printf ("[libsc] sc_check_abort_thread %d %s\n", package, message);
    abort ();
    /* abort () will not return */
    pthread_mutex_unlock (&sc_error_mutex);
  }
}

static inline pthread_mutex_t *
sc_package_mutex (int package)
{
  if (package == -1) {
    return &sc_default_mutex;
  }
  else {
#ifdef SC_ENABLE_DEBUG
    sc_check_abort_thread (sc_package_is_registered (package),
                           package, "sc_package_mutex");
#endif
    return &sc_packages[package].mutex;
  }
}

#endif /* SC_ENABLE_PTHREAD */

void
sc_package_lock (int package)
{
#ifdef SC_ENABLE_PTHREAD
  pthread_mutex_t    *mutex = sc_package_mutex (package);
  int                 pth;

  pth = pthread_mutex_lock (mutex);
  sc_check_abort_thread (pth == 0, package, "sc_package_lock");
#endif
}

void
sc_package_unlock (int package)
{
#ifdef SC_ENABLE_PTHREAD
  pthread_mutex_t    *mutex = sc_package_mutex (package);
  int                 pth;

  pth = pthread_mutex_unlock (mutex);
  sc_check_abort_thread (pth == 0, package, "sc_package_unlock");
#endif
}

void
sc_package_rc_count_add (int package_id, int toadd)
{
#ifndef SC_NOCOUNT_REFCOUNT
  int                *pcount;
#ifdef SC_ENABLE_DEBUG
  int                 newvalue;
#endif

  if (package_id == -1) {
    pcount = &default_rc_active;
  }
  else {
    SC_ASSERT (sc_package_is_registered (package_id));
    pcount = &sc_packages[package_id].rc_active;
  }

  sc_package_lock (package_id);
#ifdef SC_ENABLE_DEBUG
  newvalue =
#endif
    *pcount += toadd;
  sc_package_unlock (package_id);

  SC_ASSERT (newvalue >= 0);
#endif
}

static void
sc_signal_handler (int sig)
{
  const char         *sigstr;

  switch (sig) {
  case SIGINT:
    sigstr = "INT";
    break;
  case SIGSEGV:
    sigstr = "SEGV";
    break;
  default:
    sigstr = "<unknown>";
    break;
  }
  SC_LERRORF ("Caught signal %s\n", sigstr);

  sc_abort ();
}

/** Installs or removes a signal handler for INT SEGV that aborts.
 * \param [in] catch    If true, catch signals INT SEGV.
 *                      If false, reinstate previous signal handler.
 */
static void
sc_set_signal_handler (int catch_signals)
{
  if (catch_signals && !sc_signals_caught) {
    system_int_handler = signal (SIGINT, sc_signal_handler);
    SC_CHECK_ABORT (system_int_handler != SIG_ERR, "catching INT");
    system_segv_handler = signal (SIGSEGV, sc_signal_handler);
    SC_CHECK_ABORT (system_segv_handler != SIG_ERR, "catching SEGV");
    sc_signals_caught = 1;
  }
  else if (!catch_signals && sc_signals_caught) {
    (void) signal (SIGINT, system_int_handler);
    system_int_handler = NULL;
    (void) signal (SIGSEGV, system_segv_handler);
    system_segv_handler = NULL;
    sc_signals_caught = 0;
  }
}

static void
sc_log_handler (FILE * log_stream, const char *filename, int lineno,
                int package, int category, int priority, const char *msg)
{
  int                 wp = 0, wi = 0;
  int                 lindent = 0;

  if (package != -1) {
    if (!sc_package_is_registered (package))
      package = -1;
    else {
      wp = 1;
      lindent = sc_packages[package].log_indent;
    }
  }
  wi = (category == SC_LC_NORMAL && sc_identifier >= 0);

  if (wp || wi) {
    fputc ('[', log_stream);
    if (wp)
      fprintf (log_stream, "%s", sc_packages[package].name);
    if (wp && wi)
      fputc (' ', log_stream);
    if (wi)
      fprintf (log_stream, "%d", sc_identifier);
    fprintf (log_stream, "] %*s", lindent, "");
  }

  if (priority == SC_LP_TRACE) {
    char                bn[BUFSIZ], *bp;

    snprintf (bn, BUFSIZ, "%s", filename);
#ifdef SC_HAVE_LIBGEN_H
    bp = basename (bn);
#else
    bp = bn;
#endif
    fprintf (log_stream, "%s:%d ", bp, lineno);
  }

  fputs (msg, log_stream);
  fflush (log_stream);
}

#ifndef SC_NOCOUNT_MALLOC

static int         *
sc_malloc_count (int package)
{
  if (package == -1)
    return &default_malloc_count;

  SC_ASSERT (sc_package_is_registered (package));
  return &sc_packages[package].malloc_count;
}

static int         *
sc_free_count (int package)
{
  if (package == -1)
    return &default_free_count;

  SC_ASSERT (sc_package_is_registered (package));
  return &sc_packages[package].free_count;
}

#endif

#ifdef SC_ENABLE_MEMALIGN

/* *INDENT-OFF* */
static void        *
sc_malloc_aligned (size_t alignment, size_t size)
SC_ATTR_ALIGN (SC_MEMALIGN_BYTES);
/* *INDENT-ON* */

static void        *
sc_malloc_aligned (size_t alignment, size_t size)
{
  /* minimum requirements on alignment */
  SC_ASSERT (sizeof (char **) == sizeof (void *));
  SC_ASSERT (sizeof (char **) >= sizeof (size_t));
  SC_ASSERT (alignment > 0 && alignment % sizeof (void *) == 0);
  SC_ASSERT (alignment == SC_MEMALIGN_BYTES);

#if defined SC_HAVE_ANY_MEMALIGN && defined SC_HAVE_POSIX_MEMALIGN
  {
    void               *data = NULL;
    int                 err = posix_memalign (&data, alignment, size);
    SC_CHECK_ABORTF (err != ENOMEM, "Insufficient memory (malloc size %llu)",
                     (long long unsigned) size);
    SC_CHECK_ABORTF (err != EINVAL, "Alignment %llu is not a power of two"
                     "or not a multiple of sizeof (void *)",
                     (long long unsigned) alignment);
    SC_CHECK_ABORTF (err == 0, "Return of %d from posix_memalign", err);
    return data;
  }
#elif defined SC_HAVE_ANY_MEMALIGN && defined SC_HAVE_ALIGNED_ALLOC
  {
    void               *data = aligned_alloc (alignment, size);
    SC_CHECK_ABORT (data != NULL || size == 0,
                    "Returned NULL from aligned_alloc");
    return data;
  }
#elif defined SC_HAVE_ANY_MEMALIGN && defined SC_HAVE_ALIGNED_MALLOC
  /* MinGW, MSVC */
  {
    void               *data = _aligned_malloc (size, alignment);
    SC_CHECK_ABORT (data != NULL || size == 0,
                    "Returned NULL from aligned_alloc");
    return data;
  }
#else
  {
#if 0
    /* adapted from PetscMallocAlign */
    int                *datastart = malloc (size + 2 * alignment);
    int                 shift = ((uintptr_t) datastart) % alignment;

    shift = (2 * alignment - shift) / sizeof (int);
    datastart[shift - 1] = shift;
    datastart += shift;
    return (void *) datastart;
#endif
    /* We pad to achieve alignment, then write the original pointer and data
     * size up front, then the real data shifted by at most alignment - 1
     * bytes.  This way there is always at least one stop byte at the end that
     * we can use for debugging. */
    const ptrdiff_t     extrasize = (ptrdiff_t) (2 * sizeof (char **));
    const ptrdiff_t     signalign = (ptrdiff_t) alignment;
    const size_t        alloc_size = extrasize + size + alignment;
    char               *alloc_ptr = (char *) malloc (alloc_size);
    char               *ptr;
    ptrdiff_t           shift, modu;

    SC_CHECK_ABORT (alloc_ptr != NULL, "Returned NULL from malloc");

    /* compute shift to the right where we put the actual data */
    modu = ((ptrdiff_t) alloc_ptr + extrasize) % signalign;
    shift = (signalign - modu) % signalign;
    SC_ASSERT (0 <= shift && shift < signalign);

    /* make sure the resulting pointer is fine */
    ptr = alloc_ptr + (extrasize + shift);
    SC_ASSERT ((ptrdiff_t) ptr % signalign == 0);

    /* memorize the original pointer that we got from malloc and fill up */
    SC_ARG_ALIGN (ptr, char *, SC_MEMALIGN_BYTES);

    /* remember parameters of allocation for later use */
    ((char **) ptr)[-1] = alloc_ptr;
    ((char **) ptr)[-2] = (char *) size;
#ifdef SC_ENABLE_DEBUG
    memset (alloc_ptr, (char) -2, shift);
    SC_ASSERT (ptr + ((ptrdiff_t) size + signalign - shift) ==
               alloc_ptr + alloc_size);
    memset (ptr + size, (char) -2, signalign - shift);
#endif

    /* and we are done */
    return (void *) ptr;
  }
#endif
}

static void
sc_free_aligned (void *ptr, size_t alignment)
{
  /* minimum requirements on alignment */
  SC_ASSERT (sizeof (char **) == sizeof (void *));
  SC_ASSERT (sizeof (char **) >= sizeof (size_t));
  SC_ASSERT (alignment > 0 && alignment % sizeof (void *) == 0);

#if defined SC_HAVE_ANY_MEMALIGN && \
   (defined SC_HAVE_POSIX_MEMALIGN || defined SC_HAVE_ALIGNED_ALLOC)
  free (ptr);
#else
  {
#if 0
    int                *datastart = ptr;
    int                 shift = datastart[-1];

    datastart -= shift;
    free ((void *) datastart);
#endif
    /* this mirrors the function sc_malloc_aligned above */
    char               *alloc_ptr;
#ifdef SC_ENABLE_DEBUG
    const ptrdiff_t     extrasize = (ptrdiff_t) (2 * sizeof (char **));
    const ptrdiff_t     signalign = (ptrdiff_t) alignment;
    ptrdiff_t           shift, modu, ssize, i;
#endif

    /* we excluded these cases earlier */
    SC_ASSERT (ptr != NULL);
    SC_ASSERT ((ptrdiff_t) ptr % signalign == 0);

    alloc_ptr = ((char **) ptr)[-1];
    SC_ASSERT (alloc_ptr != NULL);

#ifdef SC_ENABLE_DEBUG
    /* compute shift to the right where we put the actual data */
    ssize = (ptrdiff_t) ((char **) ptr)[-2];
    modu = ((ptrdiff_t) alloc_ptr + extrasize) % signalign;
    shift = (signalign - modu) % signalign;
    SC_ASSERT (0 <= shift && shift < signalign);
    SC_ASSERT ((char *) ptr == alloc_ptr + (extrasize + shift));
    for (i = 0; i < shift; ++i) {
      SC_ASSERT (alloc_ptr[i] == (char) -2);
    }
    for (i = 0; i < signalign - shift; ++i) {
      SC_ASSERT (((char *) ptr)[ssize + i] == (char) -2);
    }
#endif

    /* free the original pointer */
    free (alloc_ptr);
  }
#endif
}

/* *INDENT-OFF* */
static void        *
sc_realloc_aligned (void *ptr, size_t alignment, size_t size)
SC_ATTR_ALIGN (SC_MEMALIGN_BYTES);
/* *INDENT-ON* */

static void        *
sc_realloc_aligned (void *ptr, size_t alignment, size_t size)
{
  /* minimum requirements on alignment */
  SC_ASSERT (sizeof (char **) == sizeof (void *));
  SC_ASSERT (sizeof (char **) >= sizeof (size_t));
  SC_ASSERT (alignment > 0 && alignment % sizeof (void *) == 0);
  SC_ASSERT (alignment == SC_MEMALIGN_BYTES);

#if defined SC_HAVE_ANY_MEMALIGN && \
   (defined SC_HAVE_POSIX_MEMALIGN || defined SC_HAVE_ALIGNED_ALLOC)
  /* the result is no longer aligned */
  return realloc (ptr, size);
#else
  {
#ifdef SC_ENABLE_DEBUG
    const ptrdiff_t     signalign = (ptrdiff_t) alignment;
#endif
    size_t              old_size, min_size;
    void               *new_ptr;

    /* we excluded these cases earlier */
    SC_ASSERT (ptr != NULL && size > 0);
    SC_ASSERT ((ptrdiff_t) ptr % signalign == 0);

    /* back out the previously allocated size */
    old_size = (size_t) ((char **) ptr)[-2];

    /* create new memory while the old memory is still around */
    new_ptr = sc_malloc_aligned (alignment, size);

    /* copy data */
    min_size = SC_MIN (old_size, size);
    memcpy (new_ptr, ptr, min_size);
#ifdef SC_ENABLE_DEBUG
    memset ((char *) new_ptr + min_size, (char) -3, size - min_size);
#endif

    /* free old memory and return new pointer */
    sc_free_aligned (ptr, alignment);
    return new_ptr;
  }
#endif
}

#endif /* SC_ENABLE_MEMALIGN */

void               *
sc_malloc (int package, size_t size)
{
  void               *ret;
#ifndef SC_NOCOUNT_MALLOC
  int                *malloc_count = sc_malloc_count (package);
#endif

  /* allocate memory */
#ifdef SC_ENABLE_MEMALIGN
  ret = sc_malloc_aligned (SC_MEMALIGN_BYTES, size);
#else
  ret = malloc (size);
  if (size > 0) {
    SC_CHECK_ABORTF (ret != NULL, "Allocation (malloc size %lli)",
                     (long long int) size);
  }
#endif

  /* count the allocations */
#ifdef SC_ENABLE_PTHREAD
  sc_package_lock (package);
#endif

#ifndef SC_NOCOUNT_MALLOC
  if (size > 0) {
    ++*malloc_count;
  }
  else {
    *malloc_count += ((ret == NULL) ? 0 : 1);
  }
#endif

#ifdef SC_ENABLE_PTHREAD
  sc_package_unlock (package);
#endif

  return ret;
}

void               *
sc_calloc (int package, size_t nmemb, size_t size)
{
  void               *ret;
#ifndef SC_NOCOUNT_MALLOC
  int                *malloc_count = sc_malloc_count (package);
#endif

  /* allocate memory */
#ifdef SC_ENABLE_MEMALIGN
  ret = sc_malloc_aligned (SC_MEMALIGN_BYTES, nmemb * size);
  memset (ret, 0, nmemb * size);
#else
  ret = calloc (nmemb, size);
  if (nmemb * size > 0) {
    SC_CHECK_ABORTF (ret != NULL, "Allocation (calloc size %lli)",
                     (long long int) size);
  }
#endif

  /* count the allocations */
#ifdef SC_ENABLE_PTHREAD
  sc_package_lock (package);
#endif

#ifndef SC_NOCOUNT_MALLOC
  if (nmemb * size > 0) {
    ++*malloc_count;
  }
  else {
    *malloc_count += ((ret == NULL) ? 0 : 1);
  }
#endif

#ifdef SC_ENABLE_PTHREAD
  sc_package_unlock (package);
#endif

  return ret;
}

void               *
sc_realloc (int package, void *ptr, size_t size)
{
  if (ptr == NULL) {
    return sc_malloc (package, size);
  }
  else if (size == 0) {
    sc_free (package, ptr);
    return NULL;
  }
  else {
    void               *ret;

#ifdef SC_ENABLE_MEMALIGN
    ret = sc_realloc_aligned (ptr, SC_MEMALIGN_BYTES, size);
#else
    ret = realloc (ptr, size);
    SC_CHECK_ABORTF (ret != NULL, "Reallocation (realloc size %lli)",
                     (long long int) size);
#endif

    return ret;
  }
}

char               *
sc_strdup (int package, const char *s)
{
  size_t              len;
  char               *d;

  if (s == NULL) {
    return NULL;
  }

  len = strlen (s) + 1;
  d = (char *) sc_malloc (package, len);
  memcpy (d, s, len);

  return d;
}

void
sc_free (int package, void *ptr)
{
  if (ptr == NULL) {
    return;
  }
  else {
    /* uncount the allocations */
#ifndef SC_NOCOUNT_MALLOC
    int                *free_count = sc_free_count (package);
#endif

#ifdef SC_ENABLE_PTHREAD
    sc_package_lock (package);
#endif

#ifndef SC_NOCOUNT_MALLOC
    ++*free_count;
#endif

#ifdef SC_ENABLE_PTHREAD
    sc_package_unlock (package);
#endif
  }

  /* free memory */
#ifdef SC_ENABLE_MEMALIGN
  sc_free_aligned (ptr, SC_MEMALIGN_BYTES);
#else
  free (ptr);
#endif
}

int
sc_memory_status (int package)
{
  sc_package_t       *p;

  if (package == -1) {
    return (default_malloc_count - default_free_count);
  }
  else {
    SC_ASSERT (sc_package_is_registered (package));
    p = sc_packages + package;
    return (p->malloc_count - p->free_count);
  }
}

void
sc_package_set_abort_alloc_mismatch (int package_id, int set_abort)
{
  if (package_id == -1) {
    default_abort_mismatch = set_abort;
  }
  else {
    sc_package_t       *p;

    SC_ASSERT (sc_package_is_registered (package_id));
    p = sc_packages + package_id;
    p->abort_mismatch = set_abort;
  }
}

int
sc_memory_check_noabort (int package)
{
  int                 num_errors = 0;

  if (package == -1) {
    if (default_rc_active != 0) {
      SC_LERROR ("Leftover references (default)\n");
      ++num_errors;
    }
    if (default_malloc_count != default_free_count) {
      SC_LERROR ("Memory balance (default)\n");
      ++num_errors;
    }
  }
  else {
    if (!sc_package_is_registered (package)) {
      SC_LERRORF ("Package %d not registered\n", package);
      ++num_errors;
    }
    else {
      sc_package_t       *p = sc_packages + package;

      if (p->rc_active != 0) {
        SC_LERRORF ("Leftover references (%s)\n", p->name);
        ++num_errors;
      }
      if (p->malloc_count != p->free_count) {
        SC_LERRORF ("Memory balance (%s)\n", p->name);
        ++num_errors;
      }
    }
  }
  return num_errors;
}

static int
sc_query_doabort (int package)
{
  if (package == -1) {
    return default_abort_mismatch;
  }
  else if (sc_package_is_registered (package)) {
    return sc_packages[package].abort_mismatch;
  }
  else {
    return 1;
  }
}

void
sc_memory_check (int package)
{
  if (sc_memory_check_noabort (package)) {
    SC_CHECK_ABORT (!sc_query_doabort (package), "Memory and counter check");
  }
}

int
sc_int_compare (const void *v1, const void *v2)
{
  const int           i1 = *(int *) v1;
  const int           i2 = *(int *) v2;

  return i1 == i2 ? 0 : i1 < i2 ? -1 : +1;
}

int
sc_int8_compare (const void *v1, const void *v2)
{
  const int8_t        i1 = *(int8_t *) v1;
  const int8_t        i2 = *(int8_t *) v2;

  return i1 == i2 ? 0 : i1 < i2 ? -1 : +1;
}

int
sc_int16_compare (const void *v1, const void *v2)
{
  const int16_t       i1 = *(int16_t *) v1;
  const int16_t       i2 = *(int16_t *) v2;

  return i1 == i2 ? 0 : i1 < i2 ? -1 : +1;
}

int
sc_int32_compare (const void *v1, const void *v2)
{
  const int32_t       i1 = *(int32_t *) v1;
  const int32_t       i2 = *(int32_t *) v2;

  return i1 == i2 ? 0 : i1 < i2 ? -1 : +1;
}

int
sc_int64_compare (const void *v1, const void *v2)
{
  const int64_t       i1 = *(int64_t *) v1;
  const int64_t       i2 = *(int64_t *) v2;

  return i1 == i2 ? 0 : i1 < i2 ? -1 : +1;
}

int
sc_double_compare (const void *v1, const void *v2)
{
  const double        d1 = *(double *) v1;
  const double        d2 = *(double *) v2;

  return d1 < d2 ? -1 : d1 > d2 ? 1 : 0;
}

int
sc_atoi (const char *nptr)
{
  long                r = strtol (nptr, NULL, 10);
  return r <= INT_MIN ? INT_MIN : r >= INT_MAX ? INT_MAX : (int) r;
}

long
sc_atol (const char *nptr)
{
  return strtol (nptr, NULL, 10);
}

void
sc_set_log_defaults (FILE * log_stream,
                     sc_log_handler_t log_handler, int log_threshold)
{
  sc_default_log_handler = log_handler != NULL ? log_handler : sc_log_handler;

  if (log_threshold == SC_LP_DEFAULT) {
    sc_default_log_threshold = SC_LP_THRESHOLD;
  }
  else {
    SC_ASSERT (log_threshold >= SC_LP_ALWAYS &&
               log_threshold <= SC_LP_SILENT);
    sc_default_log_threshold = log_threshold;
  }

  sc_log_stream = log_stream;
}

void
sc_log (const char *filename, int lineno,
        int package, int category, int priority, const char *msg)
{
  int                 log_threshold;
  sc_log_handler_t    log_handler;
  sc_package_t       *p;

  if (package != -1 && !sc_package_is_registered (package)) {
    package = -1;
  }
  if (package == -1) {
    p = NULL;
    log_threshold = sc_default_log_threshold;
    log_handler = sc_default_log_handler;
  }
  else {
    p = sc_packages + package;
    log_threshold =
      (p->log_threshold ==
       SC_LP_DEFAULT) ? sc_default_log_threshold : p->log_threshold;
    log_handler =
      (p->log_handler == NULL) ? sc_default_log_handler : p->log_handler;
  }
  if (!(category == SC_LC_NORMAL || category == SC_LC_GLOBAL))
    return;
  if (!(priority > SC_LP_ALWAYS && priority < SC_LP_SILENT))
    return;
  if (category == SC_LC_GLOBAL && sc_identifier > 0)
    return;

#ifdef SC_ENABLE_PTHREAD
  sc_package_lock (package);
#endif
  if (sc_trace_file != NULL && priority >= sc_trace_prio)
    log_handler (sc_trace_file, filename, lineno,
                 package, category, priority, msg);

  if (priority >= log_threshold)
    log_handler (sc_log_stream != NULL ? sc_log_stream : stdout,
                 filename, lineno, package, category, priority, msg);
#ifdef SC_ENABLE_PTHREAD
  sc_package_unlock (package);
#endif
}

void
sc_logf (const char *filename, int lineno,
         int package, int category, int priority, const char *fmt, ...)
{
  va_list             ap;

  va_start (ap, fmt);
  sc_logv (filename, lineno, package, category, priority, fmt, ap);
  va_end (ap);
}

void
sc_logv (const char *filename, int lineno,
         int package, int category, int priority, const char *fmt, va_list ap)
{
  char                buffer[BUFSIZ];

#ifdef SC_ENABLE_PTHREAD
  sc_package_lock (package);
#endif
  vsnprintf (buffer, BUFSIZ, fmt, ap);
#ifdef SC_ENABLE_PTHREAD
  sc_package_unlock (package);
#endif
  sc_log (filename, lineno, package, category, priority, buffer);
}

void
sc_log_indent_push (void)
{
  sc_log_indent_push_count (sc_package_id, 1);
}

void
sc_log_indent_pop (void)
{
  sc_log_indent_pop_count (sc_package_id, 1);
}

void
sc_log_indent_push_count (int package, int count)
{
  /* TODO: figure out a version that makes sense with threads */
#ifndef SC_NOCOUNT_LOGINDENT
#ifndef SC_ENABLE_PTHREAD
  SC_ASSERT (package < sc_num_packages);

  if (package >= 0) {
    sc_packages[package].log_indent += SC_MAX (0, count);
  }
#endif
#endif
}

void
sc_log_indent_pop_count (int package, int count)
{
  /* TODO: figure out a version that makes sense with threads */
#ifndef SC_NOCOUNT_LOGINDENT
#ifndef SC_ENABLE_PTHREAD
  int                 new_indent;

  SC_ASSERT (package < sc_num_packages);

  if (package >= 0) {
    new_indent = sc_packages[package].log_indent - SC_MAX (0, count);
    sc_packages[package].log_indent = SC_MAX (0, new_indent);
  }
#endif
#endif
}

void
sc_set_abort_handler (sc_abort_handler_t abort_handler)
{
  sc_default_abort_handler = abort_handler != NULL ? abort_handler :
    sc_abort_handler;
}

void
sc_abort (void)
{
  sc_default_abort_handler ();
  abort ();                     /* if the user supplied callback incorrecty returns, abort */
}

static void
sc_abort_handler (void)
{
  if (0) {
  }
#ifdef SC_BACKTRACE
  else if (sc_print_backtrace) {
    int                 i, bt_size;
    void               *bt_buffer[SC_STACK_SIZE];
    char              **bt_strings;
    const char         *str;

    bt_size = backtrace (bt_buffer, SC_STACK_SIZE);
    bt_strings = backtrace_symbols (bt_buffer, bt_size);

    SC_LERRORF ("Abort: Obtained %d stack frames\n", bt_size);

#ifdef SC_ADDRTOLINE
    /* implement pipe connection to addr2line */
#endif

    for (i = 0; i < bt_size; i++) {
      str = strrchr (bt_strings[i], '/');
      if (str != NULL) {
        ++str;
      }
      else {
        str = bt_strings[i];
      }
      SC_LERRORF ("Stack %d: %s\n", i, str);
    }
    free (bt_strings);
  }
#endif
  else {
    SC_LERROR ("Abort\n");
  }

  fflush (stdout);
  fflush (stderr);
#ifdef _MSC_VER
  Sleep (1);
#else
  sleep (1);                    /* allow time for pending output */
#endif
  if (sc_mpicomm != sc_MPI_COMM_NULL) {
    sc_MPI_Abort (sc_mpicomm, 1);       /* terminate all MPI processes */
  }
  abort ();
}

void
sc_abort_verbose (const char *filename, int lineno, const char *msg)
{
  SC_LERRORF ("Abort: %s\n", msg);
  SC_LERRORF ("Abort: %s:%d\n", filename, lineno);
  sc_abort ();
}

void
sc_abort_verbosef (const char *filename, int lineno, const char *fmt, ...)
{
  va_list             ap;

  va_start (ap, fmt);
  sc_abort_verbosev (filename, lineno, fmt, ap);
  va_end (ap);
}

void
sc_abort_verbosev (const char *filename, int lineno,
                   const char *fmt, va_list ap)
{
  char                buffer[BUFSIZ];

  vsnprintf (buffer, BUFSIZ, fmt, ap);
  sc_abort_verbose (filename, lineno, buffer);
}

void
sc_abort_collective (const char *msg)
{
  int                 mpiret;

  if (sc_mpicomm != sc_MPI_COMM_NULL) {
    mpiret = sc_MPI_Barrier (sc_mpicomm);
    SC_CHECK_MPI (mpiret);
  }

  if (sc_is_root ()) {
    SC_ABORT (msg);
  }
  else {
#ifdef _MSC_VER
    Sleep (3);
#else
    sleep (3);                  /* wait for root rank's sc_MPI_Abort ()... */
#endif
    abort ();                   /* ... otherwise this may call sc_MPI_Abort () */
  }
}

int
sc_package_register (sc_log_handler_t log_handler, int log_threshold,
                     const char *name, const char *full)
{
  int                 i;
  sc_package_t       *p;
  sc_package_t       *new_package = NULL;
  int                 new_package_id = -1;

  SC_CHECK_ABORT (log_threshold == SC_LP_DEFAULT ||
                  (log_threshold >= SC_LP_ALWAYS &&
                   log_threshold <= SC_LP_SILENT),
                  "Invalid package log threshold");
  SC_CHECK_ABORT (strcmp (name, "default"), "Package default forbidden");
  SC_CHECK_ABORT (strchr (name, ' ') == NULL,
                  "Packages name contains spaces");

  /* sc_packages is static and thus initialized to all zeros */
  for (i = 0; i < sc_num_packages_alloc; ++i) {
    p = sc_packages + i;
    SC_CHECK_ABORTF (!p->is_registered || strcmp (p->name, name),
                     "Package %s is already registered", name);
  }

  /* Try to find unused space in sc_packages  */
  for (i = 0; i < sc_num_packages_alloc; ++i) {
    p = sc_packages + i;
    if (!p->is_registered) {
      new_package = p;
      new_package_id = i;
      break;
    }
  }

  /* realloc if the space in sc_packages is used up */
  if (i == sc_num_packages_alloc) {
    sc_packages = (sc_package_t *) realloc (sc_packages,
                                            (2 * sc_num_packages_alloc +
                                             1) * sizeof (sc_package_t));
    SC_CHECK_ABORT (sc_packages, "Failed to allocate memory");
    new_package = sc_packages + i;
    new_package_id = i;
    sc_num_packages_alloc = 2 * sc_num_packages_alloc + 1;

    /* initialize new packages */
    for (; i < sc_num_packages_alloc; i++) {
      p = sc_packages + i;
      p->is_registered = 0;
      p->log_handler = NULL;
      p->log_threshold = SC_LP_SILENT;
      p->log_indent = 0;
      p->malloc_count = 0;
      p->free_count = 0;
      p->rc_active = 0;
      p->name = NULL;
      p->full = NULL;
    }
  }

  new_package->is_registered = 1;
  new_package->log_handler = log_handler;
  new_package->log_threshold = log_threshold;
  new_package->log_indent = 0;
  new_package->malloc_count = 0;
  new_package->free_count = 0;
  new_package->rc_active = 0;
  new_package->abort_mismatch = 1;
  new_package->name = name;
  new_package->full = full;
#ifdef SC_ENABLE_PTHREAD
  i = pthread_mutex_init (&new_package->mutex, NULL);
  SC_CHECK_ABORTF (i == 0, "Mutex init failed for package %s", name);
#endif

  ++sc_num_packages;
  SC_ASSERT (sc_num_packages <= sc_num_packages_alloc);
  SC_ASSERT (0 <= new_package_id && new_package_id < sc_num_packages);

  return new_package_id;
}

int
sc_package_is_registered (int package_id)
{
  if (package_id < 0) {
    SC_LERRORF ("Invalid package id %d\n", package_id);
  }
  return (0 <= package_id && package_id < sc_num_packages_alloc &&
          sc_packages[package_id].is_registered);
}

void
sc_package_set_verbosity (int package_id, int log_priority)
{
  sc_package_t       *p;

  SC_CHECK_ABORT (sc_package_is_registered (package_id),
                  "Package id is not registered");
  SC_CHECK_ABORT (log_priority == SC_LP_DEFAULT ||
                  (log_priority >= SC_LP_ALWAYS &&
                   log_priority <= SC_LP_SILENT),
                  "Invalid package log threshold");

  p = sc_packages + package_id;
  p->log_threshold = log_priority;
}

static int
sc_package_unregister_noabort (int package_id)
{
  int                 num_errors = 0;
  sc_package_t       *p;

  if (!sc_package_is_registered (package_id)) {
    SC_LERRORF ("Package %d not registered\n", package_id);
    ++num_errors;
  }
  else {
    /* examine counter consistency */
    num_errors += sc_memory_check_noabort (package_id);

    /* clean internal package structure */
    p = sc_packages + package_id;
    p->is_registered = 0;
    p->log_handler = NULL;
    p->log_threshold = SC_LP_DEFAULT;
    p->malloc_count = p->free_count = 0;
    p->rc_active = 0;
#ifdef SC_ENABLE_PTHREAD
    if (pthread_mutex_destroy (&p->mutex)) {
      SC_LERRORF ("Mutex destroy failed for package %s", p->name);
      ++num_errors;
    }
#endif
    p->name = p->full = NULL;
    --sc_num_packages;
  }
  return num_errors;
}

void
sc_package_unregister (int package_id)
{
  if (sc_package_unregister_noabort (package_id)) {
    SC_CHECK_ABORTF (!sc_query_doabort (package_id),
                     "Unregistering package %d", package_id);
  }
}

void
sc_package_print_summary (int log_priority)
{
  int                 i;
  sc_package_t       *p;

  SC_GEN_LOGF (sc_package_id, SC_LC_GLOBAL, log_priority,
               "Package summary (%d total):\n", sc_num_packages);

  /* sc_packages is static and thus initialized to all zeros */
  for (i = 0; i < sc_num_packages_alloc; ++i) {
    p = sc_packages + i;
    if (p->is_registered) {
      SC_GEN_LOGF (sc_package_id, SC_LC_GLOBAL, log_priority,
                   "   %3d: %-15s +%d-%d   %s\n",
                   i, p->name, p->malloc_count, p->free_count, p->full);
    }
  }
}

void
sc_init (sc_MPI_Comm mpicomm,
         int catch_signals, int print_backtrace,
         sc_log_handler_t log_handler, int log_threshold)
{
  const char         *trace_file_name;
  const char         *trace_file_prio;

  sc_identifier = -1;
  sc_mpicomm = sc_MPI_COMM_NULL;
  sc_print_backtrace = print_backtrace;

  if (mpicomm != sc_MPI_COMM_NULL) {
    int                 mpiret;

    sc_mpicomm = mpicomm;
    mpiret = sc_MPI_Comm_rank (sc_mpicomm, &sc_identifier);
    SC_CHECK_MPI (mpiret);
  }

  sc_set_signal_handler (catch_signals);
  sc_package_id = sc_package_register (log_handler, log_threshold,
                                       "libsc", "The SC Library");

  trace_file_name = getenv ("SC_TRACE_FILE");
  if (trace_file_name != NULL) {
    char                buffer[BUFSIZ];

    if (sc_identifier >= 0) {
      snprintf (buffer, BUFSIZ, "%s.%d.log", trace_file_name, sc_identifier);
    }
    else {
      snprintf (buffer, BUFSIZ, "%s.log", trace_file_name);
    }
    SC_CHECK_ABORT (sc_trace_file == NULL, "Trace file not NULL");
    sc_trace_file = fopen (buffer, "wb");
    SC_CHECK_ABORT (sc_trace_file != NULL, "Trace file open");

    trace_file_prio = getenv ("SC_TRACE_LP");
    if (trace_file_prio != NULL) {
      if (!strcmp (trace_file_prio, "SC_LP_TRACE")) {
        sc_trace_prio = SC_LP_TRACE;
      }
      else if (!strcmp (trace_file_prio, "SC_LP_DEBUG")) {
        sc_trace_prio = SC_LP_DEBUG;
      }
      else if (!strcmp (trace_file_prio, "SC_LP_VERBOSE")) {
        sc_trace_prio = SC_LP_VERBOSE;
      }
      else if (!strcmp (trace_file_prio, "SC_LP_INFO")) {
        sc_trace_prio = SC_LP_INFO;
      }
      else if (!strcmp (trace_file_prio, "SC_LP_STATISTICS")) {
        sc_trace_prio = SC_LP_STATISTICS;
      }
      else if (!strcmp (trace_file_prio, "SC_LP_PRODUCTION")) {
        sc_trace_prio = SC_LP_PRODUCTION;
      }
      else if (!strcmp (trace_file_prio, "SC_LP_ESSENTIAL")) {
        sc_trace_prio = SC_LP_ESSENTIAL;
      }
      else if (!strcmp (trace_file_prio, "SC_LP_ERROR")) {
        sc_trace_prio = SC_LP_ERROR;
      }
      else {
        SC_ABORT ("Invalid trace priority");
      }
    }
  }

  /* one line of logging if the threshold is not SC_LP_SILENT */
  SC_GLOBAL_ESSENTIALF ("This is %s\n", SC_PACKAGE_STRING);
  SC_GLOBAL_INFOF ("MPI is enabled %d shared %d\n",
                   sc_mpi_is_enabled (), sc_mpi_is_shared ());
  sc_initialized = 1;
}

int
sc_is_initialized (void)
{
  return sc_initialized;
}

int
sc_finalize_noabort (void)
{
  int                 i;
  int                 num_errors = 0;

  /* sc_packages is static and thus initialized to all zeros */
  for (i = sc_num_packages_alloc - 1; i >= 0; --i)
    if (sc_packages[i].is_registered)
      num_errors += sc_package_unregister_noabort (i);

  SC_ASSERT (sc_num_packages == 0);
  num_errors += sc_memory_check_noabort (-1);

  free (sc_packages);
  sc_packages = NULL;
  sc_num_packages_alloc = 0;

  /* with this argument the function will never abort */
  sc_set_signal_handler (0);
  sc_mpicomm = sc_MPI_COMM_NULL;

  sc_print_backtrace = 0;
  sc_identifier = -1;

  /* close trace file */
  if (sc_trace_file != NULL) {
    if (fclose (sc_trace_file)) {
      SC_LERROR ("Trace file close");
      ++num_errors;
    }
    sc_trace_file = NULL;
  }

  sc_package_id = -1;
  sc_initialized = 0;

  return num_errors;
}

void
sc_finalize (void)
{
  SC_CHECK_ABORT (!sc_finalize_noabort () ||
                  !default_abort_mismatch, "Finalize");
}

int
sc_is_root (void)
{
  return sc_identifier <= 0;
}

/* enable logging for files compiled with C++ */

#ifndef __cplusplus
#undef SC_ABORTF
#undef SC_CHECK_ABORTF
#undef SC_GEN_LOGF
#undef SC_GLOBAL_LOGF
#undef SC_LOGF
#undef SC_GLOBAL_TRACEF
#undef SC_GLOBAL_LDEBUGF
#undef SC_GLOBAL_VERBOSEF
#undef SC_GLOBAL_INFOF
#undef SC_GLOBAL_STATISTICSF
#undef SC_GLOBAL_PRODUCTIONF
#undef SC_GLOBAL_ESSENTIALF
#undef SC_GLOBAL_LERRORF
#undef SC_TRACEF
#undef SC_LDEBUGF
#undef SC_VERBOSEF
#undef SC_INFOF
#undef SC_STATISTICSF
#undef SC_PRODUCTIONF
#undef SC_ESSENTIALF
#undef SC_LERRORF
#endif

void
SC_ABORTF (const char *fmt, ...)
{
  va_list             ap;

  va_start (ap, fmt);
  sc_abort_verbosev ("<unknown>", 0, fmt, ap);
  va_end (ap);
}

void
SC_CHECK_ABORTF (int success, const char *fmt, ...)
{
  va_list             ap;

  if (!success) {
    va_start (ap, fmt);
    sc_abort_verbosev ("<unknown>", 0, fmt, ap);
    va_end (ap);
  }
}

void
SC_GEN_LOGF (int package, int category, int priority, const char *fmt, ...)
{
  va_list             ap;

  va_start (ap, fmt);
  sc_logv ("<unknown>", 0, package, category, priority, fmt, ap);
  va_end (ap);
}

void
SC_GLOBAL_LOGF (int priority, const char *fmt, ...)
{
  va_list             ap;

  va_start (ap, fmt);
  sc_logv ("<unknown>", 0, sc_package_id, SC_LC_GLOBAL, priority, fmt, ap);
  va_end (ap);
}

void
SC_LOGF (int priority, const char *fmt, ...)
{
  va_list             ap;

  va_start (ap, fmt);
  sc_logv ("<unknown>", 0, sc_package_id, SC_LC_NORMAL, priority, fmt, ap);
  va_end (ap);
}

#define SC_LOG_IMP(n,p)                                 \
  void                                                  \
  SC_GLOBAL_ ## n ## F (const char *fmt, ...)           \
  {                                                     \
    va_list             ap;                             \
    va_start (ap, fmt);                                 \
    sc_logv ("<unknown>", 0, sc_package_id,             \
             SC_LC_GLOBAL, SC_LP_ ## p, fmt, ap);       \
    va_end (ap);                                        \
  }                                                     \
  void                                                  \
  SC_ ## n ## F (const char *fmt, ...)                  \
  {                                                     \
    va_list             ap;                             \
    va_start (ap, fmt);                                 \
    sc_logv ("<unknown>", 0, sc_package_id,             \
             SC_LC_NORMAL, SC_LP_ ## p, fmt, ap);       \
    va_end (ap);                                        \
  }

/* *INDENT-OFF* */
SC_LOG_IMP (TRACE, TRACE)
SC_LOG_IMP (LDEBUG, DEBUG)
SC_LOG_IMP (VERBOSE, VERBOSE)
SC_LOG_IMP (INFO, INFO)
SC_LOG_IMP (STATISTICS, STATISTICS)
SC_LOG_IMP (PRODUCTION, PRODUCTION)
SC_LOG_IMP (ESSENTIAL, ESSENTIAL)
SC_LOG_IMP (LERROR, ERROR)
/* *INDENT-ON* */

void
sc_strcopy (char *dest, size_t size, const char *src)
{
  sc_snprintf (dest, size, "%s", src);
}

void
sc_snprintf (char *str, size_t size, const char *fmt, ...)
{
  int                 retval;
  va_list             ap;

  /* If there is no space, we do not access the buffer at all.
     Further down we expect it to be at least 1 byte wide */
  if (str == NULL || size == 0) {
    return;
  }

  /* avoid uninitialized bytes if strlen (src) < size - 1 */
  memset (str, 0, size);

  /* Writing this function just to catch the return value.
     Avoiding -Wnoformat-truncation gcc option this way */
  va_start (ap, fmt);
  retval = vsnprintf (str, size, fmt, ap);
  if (retval < 0) {
    str[0] = '\0';
  }
  /* We do not handle truncation, since it is expected in our design. */
  va_end (ap);
}

const char         *
sc_version (void)
{
  return SC_VERSION;
}

int
sc_version_major (void)
{
  /* In rare cases SC_VERSION_MINOR may be a non-numerical string */
  return sc_atoi (SC_TOSTRING (SC_VERSION_MAJOR));
}

int
sc_version_minor (void)
{
  /* In rare cases SC_VERSION_MINOR may be a non-numerical string */
  return sc_atoi (SC_TOSTRING (SC_VERSION_MINOR));
}

#if 0
int
sc_version_point (void)
{
  /* SC_VERSION_POINT may contain a dot and/or dash,
     followed by additional information */
  return sc_atoi (SC_TOSTRING (SC_VERSION_POINT));
}
#endif

int
sc_is_littleendian (void)
{
  /* We use the volatile keyword to deactivate compiler optimizations related
   * to the variable uint.
   */
  const volatile uint32_t uint = 1;
  return *(char *) &uint == 1;
}

int
sc_have_zlib (void)
{
#ifndef SC_HAVE_ZLIB
  return 0;
#else
  return 1;
#endif
}

int
sc_have_json (void)
{
#ifndef SC_HAVE_JSON
  return 0;
#else
  return 1;
#endif
}

void
sc_sleep (unsigned milliseconds){
#if _POSIX_C_SOURCE >= 199309L
  struct timespec ts;
  /* full seconds */
  ts.tv_sec = milliseconds / 1000;
  /* nanoseconds */
  ts.tv_nsec = (milliseconds % 1000) * 1000000;
  nanosleep (&ts, NULL);
#elif defined(_POSIX_C_SOURCE)
  /* older POSIX */
  if (milliseconds >= 1000) {
    sleep (milliseconds / 1000);
  }
  usleep ((milliseconds % 1000) * 1000);
#elif _MSC_VER
  Sleep (milliseconds);
#else
  SC_ABORT ("No suitable sleep function available.");
#endif
}
