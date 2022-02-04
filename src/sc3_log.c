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

#include <sc3_log.h>
#include <sc3_refcount.h>

struct sc3_log
{
  sc3_refcount_t      rc;
  sc3_allocator_t    *lator;
  int                 setup;

  int                 alloced;
  sc3_log_level_t     level;
  sc3_MPI_Comm_t      mpicomm;

  int                 call_fclose;
  FILE               *file;
  sc3_log_function_t  func;
  void               *user;
};

static sc3_log_t    statlog = {
  {SC3_REFCOUNT_MAGIC, 1}, NULL, 1, 0, SC3_LOG_LEVEL,
  SC3_MPI_COMM_WORLD, 0, NULL, sc3_log_function_default, NULL
};

sc3_log_t          *
sc3_log_new_static (void)
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
  SC3E_TEST (SC3_MPI_COMM_NULL != log->mpicomm, reason);

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

  if (lator == NULL) {
    lator = sc3_allocator_new_static ();
  }
  SC3A_IS (sc3_allocator_is_setup, lator);

  SC3E (sc3_allocator_ref (lator));
  SC3E (sc3_allocator_calloc (lator, 1, sizeof (sc3_log_t), &log));
  SC3E (sc3_refcount_init (&log->rc));
  log->alloced = 1;
#ifdef SC_ENABLE_DEBUG
  log->level = SC3_LOG_DEBUG;
#else
  log->level = SC3_LOG_INFO;
#endif
  log->mpicomm = SC3_MPI_COMM_WORLD;
  log->lator = lator;
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
  int                 rank;

  SC3A_IS (sc3_log_is_new, log);
  SC3A_CHECK (mpicomm != SC3_MPI_COMM_NULL);

  /* just checking that this process is in the communicator */
  SC3E (sc3_MPI_Comm_rank (mpicomm, &rank));

  log->mpicomm = mpicomm;
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
                       sc3_log_role_t role, int rank, sc3_log_level_t level,
                       int indent, FILE * outfile)
{
  /* survive NULL message */
  if (msg == NULL) {
    msg = "NULL log message";
  }

  /* output log message as is */
  fprintf (outfile != NULL ? outfile : stderr, "%s\n", msg);
}

void
sc3_log_function_prefix (void *user, const char *msg,
                         sc3_log_role_t role, int rank, sc3_log_level_t level,
                         int indent, FILE * outfile)
{
  char                header[SC3_BUFSIZE];
  const char         *prefix;
  const char         *pnull = "Log prefix error";
  sc3_log_puser_t    *puser = (sc3_log_puser_t *) user;

  /* survive NULL prefix */
  prefix = puser == NULL ? pnull : puser->prefix;

  /* survive NULL message */
  if (msg == NULL) {
    msg = "NULL log message";
  }

  /* survive invalid role */
  if (!(0 <= role && role < SC3_LOG_ROLE_LAST)) {
    role = SC3_LOG_LOCAL;
    msg = "Invalid log role";
  }

  /* survive invalid rank */
  if (rank < 0) {
    rank = 0;
    msg = "Invalid log rank";
  }

  /* silently fix indentation */
  if (indent < 0) {
    indent = 0;
  }
  else if (indent > 32) {
    indent = 32;
  }

  /* construct elaborate message and write it */
  if (role == SC3_LOG_LOCAL) {
    if (snprintf (header, SC3_BUFSIZE, "%s %d", prefix, rank) < 0) {
      snprintf (header, SC3_BUFSIZE, pnull);
    }
  }
  else {
    if (snprintf (header, SC3_BUFSIZE, "%s", prefix) < 0) {
      snprintf (header, SC3_BUFSIZE, pnull);
    }
  }
  fprintf (outfile != NULL ? outfile : stderr,
           "[%s] %*s%s\n", header, indent, "", msg);
}

static sc3_log_puser_t sc3_log_puser = { "sc3" };

void
sc3_log_function_default (void *user, const char *msg,
                          sc3_log_role_t role, int rank,
                          sc3_log_level_t level, int indent, FILE * outfile)
{
  sc3_log_function_prefix (&sc3_log_puser, msg, role, rank, level, indent,
                           outfile);
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
    SC3E (sc3_allocator_free (lator, &log));
    SC3E (sc3_allocator_unref (&lator));
  }
  return NULL;
}

sc3_error_t        *
sc3_log_destroy (sc3_log_t ** logp)
{
  sc3_log_t          *log;

  SC3E_INULLP (logp, log);
  SC3E_DEMIS (sc3_refcount_is_last, &log->rc, SC3_ERROR_REF);
  SC3E (sc3_log_unref (&log));

  SC3A_CHECK (log == NULL || !log->alloced);
  return NULL;
}

void
sc3_log (sc3_log_t * log,
         sc3_log_role_t role, sc3_log_level_t level,
         int indent, const char *msg)
{
  int                 rank;
#ifdef SC_ENABLE_MPI
  int                 mpiret;
#endif

  /* catch invalid arguments */
  if (!sc3_log_is_setup (log, NULL)) {
    log = sc3_log_new_static ();
    level = SC3_LOG_ERROR;
    msg = "Invalid log object";
  }
  else if (!(0 <= role && role < SC3_LOG_ROLE_LAST)) {
    role = SC3_LOG_LOCAL;
    level = SC3_LOG_ERROR;
    msg = "Invalid log role";
  }
  else if (!(0 <= level && level < SC3_LOG_LEVEL_LAST)) {
    level = SC3_LOG_ERROR;
    msg = "Invalid log level";
  }

  /* do not print when the level is too low */
  if (level < log->level || level == SC3_LOG_SILENT) {
    /* the log level is not sufficiently large */
    return;
  }

  rank = 0;
#ifdef SC_ENABLE_MPI
  mpiret = MPI_Comm_rank (log->mpicomm, &rank);
  if (mpiret != SC3_MPI_SUCCESS) {
    rank = 0;
    level = SC3_LOG_ERROR;
    msg = "Invalid MPI communicator";
  }

  /* only print on root process when specified */
  if (role == SC3_LOG_GLOBAL && rank != 0) {
    /* only log for the master thread in master process */
    return;
  }
#endif /* SC_ENABLE_MPI */

  /* finally print message */
  log->func (log->user, msg, role, rank, level, indent, log->file);
}

void
sc3_logf (sc3_log_t * log,
          sc3_log_role_t role, sc3_log_level_t level,
          int indent, const char *fmt, ...)
{
  va_list             ap;

  va_start (ap, fmt);
  sc3_logv (log, role, level, indent, fmt, ap);
  va_end (ap);
}

void
sc3_logv (sc3_log_t * log,
          sc3_log_role_t role, sc3_log_level_t level,
          int indent, const char *fmt, va_list ap)
{
  char                buf[SC3_BUFSIZE];
  const char         *msg = NULL;

  if (fmt != NULL) {
    if (0 <= vsnprintf (buf, SC3_BUFSIZE, fmt, ap)) {
      msg = buf;
    }
    else {
      msg = "Log format error";
    }
  }
  sc3_log (log, role, level, indent, msg);
}

#if 0

static sc3_error_t *
sc3_log_error_recursion (sc3_log_t * log, int depth,
                         sc3_log_role_t role, sc3_log_level_t level,
                         sc3_error_t * e, int stackdepth, char *bwork)
{
  int                 line;
  const char         *errmsg;
  const char         *filename, *bname;
  sc3_error_kind_t    kind;
  sc3_error_t        *s;

  /* TODO what to do with depth */

  /* couple debug checks */
  SC3A_CHECK (depth >= 0);
  SC3A_CHECK (stackdepth >= 0);
  SC3A_CHECK (bwork != NULL);

  /* go down the stack recursively first */
  SC3E (sc3_error_ref_stack (e, &s));
  if (s != NULL) {
    SC3E (sc3_log_error_recursion (log, depth, role, level,
                                   s, stackdepth + 1, bwork));
    SC3E (sc3_error_unref (&s));
  }

  /* log this level of the error stack */
  SC3E (sc3_error_get_kind (e, &kind));
  SC3E (sc3_error_access_message (e, &errmsg));
  SC3E (sc3_error_access_location (e, &filename, &line));
  SC3_BUFCOPY (bwork, filename);
  bname = sc3_basename (bwork);

  sc3_logf (log, role, level, "%d %s:%d:%c %s", stackdepth,
            bname, line, sc3_error_kind_char[kind], errmsg);

  SC3E (sc3_error_restore_message (e, errmsg));
  SC3E (sc3_error_restore_location (e, filename, line));
  return NULL;
}

sc3_error_t        *
sc3_log_error (sc3_log_t * log,
               sc3_log_role_t role, sc3_log_level_t level, sc3_error_t * e)
{
  char                bwork[SC3_BUFSIZE];
  SC3E (sc3_log_error_recursion (log, 0, role, level, e, 0, bwork));
  return NULL;
}

#endif /* 0 */

#define _SC3_LOG_BODY(localglobal,loglevel)                             \
  if (SC3_LOG_LEVEL <= loglevel) {                                      \
    va_list            ap;                                              \
    va_start (ap, fmt);                                                 \
    sc3_logv (sc3_log_new_static (), localglobal, loglevel,             \
              0, fmt, ap);                                              \
    va_end (ap);                                                        \
  }

#define _SC3_LOG_FUNCTIONS(barelevel)                                   \
void SC3_ ## barelevel ## F (const char *fmt, ...)                      \
{ _SC3_LOG_BODY (SC3_LOG_LOCAL, SC3_LOG_ ## barelevel) }                \
void SC3_GLOBAL_ ## barelevel ## F (const char *fmt, ...)               \
{ _SC3_LOG_BODY (SC3_LOG_GLOBAL, SC3_LOG_ ## barelevel) }

/* *INDENT-OFF* */
_SC3_LOG_FUNCTIONS (NOISE)
_SC3_LOG_FUNCTIONS (DEBUG)
_SC3_LOG_FUNCTIONS (INFO)
_SC3_LOG_FUNCTIONS (STATISTICS)
_SC3_LOG_FUNCTIONS (PRODUCTION)
_SC3_LOG_FUNCTIONS (ESSENTIAL)
_SC3_LOG_FUNCTIONS (ERROR)
/* *INDENT-ON* */
