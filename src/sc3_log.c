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
#include <sc3_openmp.h>
#include <sc3_refcount_internal.h>
#include <stdarg.h>

struct sc3_log
{
  sc3_refcount_t      rc;
  sc3_allocator_t    *lator;
  int                 setup;

  sc3_log_role_t      role;
  sc3_log_level_t     level;
  int                 rank;
  int                 indent;

  FILE               *file;
  int                 call_fclose;
};

int
sc3_log_is_valid (sc3_log_t * log, char *reason)
{
  SC3E_TEST (log != NULL, reason);
  SC3E_IS (sc3_refcount_is_valid, &log->rc, reason);
  SC3E_IS (sc3_allocator_is_setup, log->lator, reason);

  SC3E_TEST (0 <= log->role && log->role < SC3_LOG_ROLE_LAST, reason);
  SC3E_TEST (0 <= log->level && log->level < SC3_LOG_LEVEL_LAST, reason);
  SC3E_TEST (0 <= log->rank, reason);
  SC3E_TEST (0 <= log->indent, reason);

  SC3E_TEST (log->file != NULL || !log->call_fclose, reason);

  SC3E_YES (reason);
}

int
sc3_log_is_new (sc3_log_t * log, char *reason)
{
  SC3E_IS (sc3_log_is_valid, log, reason);
  SC3E_TEST (!log->setup, reason);
  SC3E_YES (reason);
}

int
sc3_log_is_setup (sc3_log_t * log, char *reason)
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
  SC3E_ALLOCATOR_CALLOC (lator, sc3_log_t, 1, log);
  SC3E (sc3_refcount_init (&log->rc));
  log->role = SC3_LOG_ANY;
#ifdef SC_ENABLE_DEBUG
  log->level = SC3_LOG_DEBUG;
#else
  log->level = SC3_LOG_INFO;
#endif
  log->lator = lator;
  log->rank = 0;
  log->file = stderr;
  SC3A_IS (sc3_log_is_new, log);

  *logp = log;
  return NULL;
}

sc3_error_t        *
sc3_log_set_role (sc3_log_t * log, sc3_log_role_t role)
{
  SC3A_IS (sc3_log_is_new, log);
  SC3A_CHECK (0 <= role && role < SC3_LOG_ROLE_LAST);
  log->role = role;
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
  if (file == NULL) {
    log->file = stderr;
    log->call_fclose = 0;
  }
  else {
    log->file = file;
    log->call_fclose = call_fclose;
  }
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
  SC3E (sc3_refcount_ref (&log->rc));
  return NULL;
}

sc3_error_t        *
sc3_log_unref (sc3_log_t ** logp)
{
  int                 waslast;
  sc3_log_t          *log;

  SC3E_INOUTP (logp, log);
  SC3A_IS (sc3_log_is_valid, log);
  SC3E (sc3_refcount_unref (&log->rc, &waslast));
  if (waslast) {
    sc3_allocator_t    *lator;

    *logp = NULL;

    lator = log->lator;
    if (log->call_fclose && fclose (log->file)) {

      /* TODO create runtime error when close fails */

    }
    SC3E_ALLOCATOR_FREE (lator, sc3_log_t, log);
    SC3E (sc3_allocator_unref (&lator));
  }
  return NULL;
}

sc3_error_t        *
sc3_log_destroy (sc3_log_t ** logp)
{
  sc3_log_t          *log;

  SC3E_INULLP (logp, log);
  SC3E_DEMIS (sc3_refcount_is_last, &log->rc);
  SC3E (sc3_log_unref (&log));

  SC3A_CHECK (log == NULL);
  return NULL;
}

void
sc3_log (sc3_log_t * log, sc3_trace_t * t,
         sc3_log_role_t role, sc3_log_level_t level, const char *msg)
{
  int                 tid;
  int                 spaces;
  char                header[SC3_BUFSIZE];

  /* catch invalid usage */
  if (!sc3_log_is_setup (log, NULL) || log->file == NULL) {
    /* currently the file can never be NULL, but we check anyway */
    return;
  }
  if (!(0 <= level && level < SC3_LOG_LEVEL_LAST) || msg == NULL) {
    return;
  }

  /* take depth of indentation from call stack */
  if (t != NULL && t->depth >= 0) {
    spaces = log->indent * t->depth;
  }
  else {
    spaces = 0;
  }

  if (level < log->level) {
    /* the log level is not sufficiently large */
    return;
  }

  tid = sc3_openmp_thread_num ();
  if (role == SC3_LOG_PROCESS0 && (log->rank != 0 || tid != 0)) {
    /* only log for the master thread in master process */
    return;
  }
  if (role == SC3_LOG_THREAD0 && tid != 0) {
    /* only log for the master thread */
    return;
  }

  /* construct message and write it */
  if (role == SC3_LOG_PROCESS0) {
    snprintf (header, SC3_BUFSIZE, "%s", "SC3");
  }
  else if (role == SC3_LOG_THREAD0) {
    snprintf (header, SC3_BUFSIZE, "%s %d", "SC3", log->rank);
  }
  else {
    snprintf (header, SC3_BUFSIZE, "%s %d:%d", "SC3", log->rank, tid);
  }
  fprintf (log->file, "[%s] %*s%s\n", header, spaces, "", msg);
}

void
sc3_logf (sc3_log_t * log, sc3_trace_t * t,
          sc3_log_role_t role, sc3_log_level_t level, const char *fmt, ...)
{
  if (sc3_log_is_setup (log, NULL) && fmt != NULL) {
    va_list             ap;

    va_start (ap, fmt);
    sc3_logv (log, t, role, level, fmt, ap);
    va_end (ap);
  }
}

void
sc3_logv (sc3_log_t * log, sc3_trace_t * t,
          sc3_log_role_t role, sc3_log_level_t level,
          const char *fmt, va_list ap)
{
  if (sc3_log_is_setup (log, NULL) && fmt != NULL) {
    char                msg[SC3_BUFSIZE];

    if (0 <= vsnprintf (msg, SC3_BUFSIZE, fmt, ap)) {
      sc3_log (log, t, role, level, msg);
    }
  }
}
