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

/* TODO think about an error having multiple messages/parents */

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
};

#if 0
static sc3_error_t  ebug = { {SC3_REFCOUNT_MAGIC, 1}, NULL, 1, SC3_ERROR_BUG,
"Inconsistency or bug", __FILE__, __LINE__, 0, NULL
};
#endif

static sc3_error_t  enom =
  { {SC3_REFCOUNT_MAGIC, 1}, NULL, 1, SC3_ERROR_MEMORY,
"Out of memory", __FILE__, __LINE__, 0, NULL
};

static sc3_error_t  enull = { {SC3_REFCOUNT_MAGIC, 1}, NULL, 1, SC3_ERROR_BUG,
"Argument must not be NULL", __FILE__, __LINE__, 0, NULL
};

static sc3_error_t  esetup =
  { {SC3_REFCOUNT_MAGIC, 1}, NULL, 1, SC3_ERROR_BUG,
"Error argument must be setup", __FILE__, __LINE__, 0, NULL
};

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
  }
  SC3E_TEST (0 <= e->kind && e->kind < SC3_ERROR_KIND_LAST, reason);
#if 0
  SC3E_TEST (0 <= e->sev && e->sev < SC3_ERROR_SEVERITY_LAST, reason);
  SC3E_TEST (0 <= e->syn && e->syn < SC3_ERROR_SYNC_LAST, reason);
#endif
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
  /* We do not classify the kind SC3_ERROR_LEAK as fatal. */

  SC3E_IS (sc3_error_is_setup, e, reason);
  switch (e->kind) {
  case SC3_ERROR_FATAL:
  case SC3_ERROR_BUG:
  case SC3_ERROR_MEMORY:
  case SC3_ERROR_NETWORK:
    SC3E_YES (reason);
  default:
    SC3E_NO (reason, "Error is not of the fatal kind");
  }
}

int
sc3_error_is_leak (const sc3_error_t * e, char *reason)
{
  SC3E_IS (sc3_error_is_setup, e, reason);
  SC3E_TEST (e->kind == SC3_ERROR_LEAK, reason);
  SC3E_YES (reason);
}

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
  SC3E_ALLOCATOR_MALLOC (eator, sc3_error_t, 1, e);
  sc3_error_defaults (e, NULL, 0, 0, eator);
  SC3A_IS (sc3_error_is_new, e);

  *ep = e;
  return NULL;
}

sc3_error_t        *
sc3_error_set_stack (sc3_error_t * e, sc3_error_t ** pstack)
{
  sc3_error_t        *stack;

  SC3E_INULLP (pstack, stack);
  SC3A_IS (sc3_error_is_new, e);
  if (stack != NULL) {
    SC3A_IS (sc3_error_is_setup, stack);
  }
  if (e->stack != NULL) {
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

    if (e->stack != NULL) {
      SC3E (sc3_error_unref (&e->stack));
    }

    eator = e->eator;
    SC3E_ALLOCATOR_FREE (eator, sc3_error_t, e);
    SC3E (sc3_allocator_unref (&eator));
  }
  return NULL;
}

sc3_error_t        *
sc3_error_destroy (sc3_error_t ** ep)
{
  sc3_error_t        *e;
  int                 leak = 0;

  SC3E_INULLP (ep, e);
  if (!sc3_refcount_is_last (&e->rc, NULL)) {
    /* Reference leak encountered, which may not occur with static errors. */
    SC3A_CHECK (e->alloced);
    leak = 1;
  }
  /* This function checks error object consistency as a side effect.  */
  SC3E (sc3_error_unref (&e));

  SC3A_CHECK (e == NULL || (!e->alloced ^ leak));
  return leak ?
    sc3_error_new_kind (SC3_ERROR_LEAK, __FILE__, __LINE__,
                        "Reference leak in sc3_error_destroy") : NULL;
}

void
sc3_error_destroy_noerr (sc3_error_t ** pe, char *flatmsg)
{
  int                 remain, result;
  int                 line;
  const char         *filename, *msg;
  char               *pos, *bname;
  sc3_error_t        *e, *inte, *stack;

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
    SC3E_SET (inte, sc3_error_get_location (e, &filename, &line));
    if (inte == NULL && (bname = strdup (filename)) != NULL) {
      filename = sc3_basename (bname);
    }
    SC3E_NULL_SET (inte, sc3_error_get_message (e, &msg));
    if (inte == NULL && remain > 0) {
      result = snprintf (pos, remain, "%s%s:%d: %s",
                         pos == flatmsg ? "" : ": ", filename, line, msg);
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
}

sc3_error_t        *
sc3_error_new_kind (sc3_error_kind_t kind,
                    const char *filename, int line, const char *errmsg)
{
  sc3_error_t        *e;
  sc3_allocator_t    *ea;

  /* Avoid infinite loop when out of memory. */

  if (filename == NULL || errmsg == NULL) {
    return &enull;
  }

  /* Any allocated allocator would have to be ref'd here. */
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

  /* Avoid infinite loop when out of memory. */

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

  /* Any allocated allocator would have to be ref'd here. */
  /* Any counting allocator would need to be thread private.
     Alternative: only allow malloc_noerr for non-counting allocators. */
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
sc3_error_get_location (sc3_error_t * e, const char **filename, int *line)
{
  SC3E_RETOPT (filename, "");
  SC3E_RETOPT (line, 0);
  SC3A_IS (sc3_error_is_setup, e);

  if (filename != NULL) {
    *filename = e->filename;
  }
  if (line != NULL) {
    *line = e->line;
  }
  return NULL;
}

sc3_error_t        *
sc3_error_get_message (sc3_error_t * e, const char **errmsg)
{
  SC3E_RETOPT (errmsg, "");
  SC3A_IS (sc3_error_is_setup, e);

  if (errmsg != NULL) {
    *errmsg = e->errmsg;
  }
  return NULL;
}

sc3_error_t        *
sc3_error_get_kind (sc3_error_t * e, sc3_error_kind_t * kind)
{
  SC3E_RETOPT (kind, SC3_ERROR_BUG);
  SC3A_IS (sc3_error_is_setup, e);

  if (kind != NULL) {
    *kind = e->kind;
  }
  return NULL;
}

#if 0
sc3_error_t        *
sc3_error_get_severity (sc3_error_t * e, sc3_error_severity_t * sev)
{
  SC3E_RETOPT (sev, SC3_ERROR_BUG);
  SC3A_IS (sc3_error_is_setup, e);

  if (sev != NULL) {
    *sev = e->sev;
  }
  return NULL;
}
#endif

sc3_error_t        *
sc3_error_get_stack (sc3_error_t * e, sc3_error_t ** pstack)
{
  SC3E_RETOPT (pstack, NULL);
  SC3A_IS (sc3_error_is_setup, e);

  if (pstack != NULL && e->stack != NULL) {
    SC3E (sc3_error_ref (*pstack = e->stack));
  }
  return NULL;
}
