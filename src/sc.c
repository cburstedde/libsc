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

/* sc.h comes first in every compilation unit */
#include <sc.h>

#ifdef SC_HAVE_SIGNAL_H
#include <signal.h>
#endif

typedef void (*sc_sig_t) (int);

#ifdef SC_HAVE_BACKTRACE
#ifdef SC_HAVE_BACKTRACE_SYMBOLS
#ifdef SC_HAVE_EXECINFO_H
#include <execinfo.h>
#define SC_BACKTRACE
#define SC_STACK_SIZE 64
#endif
#endif
#endif

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

FILE               *sc_root_stdout = NULL;
FILE               *sc_root_stderr = NULL;

static int          malloc_count = 0;
static int          free_count = 0;

static int          sc_identifier = -1;

static int          sc_log_priority = SC_LP_NONE;
static FILE        *sc_log_stream = NULL;
static bool         sc_log_stream_set = false;

static bool         signals_caught = false;
static sc_sig_t     system_int_handler = NULL;
static sc_sig_t     system_segv_handler = NULL;
static sc_sig_t     system_usr2_handler = NULL;

static sc_handler_t sc_abort_handler = NULL;
static void        *sc_abort_data = NULL;

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

void               *
sc_malloc (size_t size)
{
  void               *ret;

  ret = malloc (size);

  if (size > 0) {
    SC_CHECK_ABORT (ret != NULL, "Allocation");
    ++malloc_count;
  }
  else {
    malloc_count += ((ret == NULL) ? 0 : 1);
  }

  return ret;
}

void               *
sc_calloc (size_t nmemb, size_t size)
{
  void               *ret;

  ret = calloc (nmemb, size);

  if (nmemb * size > 0) {
    SC_CHECK_ABORT (ret != NULL, "Allocation");
    ++malloc_count;
  }
  else {
    malloc_count += ((ret == NULL) ? 0 : 1);
  }

  return ret;
}

void               *
sc_realloc (void *ptr, size_t size)
{
  void               *ret;

  ret = realloc (ptr, size);

  if (ptr == NULL) {
    if (size > 0) {
      SC_CHECK_ABORT (ret != NULL, "Reallocation");
      ++malloc_count;
    }
    else {
      malloc_count += ((ret == NULL) ? 0 : 1);
    }
  }
  else {
    if (size > 0) {
      SC_CHECK_ABORT (ret != NULL, "Reallocation");
    }
    else {
      free_count += ((ret == NULL) ? 1 : 0);
    }
  }

  return ret;
}

char               *
sc_strdup (const char *s)
{
  size_t              len;
  char               *d;

  if (s == NULL) {
    return NULL;
  }

  len = strlen (s) + 1;
  d = sc_malloc (len);
  memcpy (d, s, len);

  return d;
}

void
sc_free (void *ptr)
{
  if (ptr != NULL) {
    ++free_count;
    free (ptr);
  }
}

void
sc_memory_check (void)
{
  SC_CHECK_ABORT (malloc_count == free_count, "Memory balance");
}

void
sc_log_init (FILE * log_stream, int identifier)
{
  sc_identifier = identifier;
  sc_log_stream = log_stream;
  sc_log_stream_set = true;

  sc_root_stdout = identifier > 0 ? NULL : stdout;
  sc_root_stderr = identifier > 0 ? NULL : stderr;
}

void
sc_log_threshold (int log_priority)
{
  SC_ASSERT (log_priority >= SC_LP_NONE && log_priority <= SC_LP_SILENT);

  sc_log_priority = log_priority;
}

void
sc_logf (const char *filename, int lineno,
         int priority, int category, const char *fmt, ...)
{
  va_list             ap;

  SC_ASSERT (priority >= SC_LP_NONE && priority < SC_LP_SILENT);

  if (sc_log_stream == NULL && !sc_log_stream_set) {
    sc_log_stream = stdout;
    sc_log_stream_set = true;
  }

  if (sc_log_stream == NULL || priority < sc_log_priority)
    return;

  if (category == SC_LC_GLOBAL && sc_identifier > 0)
    return;

  if (category == SC_LC_NORMAL && sc_identifier >= 0)
    fprintf (sc_log_stream, "[%d] ", sc_identifier);

  if (priority == SC_LP_TRACE) {
    char                bn[BUFSIZ], *bp;

    snprintf (bn, BUFSIZ, "%s", filename);
    bp = basename (bn);
    fprintf (sc_log_stream, "%s:%d ", bp, lineno);
  }

  va_start (ap, fmt);
  vfprintf (sc_log_stream, fmt, ap);
  va_end (ap);

  fflush (sc_log_stream);
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

/* EOF sc.c */
