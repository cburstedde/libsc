/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System

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

#include <sc.h>

#if defined SC_ALLOC_PAGE || defined SC_ALLOC_LINE
#define SC_ALLOC_ALIGN
#endif

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

typedef struct sc_package
{
  int                 is_registered;
  sc_log_handler_t    log_handler;
  int                 log_threshold;
  int                 malloc_count;
  int                 free_count;
  const char         *name;
  const char         *full;
}
sc_package_t;

#ifdef SC_ALLOC_ALIGN
static const size_t sc_page_bytes = 4096;
#endif
#ifdef SC_ALLOC_LINE
static const size_t sc_line_bytes = 64;
static const size_t sc_line_count = 4096 / 64;
static size_t       sc_line_no = 0;
#endif

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
FILE               *sc_trace_file = NULL;
int                 sc_trace_prio = SC_LP_STATISTICS;

static int          default_malloc_count = 0;
static int          default_free_count = 0;

static int          sc_identifier = -1;
static MPI_Comm     sc_mpicomm = MPI_COMM_NULL;

static FILE        *sc_log_stream = NULL;
static sc_log_handler_t sc_default_log_handler = sc_log_handler;
static int          sc_default_log_threshold = SC_LP_THRESHOLD;

static int          sc_signals_caught = 0;
static sc_sig_t     system_int_handler = NULL;
static sc_sig_t     system_segv_handler = NULL;
static sc_sig_t     system_usr2_handler = NULL;

static int          sc_print_backtrace = 0;

static int          sc_num_packages = 0;
static int          sc_num_packages_alloc = 0;
static sc_package_t *sc_packages = NULL;

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
  case SIGUSR2:
    sigstr = "USR2";
    break;
  default:
    sigstr = "<unknown>";
    break;
  }
  SC_LERRORF ("Caught signal %s\n", sigstr);

  sc_abort ();
}

/** Installs or removes a signal handler for INT SEGV USR2 that aborts.
 * \param [in] catch    If true, catch signals INT SEGV USR2.
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
    system_usr2_handler = signal (SIGUSR2, sc_signal_handler);
    SC_CHECK_ABORT (system_usr2_handler != SIG_ERR, "catching USR2");
    sc_signals_caught = 1;
  }
  else if (!catch_signals && sc_signals_caught) {
    (void) signal (SIGINT, system_int_handler);
    system_int_handler = NULL;
    (void) signal (SIGSEGV, system_segv_handler);
    system_segv_handler = NULL;
    (void) signal (SIGUSR2, system_usr2_handler);
    system_usr2_handler = NULL;
    sc_signals_caught = 0;
  }
}

static void
sc_log_handler (FILE * log_stream, const char *filename, int lineno,
                int package, int category, int priority, const char *msg)
{
  int                 wp = 0, wi = 0;

  if (package != -1) {
    if (!sc_package_is_registered (package))
      package = -1;
    else
      wp = 1;
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
    fputs ("] ", log_stream);
  }

  if (priority == SC_LP_TRACE) {
    char                bn[BUFSIZ], *bp;

    snprintf (bn, BUFSIZ, "%s", filename);
    bp = basename (bn);
    fprintf (log_stream, "%s:%d ", bp, lineno);
  }

  fputs (msg, log_stream);
  fflush (log_stream);
}

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

void               *
sc_malloc (int package, size_t size)
{
  void               *ret;
  int                *malloc_count = sc_malloc_count (package);

#ifdef SC_ALLOC_ALIGN
  size_t              aligned;

  size += sc_page_bytes;
#endif

  ret = malloc (size);

  if (size > 0) {
    SC_CHECK_ABORT (ret != NULL, "Allocation");
    ++*malloc_count;
  }
  else {
    *malloc_count += ((ret == NULL) ? 0 : 1);
  }

#ifdef SC_ALLOC_PAGE
  aligned = (((size_t) ret + sizeof (size_t) + sc_page_bytes - 1) /
             sc_page_bytes) * sc_page_bytes;
#endif
#ifdef SC_ALLOC_LINE
  aligned = (((size_t) ret + sizeof (size_t) +
              sc_page_bytes - sc_line_no * sc_line_bytes - 1) /
             sc_page_bytes) * sc_page_bytes + sc_line_no * sc_line_bytes;
  sc_line_no = (sc_line_no + 1) % sc_line_count;
#endif
#ifdef SC_ALLOC_ALIGN
  SC_ASSERT (aligned >= (size_t) ret + sizeof (size_t));
  SC_ASSERT (aligned <= (size_t) ret + sc_page_bytes);
  ((size_t *) aligned)[-1] = (size_t) ret;
  ret = (void *) aligned;
#endif

  return ret;
}

void               *
sc_calloc (int package, size_t nmemb, size_t size)
{
  void               *ret;
  int                *malloc_count = sc_malloc_count (package);

#ifdef SC_ALLOC_ALIGN
  size_t              aligned;

  if (size == 0) {
    return NULL;
  }
  nmemb += (sc_page_bytes + size - 1) / size;
#endif

  ret = calloc (nmemb, size);

  if (nmemb * size > 0) {
    SC_CHECK_ABORT (ret != NULL, "Allocation");
    ++*malloc_count;
  }
  else {
    *malloc_count += ((ret == NULL) ? 0 : 1);
  }

#ifdef SC_ALLOC_PAGE
  aligned = (((size_t) ret + sizeof (size_t) + sc_page_bytes - 1) /
             sc_page_bytes) * sc_page_bytes;
#endif
#ifdef SC_ALLOC_LINE
  aligned = (((size_t) ret + sizeof (size_t) +
              sc_page_bytes - sc_line_no * sc_line_bytes - 1) /
             sc_page_bytes) * sc_page_bytes + sc_line_no * sc_line_bytes;
  sc_line_no = (sc_line_no + 1) % sc_line_count;
#endif
#ifdef SC_ALLOC_ALIGN
  SC_ASSERT (aligned >= (size_t) ret + sizeof (size_t));
  SC_ASSERT (aligned <= (size_t) ret + sc_page_bytes);
  ((size_t *) aligned)[-1] = (size_t) ret;
  ret = (void *) aligned;
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

#ifdef SC_ALLOC_ALIGN
    size_t              sptr;
    size_t              shift;
    size_t              aligned;

    sptr = (size_t) ptr;
    ptr = (void *) ((size_t *) ptr)[-1];
    SC_ASSERT (ptr != NULL && sptr >= (size_t) ptr + sizeof (size_t));
    shift = sptr - (size_t) ptr;

    size += sc_page_bytes;
#endif

    ret = realloc (ptr, size);
    SC_CHECK_ABORT (ret != NULL, "Reallocation");

#ifdef SC_ALLOC_ALIGN
    aligned = (size_t) ret + shift;
    SC_ASSERT (aligned >= (size_t) ret + sizeof (size_t));
    SC_ASSERT (aligned <= (size_t) ret + sc_page_bytes);
    SC_ASSERT (((size_t *) aligned)[-1] == (size_t) ptr);
    ((size_t *) aligned)[-1] = (size_t) ret;
    ret = (void *) aligned;
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
  if (ptr != NULL) {
    int                *free_count = sc_free_count (package);
    ++*free_count;

#ifdef SC_ALLOC_ALIGN
    ptr = (void *) ((size_t *) ptr)[-1];
    SC_ASSERT (ptr != NULL);
#endif
  }
  free (ptr);
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
sc_memory_check (int package)
{
  sc_package_t       *p;

  if (package == -1)
    SC_CHECK_ABORT (default_malloc_count == default_free_count,
                    "Memory balance (default)");
  else {
    SC_ASSERT (sc_package_is_registered (package));
    p = sc_packages + package;
    SC_CHECK_ABORTF (p->malloc_count == p->free_count,
                     "Memory balance (%s)", p->name);
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

void
sc_set_log_defaults (FILE * log_stream,
                     sc_log_handler_t log_handler, int log_threshold)
{
  sc_default_log_handler = log_handler != NULL ? log_handler : sc_log_handler;

  if (log_threshold == SC_LP_DEFAULT) {
    sc_default_log_threshold = SC_LP_THRESHOLD;
  }
  else {
    SC_ASSERT (log_threshold >= SC_LP_ALWAYS
               && log_threshold <= SC_LP_SILENT);
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

  if (sc_trace_file != NULL && priority >= sc_trace_prio)
    log_handler (sc_trace_file, filename, lineno,
                 package, category, priority, msg);

  if (priority >= log_threshold)
    log_handler (sc_log_stream != NULL ? sc_log_stream : stdout,
                 filename, lineno, package, category, priority, msg);
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

  vsnprintf (buffer, BUFSIZ, fmt, ap);
  sc_log (filename, lineno, package, category, priority, buffer);
}

void
sc_abort (void)
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
  sleep (1);                    /* allow time for pending output */

  if (sc_mpicomm != MPI_COMM_NULL) {
    MPI_Abort (sc_mpicomm, 1);  /* terminate all MPI processes */
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

  if (sc_mpicomm != MPI_COMM_NULL) {
    mpiret = MPI_Barrier (sc_mpicomm);
    SC_CHECK_MPI (mpiret);
  }

  if (sc_is_root ()) {
    SC_ABORT (msg);
  }
  else {
    sleep (3);                  /* wait for root rank's MPI_Abort ()... */
    abort ();                   /* ... otherwise this may call MPI_Abort () */
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
                  (log_threshold >= SC_LP_ALWAYS
                   && log_threshold <= SC_LP_SILENT),
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
      p->malloc_count = 0;
      p->free_count = 0;
      p->name = NULL;
      p->full = NULL;
    }
  }

  new_package->is_registered = 1;
  new_package->log_handler = log_handler;
  new_package->log_threshold = log_threshold;
  new_package->malloc_count = 0;
  new_package->free_count = 0;
  new_package->name = name;
  new_package->full = full;

  ++sc_num_packages;
  SC_ASSERT (sc_num_packages <= sc_num_packages_alloc);
  SC_ASSERT (0 <= new_package_id && new_package_id < sc_num_packages);

  return new_package_id;
}

int
sc_package_is_registered (int package_id)
{
  SC_CHECK_ABORT (0 <= package_id, "Invalid package id");

  return (package_id < sc_num_packages_alloc &&
          sc_packages[package_id].is_registered);
}

void
sc_package_unregister (int package_id)
{
  sc_package_t       *p;

  SC_CHECK_ABORT (sc_package_is_registered (package_id),
                  "Package not registered");
  sc_memory_check (package_id);

  p = sc_packages + package_id;
  p->is_registered = 0;
  p->log_handler = NULL;
  p->log_threshold = SC_LP_DEFAULT;
  p->malloc_count = p->free_count = 0;
  p->name = p->full = NULL;

  --sc_num_packages;
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
sc_init (MPI_Comm mpicomm,
         int catch_signals, int print_backtrace,
         sc_log_handler_t log_handler, int log_threshold)
{
  int                 w;
  const char         *trace_file_name;
  const char         *trace_file_prio;

  sc_identifier = -1;
  sc_mpicomm = MPI_COMM_NULL;
  sc_print_backtrace = print_backtrace;

  if (mpicomm != MPI_COMM_NULL) {
    int                 mpiret;

    sc_mpicomm = mpicomm;
    mpiret = MPI_Comm_rank (sc_mpicomm, &sc_identifier);
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

  w = 24;
  SC_GLOBAL_ESSENTIALF ("This is %s\n", SC_PACKAGE_STRING);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "CC", SC_CC);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "C_VERSION", SC_C_VERSION);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "CFLAGS", SC_CFLAGS);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "CPP", SC_CPP);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "CPPFLAGS", SC_CPPFLAGS);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "F77", SC_F77);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "FFLAGS", SC_FFLAGS);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "LDFLAGS", SC_LDFLAGS);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "BLAS_LIBS", SC_BLAS_LIBS);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "LAPACK_LIBS", SC_LAPACK_LIBS);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "LIBS", SC_LIBS);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "FLIBS", SC_FLIBS);
}

void
sc_finalize (void)
{
  int                 i;
  int                 retval;

  /* sc_packages is static and thus initialized to all zeros */
  for (i = sc_num_packages_alloc - 1; i >= 0; --i)
    if (sc_packages[i].is_registered)
      sc_package_unregister (i);

  SC_ASSERT (sc_num_packages == 0);
  sc_memory_check (-1);

  free (sc_packages);
  sc_packages = NULL;
  sc_num_packages_alloc = 0;

  sc_set_signal_handler (0);
  sc_mpicomm = MPI_COMM_NULL;

  sc_print_backtrace = 0;
  sc_identifier = -1;

  /* close trace file */
  if (sc_trace_file != NULL) {
    retval = fclose (sc_trace_file);
    SC_CHECK_ABORT (!retval, "Trace file close");

    sc_trace_file = NULL;
  }
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

#ifndef SC_SPLINT

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

#endif
