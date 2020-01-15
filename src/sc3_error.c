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

#include <sc3_error.h>
#include <sc3_alloc_internal.h>
#include <sc3_refcount_internal.h>

struct sc3_error
{
  sc3_refcount_t      rc;
  sc3_allocator_t    *eator;
  int                 setup;

  sc3_error_severity_t sev;
  sc3_error_sync_t    syn;
  char                errmsg[SC3_BUFSIZE];
  char                filename[SC3_BUFSIZE];
  int                 line;
  int                 alloced;
  sc3_error_t        *stack;
};

/* TODO: write functions to make bug and nom available */

static sc3_error_t  bug =
  { {SC3_REFCOUNT_MAGIC, 1}, NULL, 1, SC3_ERROR_FATAL, SC3_ERROR_LOCAL,
"Inconsistency or bug", "", 0, 0, NULL
};

static sc3_error_t  nom =
  { {SC3_REFCOUNT_MAGIC, 1}, NULL, 1, SC3_ERROR_FATAL, SC3_ERROR_LOCAL,
"Out of memory", "", 0, 0, NULL
};

int
sc3_error_is_valid (sc3_error_t * e)
{
  if (e == NULL || !sc3_refcount_is_valid (&e->rc)) {
    return 0;
  }
  if (!sc3_allocator_is_setup (e->eator)) {
    return 0;
  }
  if (e->stack != NULL && !sc3_error_is_setup (e->stack)) {
    return 0;
  }
  if (!(0 <= e->sev && e->sev < SC3_ERROR_SEVERITY_LAST)) {
    return 0;
  }
  if (!(0 <= e->syn && e->syn < SC3_ERROR_SYNC_LAST)) {
    return 0;
  }
  return 1;
}

int
sc3_error_is_new (sc3_error_t * e)
{
  return sc3_error_is_valid (e) && !e->setup;
}

int
sc3_error_is_setup (sc3_error_t * e)
{
  return sc3_error_is_valid (e) && e->setup;
}

int
sc3_error_is_fatal (sc3_error_t * e)
{
  return sc3_error_is_setup (e) && e->sev == SC3_ERROR_FATAL;
}

static void
sc3_error_defaults (sc3_error_t * e, sc3_error_t * stack,
                    int setup, int inherit, sc3_allocator_t * eator)
{
  memset (e, 0, sizeof (sc3_error_t));
  sc3_refcount_init (&e->rc);
  e->eator = eator;
  e->setup = setup;

  e->sev = SC3_ERROR_FATAL;
  e->syn = SC3_ERROR_LOCAL;
  e->alloced = 1;
  e->stack = stack;
  if (inherit && stack != NULL) {
    e->sev = stack->sev;
  }
}

sc3_error_t        *
sc3_error_new (sc3_allocator_t * eator, sc3_error_t ** ep)
{
  sc3_error_t        *e;

  SC3E_RETVAL (ep, NULL);
  SC3A_CHECK (sc3_allocator_is_setup (eator));

  SC3E (sc3_allocator_ref (eator));
  SC3E_ALLOCATOR_MALLOC (eator, sc3_error_t, 1, e);
  sc3_error_defaults (e, NULL, 0, 0, eator);
  SC3A_CHECK (sc3_error_is_new (e));

  *ep = e;
  return NULL;
}

sc3_error_t        *
sc3_error_set_stack (sc3_error_t * e, sc3_error_t ** pstack)
{
  sc3_error_t        *stack;

  SC3E_INULLP (pstack, stack);
  SC3A_CHECK (sc3_error_is_new (e));
  SC3A_CHECK (stack == NULL || sc3_error_is_setup (stack));

  if (e->stack != NULL) {
    SC3E (sc3_error_unref (&e->stack));
  }
  e->stack = stack;
  return NULL;
}

sc3_error_t        *
sc3_error_set_location (sc3_error_t * e, const char *filename, int line)
{
  SC3A_CHECK (sc3_error_is_new (e));
  SC3A_CHECK (filename != NULL);

  SC3_BUFCOPY (e->filename, filename);
  e->line = line;
  return NULL;
}

sc3_error_t        *
sc3_error_set_message (sc3_error_t * e, const char *errmsg)
{
  SC3A_CHECK (sc3_error_is_new (e));
  SC3A_CHECK (errmsg != NULL);

  SC3_BUFCOPY (e->errmsg, errmsg);
  return NULL;
}

sc3_error_t        *
sc3_error_set_severity (sc3_error_t * e, sc3_error_severity_t sev)
{
  SC3A_CHECK (sc3_error_is_new (e));
  SC3A_CHECK (0 <= sev && sev < SC3_ERROR_SEVERITY_LAST);

  e->sev = sev;
  return NULL;
}

#if 0
void                sc3_error_set_sync (sc3_error_t * ea,
                                        sc3_error_sync_t syn);
void                sc3_error_set_msgf (sc3_error_t * ea,
                                        const char *errfmt, ...)
  __attribute__ ((format (printf, 2, 3)));
#endif

sc3_error_t        *
sc3_error_setup (sc3_error_t * e)
{
  SC3A_CHECK (sc3_error_is_new (e));

  e->setup = 1;
  SC3A_CHECK (sc3_error_is_setup (e));
  return NULL;
}

sc3_error_t        *
sc3_error_ref (sc3_error_t * e)
{
  SC3A_CHECK (sc3_error_is_setup (e));
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
  SC3A_CHECK (sc3_error_is_valid (e));

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

sc3_error_t         *
sc3_error_destroy (sc3_error_t ** ep)
{
  sc3_error_t        *e;

  SC3E_INULLP (ep, e);
  SC3E_DEMAND (sc3_refcount_is_last (&e->rc));
  SC3E (sc3_error_unref (&e));

  SC3A_CHECK (e == NULL || !e->alloced);
  return NULL;
}

sc3_error_t        *
sc3_error_new_fatal (const char *filename, int line, const char *errmsg)
{
  sc3_error_t        *e;
  sc3_allocator_t    *ea;

  /* Avoid infinite loop when out of memory. */

  if (filename == NULL || errmsg == NULL) {
    return &bug;
  }

  /* Any allocated allocator would have to be ref'd here. */
  ea = sc3_allocator_nocount ();
  e = (sc3_error_t *) sc3_allocator_malloc_noerr (ea, sizeof (sc3_error_t));
  if (e == NULL) {
    return &nom;
  }
  sc3_error_defaults (e, NULL, 1, 0, ea);

  SC3_BUFCOPY (e->errmsg, errmsg);
  SC3_BUFCOPY (e->filename, filename);
  e->line = line;

  /* This function returns the new error object and has no error code. */
  return e;
}

/** This function takes over one reference to stack. */
static sc3_error_t *
sc3_error_new_stack_inherit (sc3_error_t ** pstack, int inherit,
                             const char *filename, int line,
                             const char *errmsg)
{
  sc3_error_t        *stack, *e;
  sc3_allocator_t    *ea;

  /* Avoid infinite loop when out of memory. */

  if (pstack == NULL) {
    return &bug;
  }
  stack = *pstack;
  *pstack = NULL;
  if (!sc3_error_is_setup (stack)) {
    return &bug;
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

void
sc3_error_get_location (sc3_error_t * e, const char **filename, int *line)
{
  if (filename != NULL) {
    *filename = e != NULL && e->filename != NULL ? e->filename : "";
  }
  if (line != NULL) {
    *line = e != NULL ? e->line : 0;
  }
}

void
sc3_error_get_message (sc3_error_t * e, const char **errmsg)
{
  if (errmsg != NULL) {
    *errmsg = e != NULL && e->errmsg != NULL ? e->errmsg : "";
  }
}

void
sc3_error_get_severity (sc3_error_t * e, sc3_error_severity_t * sev)
{
  if (sev != NULL) {
    *sev = e != NULL ? e->sev : SC3_ERROR_FATAL;
  }
}

sc3_error_t        *
sc3_error_get_stack (sc3_error_t * e, sc3_error_t ** pstack)
{
  SC3E_ONULLP (pstack);
  SC3A_CHECK (sc3_error_is_setup (e));

  if (e->stack != NULL) {
    SC3E (sc3_error_ref (*pstack = e->stack));
  }
  return NULL;
}
