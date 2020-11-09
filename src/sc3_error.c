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

#include <sc3_alloc_internal.h>
#include <sc3_error.h>
#include <sc3_refcount.h>

struct sc3_error
{
  sc3_refcount_t      rc;
  sc3_allocator_t    *eator;
  int                 setup;

  sc3_error_kind_t    kind;
#if 0
  sc3_error_severity_t sev;
  sc3_error_sync_t    syn;
#endif
  char                errmsg[SC3_BUFSIZE];
  char                filename[SC3_BUFSIZE];
  int                 line;
  int                 alloced;
  sc3_error_t        *stack;

  int                 accessed_locations;
  int                 accessed_messages;
};

const char          sc3_error_kind_char[SC3_ERROR_KIND_LAST] =
  { 'F', 'W', 'R', 'B', 'M', 'N', 'L', 'I', 'U' };

#if 0
static sc3_error_t  ebug = { {SC3_REFCOUNT_MAGIC, 1}, NULL, 1, SC3_ERROR_BUG,
"Inconsistency or bug", __FILE__, __LINE__, 0, NULL
};
#endif

static sc3_error_t  enom =
  { {SC3_REFCOUNT_MAGIC, 1}, NULL, 1, SC3_ERROR_MEMORY,
"Out of memory", __FILE__, __LINE__, 0, NULL, 0, 0
};

static sc3_error_t  enull = { {SC3_REFCOUNT_MAGIC, 1}, NULL, 1, SC3_ERROR_BUG,
"Argument must not be NULL", __FILE__, __LINE__, 0, NULL, 0, 0
};

static sc3_error_t  esetup =
  { {SC3_REFCOUNT_MAGIC, 1}, NULL, 1, SC3_ERROR_BUG,
"Error argument must be setup", __FILE__, __LINE__, 0, NULL, 0, 0
};

static int
sc3_error_kind_is_fatal (sc3_error_kind_t kind)
{
  /* We do not classify the kind SC3_ERROR_LEAK as fatal. */
  return kind == SC3_ERROR_FATAL || kind == SC3_ERROR_BUG ||
    kind == SC3_ERROR_MEMORY || kind == SC3_ERROR_NETWORK;
}

int
sc3_error_is_valid (const sc3_error_t * e, char *reason)
{
  SC3E_TEST (e != NULL, reason);
  SC3E_IS (sc3_refcount_is_valid, &e->rc, reason);
  SC3E_TEST (!e->alloced == (e->eator == NULL), reason);
  if (e->eator != NULL) {
    SC3E_IS (sc3_allocator_is_setup, e->eator, reason);
  }
  if (e->stack != NULL) {
    SC3E_IS (sc3_error_is_setup, e->stack, reason);
    if (e->setup) {
      SC3E_TEST (!(sc3_error_kind_is_fatal (e->stack->kind) &&
                   !sc3_error_kind_is_fatal (e->kind)), reason);
    }
  }
  SC3E_TEST (0 <= e->kind && e->kind < SC3_ERROR_KIND_LAST, reason);
#if 0
  SC3E_TEST (0 <= e->sev && e->sev < SC3_ERROR_SEVERITY_LAST, reason);
  SC3E_TEST (0 <= e->syn && e->syn < SC3_ERROR_SYNC_LAST, reason);
#endif
  SC3E_TEST (0 <= e->accessed_locations, reason);
  SC3E_TEST (0 <= e->accessed_messages, reason);
  SC3E_YES (reason);
}

int
sc3_error_is_new (const sc3_error_t * e, char *reason)
{
  SC3E_IS (sc3_error_is_valid, e, reason);
  SC3E_TEST (!e->setup, reason);
  SC3E_YES (reason);
}

int
sc3_error_is_setup (const sc3_error_t * e, char *reason)
{
  SC3E_IS (sc3_error_is_valid, e, reason);
  SC3E_TEST (e->setup, reason);
  SC3E_YES (reason);
}

int
sc3_error_is_fatal (const sc3_error_t * e, char *reason)
{
  SC3E_IS (sc3_error_is_setup, e, reason);
  SC3E_TEST (sc3_error_kind_is_fatal (e->kind), reason);
  SC3E_YES (reason);
}

int
sc3_error_is_leak (const sc3_error_t * e, char *reason)
{
  SC3E_IS (sc3_error_is_setup, e, reason);
  SC3E_TEST (e->kind == SC3_ERROR_LEAK, reason);
  SC3E_YES (reason);
}

int
sc3_error_is2_kind (const sc3_error_t * e, sc3_error_kind_t kind,
                    char *reason)
{
  SC3E_IS (sc3_error_is_setup, e, reason);
  SC3E_TEST (e->kind == kind, reason);
  SC3E_YES (reason);
}

#ifdef SC_ENABLE_DEBUG

static int
sc3_error_is_null_or_leak (const sc3_error_t * e, char *reason)
{
  if (e == NULL) {
    SC3E_YES (reason);
  }
  return sc3_error_is_leak (e, reason);
}

#endif

static void
sc3_error_defaults (sc3_error_t * e, sc3_error_t * stack,
                    int setup, int inherit, sc3_allocator_t * eator)
{
  memset (e, 0, sizeof (sc3_error_t));
  sc3_refcount_init (&e->rc);
  e->eator = eator;
  e->setup = setup;

  e->kind = SC3_ERROR_FATAL;
#if 0
  e->sev = SC3_ERROR_SEVERITY_LAST;
  e->syn = SC3_ERROR_SYNC_LAST;
#endif
  e->alloced = 1;
  e->stack = stack;
  if (inherit && stack != NULL) {
    e->kind = stack->kind;
#if 0
    e->sev = stack->sev;
#endif
  }
}

sc3_error_t        *
sc3_error_new (sc3_allocator_t * eator, sc3_error_t ** ep)
{
  sc3_error_t        *e;

  SC3E_RETVAL (ep, NULL);
  SC3A_IS (sc3_allocator_is_setup, eator);

  SC3E (sc3_allocator_ref (eator));
  SC3E (sc3_allocator_malloc (eator, sizeof (sc3_error_t), &e));
  sc3_error_defaults (e, NULL, 0, 0, eator);
  SC3A_IS (sc3_error_is_new, e);

  *ep = e;
  return NULL;
}

sc3_error_t        *
sc3_error_set_stack (sc3_error_t * e, sc3_error_t ** pstack)
{
  sc3_error_t        *stack;

  SC3A_IS (sc3_error_is_new, e);
  SC3A_CHECK (pstack != NULL);
  if ((stack = *pstack) != NULL) {
    *pstack = NULL;
    SC3A_IS (sc3_error_is_setup, stack);
  }
  if (e->stack != NULL) {
    /* a leak error at this point is considered fatal */
    SC3E (sc3_error_unref (&e->stack));
  }
  e->stack = stack;
  return NULL;
}

sc3_error_t        *
sc3_error_set_location (sc3_error_t * e, const char *filename, int line)
{
  SC3A_IS (sc3_error_is_new, e);
  SC3A_CHECK (filename != NULL);

  SC3_BUFCOPY (e->filename, filename);
  e->line = line;
  return NULL;
}

sc3_error_t        *
sc3_error_set_message (sc3_error_t * e, const char *errmsg)
{
  SC3A_IS (sc3_error_is_new, e);
  SC3A_CHECK (errmsg != NULL);

  SC3_BUFCOPY (e->errmsg, errmsg);
  return NULL;
}

sc3_error_t        *
sc3_error_set_kind (sc3_error_t * e, sc3_error_kind_t kind)
{
  SC3A_IS (sc3_error_is_new, e);
  SC3A_CHECK (0 <= kind && kind < SC3_ERROR_KIND_LAST);

  e->kind = kind;
  return NULL;
}

#if 0
sc3_error_t        *
sc3_error_set_severity (sc3_error_t * e, sc3_error_severity_t sev)
{
  SC3A_IS (sc3_error_is_new, e);
  SC3A_CHECK (0 <= sev && sev < SC3_ERROR_SEVERITY_LAST);

  e->sev = sev;
  return NULL;
}

void                sc3_error_set_sync (sc3_error_t * ea,
                                        sc3_error_sync_t syn);
void                sc3_error_set_msgf (sc3_error_t * ea,
                                        const char *errfmt, ...)
  __attribute__((format (printf, 2, 3)));
#endif

sc3_error_t        *
sc3_error_setup (sc3_error_t * e)
{
  SC3A_IS (sc3_error_is_new, e);

  /* promote error to fatal if stack is fatal */
  if (e->stack != NULL && sc3_error_kind_is_fatal (e->stack->kind) &&
      !sc3_error_kind_is_fatal (e->kind)) {
    e->kind = SC3_ERROR_FATAL;
  }

  /* we are done with setup */
  e->setup = 1;
  SC3A_IS (sc3_error_is_setup, e);
  return NULL;
}

sc3_error_t        *
sc3_error_ref (sc3_error_t * e)
{
  SC3A_IS (sc3_error_is_setup, e);
  if (e->alloced) {
    SC3E (sc3_refcount_ref (&e->rc));
  }
  return NULL;
}

sc3_error_t        *
sc3_error_unref (sc3_error_t ** ep)
{
  int                 waslast;
  sc3_allocator_t    *eator;
  sc3_error_t        *e;
  sc3_error_t        *leak = NULL;

  SC3E_INOUTP (ep, e);
  SC3A_IS (sc3_error_is_valid, e);

  if (!e->alloced) {
    /* It is our convention that non-alloced errors must not have a stack. */
    SC3A_CHECK (e->stack == NULL);
    return NULL;
  }

  SC3E (sc3_refcount_unref (&e->rc, &waslast));
  if (waslast) {
    *ep = NULL;

    SC3E_DEMAND (e->accessed_locations == 0, "Pending location accesses");
    SC3E_DEMAND (e->accessed_messages == 0, "Pending message accesses");
    if (e->stack != NULL) {
      SC3L (&leak, sc3_error_unref (&e->stack));
    }

    eator = e->eator;
    SC3E (sc3_allocator_free (eator, e));
    SC3L (&leak, sc3_allocator_unref (&eator));
  }
  return leak;
}

sc3_error_t        *
sc3_error_destroy (sc3_error_t ** ep)
{
  sc3_error_t        *e, *leak = NULL;

  SC3E_INULLP (ep, e);
  SC3L_DEMAND (&leak, sc3_refcount_is_last (&e->rc, NULL));
  SC3L (&leak, sc3_error_unref (&e));

  SC3A_CHECK (e == NULL || !e->alloced || leak != NULL);
  return leak;
}

void
sc3_error_destroy_noerr (sc3_error_t ** pe, char *flatmsg)
{
#if 0
  int                 remain, result;
  int                 line;
  const char         *filename, *msg;
  char               *pos, *bname;
  sc3_error_t        *e, *inte, *stack;
  sc3_error_kind_t    kind;

  /* catch invalid calls */
  if (flatmsg == NULL) {
    return;
  }
  if (pe == NULL || *pe == NULL) {
    SC3_BUFCOPY (flatmsg, "No error supplied");
    return;
  }

  /* now the incoming error object must be analyzed and cleaned up */
  e = *pe;
  *pe = NULL;

  /* go through error stack's messages */
  remain = SC3_BUFSIZE;
  *(pos = flatmsg) = '\0';
  do {
    /* append error location and message to output string */
    bname = NULL;
    SC3E_SET (inte, sc3_error_access_location (e, &filename, &line));
    if (inte == NULL && (bname = strdup (filename)) != NULL) {
      filename = sc3_basename (bname);
    }
    SC3E_NULL_SET (inte, sc3_error_get_message (e, &msg));
    SC3E_NULL_SET (inte, sc3_error_get_kind (e, &kind));
    if (inte == NULL && remain > 0) {
      result = snprintf (pos, remain, "%s%s:%d:%c %s",
                         pos == flatmsg ? "" : ": ", filename, line,
                         sc3_error_kind_char[kind], msg);
      if (result < 0 || result >= remain) {
        pos = NULL;
        remain = 0;
      }
      else {
        pos += result;
        remain -= result;
      }
    }
    free (bname);

    /* do down the error stack */
    stack = NULL;
    SC3E_NULL_SET (inte, sc3_error_get_stack (e, &stack));
    SC3E_NULL_SET (inte, sc3_error_destroy (&e));
    if (inte != NULL) {
      sc3_error_destroy (&inte);
    }

    /* continue if there is a stack left */
    e = stack;
  }
  while (e != NULL);
#endif
}

sc3_error_t        *
sc3_error_new_kind (sc3_error_kind_t kind,
                    const char *filename, int line, const char *errmsg)
{
  sc3_error_t        *e;
  sc3_allocator_t    *ea;

  /* Avoid infinite loop when out of memory by returning static errors. */
  if (filename == NULL || errmsg == NULL) {
    return &enull;
  }

  /* Call system malloc without additional internal tracking of memory. */
  ea = sc3_allocator_nocount ();
  e = (sc3_error_t *) sc3_allocator_malloc_noerr (ea, sizeof (sc3_error_t));
  if (e == NULL) {
    return &enom;
  }
  sc3_error_defaults (e, NULL, 1, 0, ea);
  if (0 <= kind && kind < SC3_ERROR_KIND_LAST) {
    e->kind = kind;
  }

  SC3_BUFCOPY (e->errmsg, errmsg);
  SC3_BUFCOPY (e->filename, filename);
  e->line = line;

  /* This function returns the new error object and has no error code. */
  return e;
}

sc3_error_t        *
sc3_error_new_bug (const char *filename, int line, const char *errmsg)
{
  return sc3_error_new_kind (SC3_ERROR_BUG, filename, line, errmsg);
}

/* This function takes over one reference to stack. */
static sc3_error_t *
sc3_error_new_stack_inherit (sc3_error_t ** pstack, int inherit,
                             const char *filename, int line,
                             const char *errmsg)
{
  sc3_error_t        *stack, *e;
  sc3_allocator_t    *ea;

  /* Avoid infinite loop when out of memory by returning static errors. */
  if (pstack == NULL) {
    return &enull;
  }
  stack = *pstack;
  *pstack = NULL;
  if (!sc3_error_is_setup (stack, NULL)) {
    return &esetup;
  }
  if (filename == NULL || errmsg == NULL) {
    return stack;
  }

  /* Call system malloc without additional internal tracking of memory. */
  ea = sc3_allocator_nocount ();
  e = (sc3_error_t *) sc3_allocator_malloc_noerr (ea, sizeof (sc3_error_t));
  if (e == NULL) {
    return stack;
  }
  sc3_error_defaults (e, stack, 1, inherit, ea);

  SC3_BUFCOPY (e->errmsg, errmsg);
  SC3_BUFCOPY (e->filename, filename);
  e->line = line;

  /* This function returns the new error object and has no error code. */
  return e;
}

sc3_error_t        *
sc3_error_new_stack (sc3_error_t ** pstack,
                     const char *filename, int line, const char *errmsg)
{
  return sc3_error_new_stack_inherit (pstack, 0, filename, line, errmsg);
}

sc3_error_t        *
sc3_error_new_inherit (sc3_error_t ** pstack,
                       const char *filename, int line, const char *errmsg)
{
  return sc3_error_new_stack_inherit (pstack, 1, filename, line, errmsg);
}

sc3_error_t        *
sc3_error_flatten (sc3_error_t ** pe, const char *prefix, char *flatmsg)
{
  int                 stline;
  int                 remain, result;
  char                tempmsg[SC3_BUFSIZE];
  char                stbname[SC3_BUFSIZE];
  char               *pos, *out;
  const char         *stfilename, *stmsg;
  sc3_error_t        *e, *stack;
  sc3_error_kind_t    kind;

  /* We take ownership of *pe and expect an existing error and buffer */
  SC3E_INULLP (pe, e);
  SC3A_CHECK (flatmsg != NULL);

  /* So now we can go to work */
  SC3A_IS (sc3_error_is_setup, e);
  out = (prefix != NULL ? tempmsg : flatmsg);

  /* go through error stack's messages */
  remain = SC3_BUFSIZE;
  *(pos = out) = '\0';
  do {
    if (remain > 0) {
      /* access information on current stack top */
      SC3E (sc3_error_access_location (e, &stfilename, &stline));
      sc3_strcopy (stbname, SC3_BUFSIZE, stfilename);
      SC3E (sc3_error_access_message (e, &stmsg));
      SC3E (sc3_error_get_kind (e, &kind));

      /* append error location and message to output string */
      result = snprintf (pos, remain,
#if 0
                         "%s%s:%d:%c %s", pos == out ? "" : ": ",
#else
                         "%s(%s:%d:%c %s)", pos == out ? "" : " ",
#endif
                         sc3_basename (stbname), stline,
                         sc3_error_kind_char[kind], stmsg);
      SC3E (sc3_error_restore_location (e, stfilename, stline));
      SC3E (sc3_error_restore_message (e, stmsg));
      if (result < 0 || result >= remain) {
        pos = NULL;
        remain = 0;
      }
      else {
        pos += result;
        remain -= result;
      }
    }

    /* do down the error stack, we get a stack with a bumped reference */
    SC3E (sc3_error_get_stack (e, &stack));

    /* leak errors in this function are fatal to avoid infinite recursion */
    SC3E (sc3_error_unref (&e));
    e = stack;
  }
  while (e != NULL);

  /* finish construction of new flat error string */
  if (prefix != NULL) {
    sc3_snprintf (flatmsg, SC3_BUFSIZE, "%s %s", prefix, tempmsg);
  }
  return NULL;
}

sc3_error_t        *
sc3_error_accum_kind (sc3_allocator_t * alloc,
                      sc3_error_t ** pcollect, sc3_error_kind_t kind,
                      const char *filename, int line, const char *errmsg)
{
  sc3_error_t        *c, *e;

  /* check call convention */
  SC3A_IS (sc3_allocator_is_setup, alloc);
  SC3A_CHECK (pcollect != NULL);
  SC3A_CHECK (0 <= kind && kind < SC3_ERROR_KIND_LAST);

  /* Construct a new error to hold the flat message. */
  SC3E (sc3_error_new (alloc, &e));
  SC3E (sc3_error_set_location (e, filename, line));
  SC3E (sc3_error_set_message (e, errmsg));
  SC3E (sc3_error_set_kind (e, kind));

  /* if the collection is not empty, take it with us as stack */
  if ((c = *pcollect) != NULL) {
    *pcollect = NULL;
    SC3E (sc3_error_set_stack (e, &c));
  }

  /* Finalize the error object and place it as new collection. */
  SC3E (sc3_error_setup (e));
  *pcollect = e;
  return NULL;
}

sc3_error_t        *
sc3_error_accumulate (sc3_allocator_t * alloc,
                      sc3_error_t ** pcollect, sc3_error_t ** pe,
                      const char *filename, int line, const char *errmsg)
{
  sc3_error_t        *e;
  sc3_error_kind_t    kind;
  char                flatmsg[SC3_BUFSIZE];

  /* check call convention */
  SC3A_IS (sc3_allocator_is_setup, alloc);
  SC3A_CHECK (pcollect != NULL);
  SC3A_CHECK (pe != NULL);

  /* If no error comes in, there is nothing to do. */
  if ((e = *pe) == NULL) {
    return NULL;
  }

  /* The input error is owned and flattened. */
  *pe = NULL;
  SC3E (sc3_error_get_kind (e, &kind));
  SC3E (sc3_error_flatten (&e, errmsg, flatmsg));

  /* Accumulate a new error to hold the flat message. */
  SC3E (sc3_error_accum_kind (alloc, pcollect,
                              kind, filename, line, flatmsg));
  return NULL;
}

/** Accumulate a newly created error into a collection of leaks.
 * This function is a restricted version of \ref sc3_error_accum_kind.
 * We use a static allocator, which is ok for macros without context.
 * \param [in,out] leak     Pointer to error collection must not be NULL.
 *                          If the pointed-to error is not NULL, it must be
 *                          of kind \ref SC3_ERROR_LEAK or we return fatal.
 *                          On output, new leak error with input as stack.
 * \param [in] filename     File name to report in the new error object.
 * \param [in] line         Line number to report.
 * \param [in] errmsg       The message to report.
 * \return              NULL on success, fatal error otherwise.
 */
static sc3_error_t *
sc3_error_accum_leak (sc3_error_t ** leak,
                      const char *filename, int line, const char *errmsg)
{
  SC3A_CHECK (leak != NULL);
  SC3A_IS (sc3_error_is_null_or_leak, *leak);

  SC3E (sc3_error_accum_kind (sc3_allocator_nocount (), leak,
                              SC3_ERROR_LEAK, filename, line, errmsg));
  return NULL;
}

sc3_error_t        *
sc3_error_leak (sc3_error_t ** leak, sc3_error_t * e,
                const char *filename, int line, const char *errmsg)
{

  SC3A_CHECK (leak != NULL);
  SC3A_IS (sc3_error_is_null_or_leak, *leak);

  if (e != NULL) {
    char                flatmsg[SC3_BUFSIZE];

    SC3E_DEMIS (!sc3_error_is_fatal, e);
    SC3E (sc3_error_flatten (&e, errmsg, flatmsg));
    SC3E (sc3_error_accum_leak (leak, filename, line, flatmsg));
  }
  return NULL;
}

sc3_error_t        *
sc3_error_leak_demand (sc3_error_t ** leak, int x,
                       const char *filename, int line, const char *errmsg)
{
  SC3A_CHECK (leak != NULL);
  SC3A_IS (sc3_error_is_null_or_leak, *leak);

  if (!x) {
    SC3E (sc3_error_accum_leak (leak, filename, line, errmsg));
  }
  return NULL;
}

sc3_error_t        *
sc3_error_access_location (sc3_error_t * e, const char **filename, int *line)
{
  SC3E_RETVAL (filename, NULL);
  SC3E_RETVAL (line, 0);
  SC3A_IS (sc3_error_is_setup, e);

  *filename = e->filename;
  *line = e->line;
  ++e->accessed_locations;
  return NULL;
}

sc3_error_t        *
sc3_error_restore_location (sc3_error_t * e, const char *filename, int line)
{
  SC3A_IS (sc3_error_is_setup, e);
  SC3A_CHECK (filename == e->filename);
  SC3A_CHECK (line == e->line);
  SC3A_CHECK (e->accessed_locations > 0);

  --e->accessed_locations;
  return NULL;
}

sc3_error_t        *
sc3_error_access_message (sc3_error_t * e, const char **errmsg)
{
  SC3E_RETVAL (errmsg, NULL);
  SC3A_IS (sc3_error_is_setup, e);

  *errmsg = e->errmsg;
  ++e->accessed_messages;
  return NULL;
}

sc3_error_t        *
sc3_error_restore_message (sc3_error_t * e, const char *errmsg)
{
  SC3A_IS (sc3_error_is_setup, e);
  SC3A_CHECK (errmsg == e->errmsg);
  SC3A_CHECK (e->accessed_messages > 0);

  --e->accessed_messages;
  return NULL;
}

sc3_error_t        *
sc3_error_get_kind (sc3_error_t * e, sc3_error_kind_t * kind)
{
  SC3E_RETVAL (kind, SC3_ERROR_BUG);
  SC3A_IS (sc3_error_is_setup, e);

  *kind = e->kind;
  return NULL;
}

#if 0
sc3_error_t        *
sc3_error_get_severity (sc3_error_t * e, sc3_error_severity_t * sev)
{
  SC3E_RETVAL (sev, SC3_ERROR_BUG);
  SC3A_IS (sc3_error_is_setup, e);

  *sev = e->sev;
  return NULL;
}
#endif

sc3_error_t        *
sc3_error_get_stack (sc3_error_t * e, sc3_error_t ** pstack)
{
  SC3E_RETVAL (pstack, NULL);
  SC3A_IS (sc3_error_is_setup, e);

  if (e->stack != NULL) {
    SC3E (sc3_error_ref (*pstack = e->stack));
  }
  return NULL;
}

sc3_error_t        *
sc3_error_get_text_rec (sc3_error_t * e, int recursion, int rdepth,
                        char *bwork, char *buffer, size_t *bufrem)
{
  int                 printed;
  int                 eline;
  size_t              bufin;
  char                pref[8];
  const char         *bname;
  const char         *efile;
  const char         *emsg;
  sc3_error_kind_t    ekind;
  sc3_error_t        *stack;

  SC3A_IS (sc3_error_is_valid, e);
  SC3A_CHECK (buffer != NULL);
  SC3A_CHECK (*bufrem > 0);

  /* see if there is reason for recursion */
  stack = NULL;
  if (recursion != 0) {
    SC3E (sc3_error_get_stack (e, &stack));
  }

  if (stack != NULL && recursion < 0) {
    /* postorder */

    bufin = *bufrem;
    SC3E (sc3_error_get_text_rec (stack, recursion, rdepth + 1,
                                  bwork, buffer, bufrem));
    SC3A_CHECK (*bufrem < bufin);
    buffer += bufin - *bufrem;

    if (*bufrem > 0) {
      /* we replace the terminating NUL with a line break to continue */
      SC3A_CHECK (buffer[-1] == '\0');
      buffer[-1] = '\n';
    }
  }

  /* print stuff into buffer, move buffer pointer and decrease *bufrem */

  if (*bufrem > 0) {
    SC3E (sc3_error_access_location (e, &efile, &eline));
    SC3E (sc3_error_access_message (e, &emsg));
    SC3E (sc3_error_get_kind (e, &ekind));

    if (recursion == 0) {
      snprintf (pref, 8, "ET ");
    }
    else {
      snprintf (pref, 8, "E%d ", rdepth);
    }
    if (bwork == NULL) {
      bname = efile;
    }
    else {
      sc3_strcopy (bwork, SC3_BUFSIZE, efile);
      bname = sc3_basename (bwork);
    }
    printed = snprintf (buffer, *bufrem, "%s%s:%d %c:%s", pref,
                        bname, eline, sc3_error_kind_char[ekind], emsg);

    SC3E (sc3_error_restore_location (e, efile, eline));
    SC3E (sc3_error_restore_message (e, emsg));

    if (printed < 0) {
      /* something went wrong with snprintf */
      buffer[0] = '\0';
      printed = 1;
    }
    else if ((size_t) printed >= *bufrem) {
      /* output was truncated */
      printed = *bufrem;
    }
    else {
      /* count terminating NUL */
      ++printed;
    }
    buffer += printed;
    *bufrem -= printed;
  }

  if (stack != NULL && recursion > 0) {
    /* preorder */

    if (*bufrem > 0) {
      /* we replace the terminating NUL with a line break to continue */
      SC3A_CHECK (buffer[-1] == '\0');
      buffer[-1] = '\n';
      SC3E (sc3_error_get_text_rec (stack, recursion, rdepth + 1,
                                    bwork, buffer, bufrem));
    }
  }

  /* clean up and return */
  if (stack != NULL) {
    SC3E (sc3_error_unref (&stack));
  }
  return NULL;
}

sc3_error_t        *
sc3_error_get_text (sc3_error_t * e, int recursion, int dobasename,
                    char *buffer, size_t buflen)
{
  /* compute number of remaining bytes but do not return it to the caller */
  if (!dobasename) {
    SC3E (sc3_error_get_text_rec (e, recursion, 0, NULL, buffer, &buflen));
  }
  else {
    char                bwork[SC3_BUFSIZE];
    SC3E (sc3_error_get_text_rec (e, recursion, 0, bwork, buffer, &buflen));
  }
  return NULL;
}

static sc3_error_t *
sc3_error_check_text (sc3_error_t ** e, char *buffer, size_t buflen)
{
  SC3A_CHECK (e != NULL);
  SC3E (sc3_error_get_text (*e, -1, 1, buffer, buflen));
  SC3E (sc3_error_unref (e));
  return NULL;
}

int
sc3_error_check (sc3_error_t ** e, char *buffer, size_t buflen)
{
  sc3_error_t        *e2, *e3;

  /* invalid usage */
  if (e == NULL || buffer == NULL || buflen == 0) {
    if (!(buffer == NULL || buflen == 0)) {
      snprintf (buffer, buflen, "%s", "Error: Null input to sc3_error_check");
    }
    if (!(e == NULL)) {
      sc3_error_unref (e);
    }
    return -1;
  }

  /* the only case this function returns successfully */
  if (*e == NULL) {
    snprintf (buffer, buflen, "%s", "");
    return 0;
  }

  /* analyze the error passed in and try to make sense of strange cases */
  e2 = sc3_error_check_text (e, buffer, buflen);
  if (e2 != NULL) {
    /* something is wrong with internal error reporting */
    e3 = sc3_error_get_text (e2, -1, 1, buffer, buflen);
    if (e3 != NULL) {
      /* something is even more badly wrong with internal error reporting */
      snprintf (buffer, buflen, "%s",
                "Error: inconsistency inside sc3_error_get_text");
      sc3_error_unref (&e3);
    }
    sc3_error_unref (&e2);
  }
  return -1;
}
