/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2008,2009 Carsten Burstedde, Lucas Wilcox.

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

/* sc.h comes first in every compilation unit */
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

#define SC_MAX_PACKAGES 128

typedef struct sc_package
{
  bool                is_registered;
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
static void         sc_log_handler (const char *filename, int lineno,
                                    int package, int category, int priority,
                                    const char *fmt, va_list ap);

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

static long         sc_vp_default_key[2];
void               *SC_VP_DEFAULT  = (void *) &sc_vp_default_key[0];
/*@access FILE@*/
FILE               *SC_FP_KEEP     = (FILE *) &sc_vp_default_key[1];
int                 sc_package_id  = -1;
/* *INDENT-ON* */

static int          default_malloc_count = 0;
static int          default_free_count = 0;

static int          sc_identifier = -1;

static int          sc_default_log_threshold = SC_LP_THRESHOLD;
static sc_log_handler_t sc_default_log_handler = sc_log_handler;

static FILE        *sc_log_stream = NULL;
static bool         sc_log_stream_set = false;

static bool         signals_caught = false;
static sc_sig_t     system_int_handler = NULL;
static sc_sig_t     system_segv_handler = NULL;
static sc_sig_t     system_usr2_handler = NULL;

static sc_handler_t sc_abort_handler = NULL;
static void        *sc_abort_data = NULL;

static int          sc_num_packages = 0;
static sc_package_t sc_packages[SC_MAX_PACKAGES];

static void
sc_signal_handler (int sig)
{
  char                prefix[BUFSIZ];
  char               *sigstr;

  if (sc_identifier >= 0) {
    snprintf (prefix, BUFSIZ, "[%d] ", sc_identifier);
  }
  else {
    prefix[0] = '\0';
  }

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
  fprintf (stderr, "%sAbort: Signal %s\n", prefix, sigstr);

  sc_abort ();
}

static void
sc_log_handler (const char *filename, int lineno,
                int package, int category, int priority,
                const char *fmt, va_list ap)
{
  bool                wp = false, wi = false;

  if (sc_log_stream == NULL && !sc_log_stream_set) {
    sc_log_stream = stdout;
    sc_log_stream_set = true;
  }
  if (sc_log_stream == NULL)
    return;

  if (package != -1) {
    SC_ASSERT (sc_package_is_registered (package));
    wp = true;
  }
  wi = (category == SC_LC_NORMAL && sc_identifier >= 0);

  if (wp || wi) {
    fputc ('[', sc_log_stream);
    if (wp)
      fprintf (sc_log_stream, "%s", sc_packages[package].name);
    if (wp && wi)
      fputc (' ', sc_log_stream);
    if (wi)
      fprintf (sc_log_stream, "%d", sc_identifier);
    fputs ("] ", sc_log_stream);
  }

  if (priority == SC_LP_TRACE) {
    char                bn[BUFSIZ], *bp;

    snprintf (bn, BUFSIZ, "%s", filename);
    bp = basename (bn);
    fprintf (sc_log_stream, "%s:%d ", bp, lineno);
  }

  vfprintf (sc_log_stream, fmt, ap);
  fflush (sc_log_stream);
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
  d = sc_malloc (package, len);
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
  return *(int *) v1 - *(int *) v2;
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
sc_set_log_defaults (sc_log_handler_t log_handler, int log_threshold,
                     FILE * log_stream)
{
  sc_default_log_handler =
    (log_handler == NULL ? sc_log_handler : log_handler);

  if (log_threshold == SC_LP_DEFAULT) {
    sc_default_log_threshold = SC_LP_THRESHOLD;
  }
  else {
    SC_ASSERT (log_threshold >= SC_LP_NONE && log_threshold <= SC_LP_SILENT);
    sc_default_log_threshold = log_threshold;
  }

  if (log_stream != SC_FP_KEEP) {
    sc_log_stream = log_stream;
    sc_log_stream_set = true;
  }
}

void
sc_logf (const char *filename, int lineno,
         int package, int category, int priority, const char *fmt, ...)
{
  int                 log_threshold;
  sc_log_handler_t    log_handler;
  sc_package_t       *p;
  va_list             ap;

  if (package == -1) {
    p = NULL;
    log_threshold = sc_default_log_threshold;
    log_handler = sc_default_log_handler;
  }
  else {
    SC_ASSERT (sc_package_is_registered (package));
    p = sc_packages + package;
    log_threshold =
      (p->log_threshold ==
       SC_LP_DEFAULT) ? sc_default_log_threshold : p->log_threshold;
    log_handler =
      (p->log_handler == NULL) ? sc_default_log_handler : p->log_handler;
  }
  SC_ASSERT (category == SC_LC_NORMAL || category == SC_LC_GLOBAL);
  SC_ASSERT (priority >= SC_LP_NONE && priority < SC_LP_SILENT);

  if (category == SC_LC_GLOBAL && sc_identifier > 0)
    return;
  if (priority < log_threshold)
    return;

  va_start (ap, fmt);
  log_handler (filename, lineno, package, category, priority, fmt, ap);
  va_end (ap);
}

void
sc_generic_abort (void *data)
{
  int                 rank;
  MPI_Comm           *mpicomm = data;

  if (mpicomm == NULL) {
    fprintf (stderr, "Error: sc_generic_abort needs MPI_Comm* data!\n");
  }
  else {
    rank = -1;
    MPI_Comm_rank (*mpicomm, &rank);
    fprintf (stderr, "[%d] generic MPI abort handler\n", rank);
    MPI_Abort (*mpicomm, 1);
  }
}

void
sc_set_abort_handler (sc_handler_t handler, void *data)
{
  sc_abort_handler = handler;
  sc_abort_data = data;

  if (handler != NULL && !signals_caught) {
    system_int_handler = signal (SIGINT, sc_signal_handler);
    SC_CHECK_ABORT (system_int_handler != SIG_ERR, "catching INT");
    system_segv_handler = signal (SIGSEGV, sc_signal_handler);
    SC_CHECK_ABORT (system_segv_handler != SIG_ERR, "catching SEGV");
    system_usr2_handler = signal (SIGUSR2, sc_signal_handler);
    SC_CHECK_ABORT (system_usr2_handler != SIG_ERR, "catching USR2");
    signals_caught = true;
  }
  else if (handler == NULL && signals_caught) {
    (void) signal (SIGINT, system_int_handler);
    system_int_handler = NULL;
    (void) signal (SIGSEGV, system_segv_handler);
    system_segv_handler = NULL;
    (void) signal (SIGUSR2, system_usr2_handler);
    system_usr2_handler = NULL;
    signals_caught = false;
  }
}

void
sc_abort (void)
{
  char                prefix[BUFSIZ];
#ifdef SC_BACKTRACE
  int                 i, bt_size;
  void               *bt_buffer[SC_STACK_SIZE];
  char              **bt_strings;
  const char         *str;
#endif

  if (sc_identifier >= 0) {
    snprintf (prefix, BUFSIZ, "[%d] ", sc_identifier);
  }
  else {
    prefix[0] = '\0';
  }

#ifdef SC_BACKTRACE
  bt_size = backtrace (bt_buffer, SC_STACK_SIZE);
  bt_strings = backtrace_symbols (bt_buffer, bt_size);

  fprintf (stderr, "%sAbort: Obtained %d stack frames\n", prefix, bt_size);

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
    /* fprintf (stderr, "   %p %s\n", bt_buffer[i], str); */
    fprintf (stderr, "%s   %s\n", prefix, str);
  }
  free (bt_strings);
#endif /* SC_BACKTRACE */

  fflush (stdout);
  fflush (stderr);
  sleep (1);

  if (sc_abort_handler != NULL) {
    sc_abort_handler (sc_abort_data);
  }
  abort ();
}

int
sc_package_register (sc_log_handler_t log_handler, int log_threshold,
                     const char *name, const char *full)
{
  int                 i;
  sc_package_t       *p;

  SC_CHECK_ABORT (sc_num_packages < SC_MAX_PACKAGES, "Too many packages");
  SC_CHECK_ABORT (log_threshold == SC_LP_DEFAULT ||
                  (log_threshold >= SC_LP_NONE
                   && log_threshold <= SC_LP_SILENT),
                  "Invalid package log threshold");
  SC_CHECK_ABORT (strcmp (name, "default"), "Package default forbidden");
  SC_CHECK_ABORT (strchr (name, ' ') == NULL,
                  "Packages name contains spaces");

  /* sc_packages is static and thus initialized to all zeros */
  for (i = 0; i < SC_MAX_PACKAGES; ++i) {
    p = sc_packages + i;
    SC_CHECK_ABORTF (!p->is_registered || strcmp (p->name, name),
                     "Package %s is already registered", name);
  }
  for (i = 0; i < SC_MAX_PACKAGES; ++i) {
    p = sc_packages + i;
    if (!p->is_registered) {
      p->is_registered = true;
      p->log_handler = log_handler;
      p->log_threshold = log_threshold;
      p->malloc_count = p->free_count = 0;
      p->name = name;
      p->full = full;
      break;
    }
  }
  SC_ASSERT (i < SC_MAX_PACKAGES);

  ++sc_num_packages;
  SC_ASSERT (sc_num_packages <= SC_MAX_PACKAGES);

  return i;
}

bool
sc_package_is_registered (int package_id)
{
  SC_CHECK_ABORT (0 <= package_id && package_id < SC_MAX_PACKAGES,
                  "Invalid package id");

  /* sc_packages is static and thus initialized to all zeros */
  return sc_packages[package_id].is_registered;
}

void
sc_package_unregister (int package_id)
{
  sc_package_t       *p;

  SC_CHECK_ABORT (sc_package_is_registered (package_id),
                  "Package not registered");
  sc_memory_check (package_id);

  p = sc_packages + package_id;
  p->is_registered = false;
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

  SC_LOGF (sc_package_id, SC_LC_GLOBAL, log_priority,
           "Package summary (%d total):\n", sc_num_packages);

  /* sc_packages is static and thus initialized to all zeros */
  for (i = 0; i < SC_MAX_PACKAGES; ++i) {
    p = sc_packages + i;
    if (p->is_registered) {
      SC_LOGF (sc_package_id, SC_LC_GLOBAL, log_priority,
               "   %3d: %-15s +%d-%d   %s\n",
               i, p->name, p->malloc_count, p->free_count, p->full);
    }
  }
}

void
sc_init (int identifier,
         sc_handler_t abort_handler, void *abort_data,
         sc_log_handler_t log_handler, int log_threshold)
{
  int                 w;
  const char        **on, **ov;
  const char         *overrides[] = { SC_OVERRIDES NULL, NULL };

  sc_identifier = identifier;
  sc_set_abort_handler (abort_handler, abort_data);
  sc_package_id = sc_package_register (log_handler, log_threshold,
                                       "libsc", "The SC Library");

  w = 24;
  SC_GLOBAL_PRODUCTIONF ("%s\n", SC_PACKAGE_STRING);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "CC", SC_CC);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "C_VERSION", SC_C_VERSION);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "CFLAGS", SC_CFLAGS);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "CPP", SC_CPP);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "CPPFLAGS", SC_CPPFLAGS);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "F77", SC_F77);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "FFLAGS", SC_FFLAGS);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "LINK", SC_LINK);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "LDFLAGS", SC_LDFLAGS);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "BLAS_LIBS", SC_BLAS_LIBS);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "LAPACK_LIBS", SC_LAPACK_LIBS);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "LIBS", SC_LIBS);
  SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, "FLIBS", SC_FLIBS);

  w = 32;
  for (on = overrides; *on != NULL; on = ov + 1) {
    ov = on + 1;
    SC_CHECK_ABORT (*ov != NULL, "SC_OVERRIDES should contain pairs");
    SC_GLOBAL_PRODUCTIONF ("%-*s %s\n", w, *on, *ov);
  }
}

void
sc_finalize (void)
{
  int                 i;

  /* sc_packages is static and thus initialized to all zeros */
  for (i = 0; i < SC_MAX_PACKAGES; ++i)
    if (sc_packages[i].is_registered)
      sc_package_unregister (i);

  SC_ASSERT (sc_num_packages == 0);
  sc_memory_check (-1);

  sc_set_abort_handler (NULL, NULL);

  sc_identifier = -1;
}
