/*
  This file is part of the SC Library, version 3.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include <sc3_array.h>
#include <sc3_log.h>
#include <sc3_omp.h>
#include <sc3_refcount.h>

struct sc3_log
{
  sc3_refcount_t      rc;
  sc3_allocator_t    *lator;
  int                 setup;

  int                 alloced;
  int                 rank;
  int                 indent;
  sc3_log_level_t     level;

  int                 call_fclose;
  FILE               *file;
  sc3_log_function_t  func;
  void               *user;
};

static sc3_log_t    statlog = {
  {SC3_REFCOUNT_MAGIC, 1}, NULL, 1, 0, 0, 1, SC3_LOG_TOP, 0, NULL,
  sc3_log_function_default, NULL
};

sc3_log_t          *
sc3_log_predef (void)
{
  return &statlog;
}

int
sc3_log_is_valid (const sc3_log_t * log, char *reason)
{
  SC3E_TEST (log != NULL, reason);
  SC3E_IS (sc3_refcount_is_valid, &log->rc, reason);
  SC3E_TEST (!log->alloced == (log->lator == NULL), reason);
  if (log->lator != NULL) {
    SC3E_IS (sc3_allocator_is_setup, log->lator, reason);
  }

  SC3E_TEST (0 <= log->level && log->level < SC3_LOG_LEVEL_LAST, reason);
  SC3E_TEST (0 <= log->rank, reason);
  SC3E_TEST (0 <= log->indent, reason);

  SC3E_TEST (log->file != NULL || !log->call_fclose, reason);
  SC3E_TEST (log->func != NULL, reason);

  SC3E_YES (reason);
}

int
sc3_log_is_new (const sc3_log_t * log, char *reason)
{
  SC3E_IS (sc3_log_is_valid, log, reason);
  SC3E_TEST (!log->setup, reason);
  SC3E_YES (reason);
}

int
sc3_log_is_setup (const sc3_log_t * log, char *reason)
{
  SC3E_IS (sc3_log_is_valid, log, reason);
  SC3E_TEST (log->setup, reason);
  SC3E_YES (reason);
}

sc3_error_t        *
sc3_log_new (sc3_allocator_t * lator, sc3_log_t ** logp)
{
  sc3_log_t          *log;

  SC3E_RETVAL (logp, NULL);
  SC3A_IS (sc3_allocator_is_setup, lator);

  SC3E (sc3_allocator_ref (lator));
  SC3E (sc3_allocator_calloc_one (lator, sizeof (sc3_log_t), &log));
  SC3E (sc3_refcount_init (&log->rc));
  log->alloced = 1;
#ifdef SC_ENABLE_DEBUG
  log->level = SC3_LOG_DEBUG;
#else
  log->level = SC3_LOG_TOP;
#endif
  log->lator = lator;
  log->rank = 0;
  log->file = stderr;
  log->func = sc3_log_function_default;
  SC3A_IS (sc3_log_is_new, log);

  *logp = log;
  return NULL;
}

sc3_error_t        *
sc3_log_set_level (sc3_log_t * log, sc3_log_level_t level)
{
  SC3A_IS (sc3_log_is_new, log);
  SC3A_CHECK (0 <= log->level && log->level < SC3_LOG_LEVEL_LAST);
  log->level = level;
  return NULL;
}

sc3_error_t        *
sc3_log_set_comm (sc3_log_t * log, sc3_MPI_Comm_t mpicomm)
{
  SC3A_IS (sc3_log_is_new, log);
  if (mpicomm == SC3_MPI_COMM_NULL) {
    log->rank = 0;
  }
  else {
    SC3E (sc3_MPI_Comm_rank (mpicomm, &log->rank));
  }
  return NULL;
}

sc3_error_t        *
sc3_log_set_file (sc3_log_t * log, FILE * file, int call_fclose)
{
  SC3A_IS (sc3_log_is_new, log);
  SC3A_CHECK (file != NULL);

  log->call_fclose = call_fclose;
  log->file = file;
  return NULL;
}

void
sc3_log_function_bare (void *user, const char *msg,
                       sc3_log_role_t role, int rank, int tid,
                       sc3_log_level_t level, int spaces, FILE * outfile)
{
  /* output message as is */
  fprintf (outfile != NULL ? outfile : stderr, "%s\n", msg);
}

void
sc3_log_function_default (void *user, const char *msg,
                          sc3_log_role_t role, int rank, int tid,
                          sc3_log_level_t level, int spaces, FILE * outfile)
{
  char                header[SC3_BUFSIZE];

  /* construct elaborate message and write it */
  if (role == SC3_LOG_PROCESS0) {
    snprintf (header, SC3_BUFSIZE, "%s", "sc3");
  }
  else if (role == SC3_LOG_THREAD0) {
    snprintf (header, SC3_BUFSIZE, "%s %d", "sc3", rank);
  }
  else {
    snprintf (header, SC3_BUFSIZE, "%s %d:%d", "sc3", rank, tid);
  }
  fprintf (outfile != NULL ? outfile : stderr,
           "[%s] %*s%s\n", header, spaces, "", msg);
}

sc3_error_t        *
sc3_log_set_function (sc3_log_t * log, sc3_log_function_t func, void *user)
{
  SC3A_IS (sc3_log_is_new, log);
  SC3A_CHECK (func != NULL);

  log->func = func;
  log->user = user;
  return NULL;
}

sc3_error_t        *
sc3_log_set_indent (sc3_log_t * log, int indent)
{
  SC3A_IS (sc3_log_is_new, log);
  SC3A_CHECK (0 <= indent);
  log->indent = indent;
  return NULL;
}

sc3_error_t        *
sc3_log_setup (sc3_log_t * log)
{
  SC3A_IS (sc3_log_is_new, log);

  log->setup = 1;
  SC3A_IS (sc3_log_is_setup, log);
  return NULL;
}

sc3_error_t        *
sc3_log_ref (sc3_log_t * log)
{
  SC3A_IS (sc3_log_is_setup, log);
  if (log->alloced) {
    SC3E (sc3_refcount_ref (&log->rc));
  }
  return NULL;
}

sc3_error_t        *
sc3_log_unref (sc3_log_t ** logp)
{
  int                 waslast;
  sc3_log_t          *log;
  sc3_error_t        *leak = NULL;

  SC3E_INOUTP (logp, log);
  SC3A_IS (sc3_log_is_valid, log);

  if (!log->alloced) {
    return NULL;
  }

  SC3E (sc3_refcount_unref (&log->rc, &waslast));
  if (waslast) {
    sc3_allocator_t    *lator;

    *logp = NULL;

    lator = log->lator;
    if (!log->call_fclose && fflush (log->file)) {
      /* TODO create runtime error when flush fails */
    }
    if (log->call_fclose && fclose (log->file)) {
      /* TODO create runtime error when close fails */
    }
    SC3E (sc3_allocator_free (lator, log));
    SC3L (&leak, sc3_allocator_unref (&lator));
  }
  return leak;
}

sc3_error_t        *
sc3_log_destroy (sc3_log_t ** logp)
{
  sc3_log_t          *log;
  sc3_error_t        *leak = NULL;

  SC3E_INULLP (logp, log);
  SC3L_DEMAND (&leak, sc3_refcount_is_last (&log->rc, NULL));
  SC3L (&leak, sc3_log_unref (&log));

  SC3A_CHECK (log == NULL || !log->alloced || leak != NULL);
  return leak;
}

void
sc3_log (sc3_log_t * log, int depth,
         sc3_log_role_t role, sc3_log_level_t level, const char *msg)
{
  int                 tid;

  /* survive NULL message */
  if (msg == NULL) {
    msg = "NULL message";
  }

  /* catch invalid usage */
  if (!sc3_log_is_setup (log, NULL) ||
      !(0 <= role && role < SC3_LOG_ROLE_LAST) ||
      !(0 <= level && level < SC3_LOG_LEVEL_LAST)) {
    fprintf (stderr, "[sc3] BAD sc3_log: %s\n", msg);
    return;
  }

  if (level < log->level || level == SC3_LOG_SILENT) {
    /* the log level is not sufficiently large */
    return;
  }

  tid = sc3_omp_thread_num ();
  if (role == SC3_LOG_PROCESS0 && (log->rank != 0 || tid != 0)) {
    /* only log for the master thread in master process */
    return;
  }
  if (role == SC3_LOG_THREAD0 && tid != 0) {
    /* only log for the master thread */
    return;
  }

  /* output message */
  if (log->func != NULL) {
    log->func (log->user, msg, role, log->rank, tid, level,
               depth >= 0 ? depth * log->indent : 0,
               log->file != NULL ? log->file : stderr);
  }
  else {
    fputs (msg, log->file != NULL ? log->file : stderr);
  }
}

void
sc3_logf (sc3_log_t * log, int depth,
          sc3_log_role_t role, sc3_log_level_t level, const char *fmt, ...)
{
  if (fmt != NULL) {
    va_list             ap;

    va_start (ap, fmt);
    sc3_logv (log, depth, role, level, fmt, ap);
    va_end (ap);
  }
  else {
    fprintf (stderr, "[sc3] BAD fmt in sc3_logf\n");
  }
}

void
sc3_logv (sc3_log_t * log, int depth,
          sc3_log_role_t role, sc3_log_level_t level,
          const char *fmt, va_list ap)
{
  if (fmt != NULL) {
    char                msg[SC3_BUFSIZE];

    if (0 <= vsnprintf (msg, SC3_BUFSIZE, fmt, ap)) {
      sc3_log (log, depth, role, level, msg);
    }
    else {
      fprintf (stderr, "[sc3] BAD vsnprintf in sc3_logv\n");
    }
  }
  else {
    fprintf (stderr, "[sc3] BAD fmt in sc3_logv\n");
  }
}

static sc3_error_t *
sc3_log_error_recursion (sc3_log_t * log, int depth,
                         sc3_log_role_t role, sc3_log_level_t level,
                         sc3_error_t * e, int stackdepth)
{
  int                 line;
  const char         *errmsg;
  const char         *filename;
  sc3_error_kind_t    kind;
  sc3_error_t        *s;

  /* go down the stack recursively first */
  SC3E (sc3_error_get_stack (e, &s));
  if (s != NULL) {
    SC3E (sc3_log_error_recursion (log, depth, role, level,
                                   s, stackdepth + 1));
    SC3E (sc3_error_unref (&s));
  }

  /* log this level of the error stack */
  SC3E (sc3_error_get_kind (e, &kind));
  SC3E (sc3_error_access_message (e, &errmsg));
  SC3E (sc3_error_access_location (e, &filename, &line));

  sc3_logf (log, depth, role, level, "%d %s:%d:%c %s", stackdepth,
            filename, line, sc3_error_kind_char[kind], errmsg);

  SC3E (sc3_error_restore_message (e, errmsg));
  SC3E (sc3_error_restore_location (e, filename, line));
  return NULL;
}

sc3_error_t        *
sc3_log_error (sc3_log_t * log, int depth,
               sc3_log_role_t role, sc3_log_level_t level, sc3_error_t * e)
{
  SC3E (sc3_log_error_recursion (log, depth, role, level, e, 0));
  return NULL;
}
