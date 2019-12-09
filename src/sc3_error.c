/*
  This file is part of the SC Library, version 3.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

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

#include <sc3_error.h>
#include <sc3_alloc_internal.h>
#include <sc3_refcount_internal.h>

struct sc3_error
{
  sc3_refcount_t      rc;
  sc3_error_severity_t sev;
  sc3_error_sync_t    syn;
  char                errmsg[SC3_BUFLEN];
  char                filename[SC3_BUFLEN];
  int                 line;
  int                 alloced;
  sc3_error_t        *stack;
  sc3_allocator_t    *eator;
};

struct sc3_error_args
{
  int                 used;
  sc3_error_t        *values;
};

static sc3_error_t  bug =
  { {SC3_REFCOUNT_MAGIC, 1}, SC3_ERROR_FATAL, SC3_ERROR_LOCAL,
"Inconsistency or bug", "", 0, 0, NULL, NULL
};

static sc3_error_t  nom =
  { {SC3_REFCOUNT_MAGIC, 1}, SC3_ERROR_FATAL, SC3_ERROR_LOCAL,
"Out of memory", "", 0, 0, NULL, NULL
};

/** Initialize members of an error object.
 * This function is used internally.
 * It references neither stack nor eator.
 * We do not want it to call any error recursion, thus it uses no assertions.
 */
static void
sc3_error_defaults (sc3_error_t * e,
                    sc3_error_t * stack, sc3_allocator_t * eator)
{
  (void) sc3_refcount_init_invalid (&e->rc);
  e->sev = SC3_ERROR_FATAL;
  e->syn = SC3_ERROR_LOCAL;
  SC3_BUFINIT (e->errmsg);
  SC3_BUFINIT (e->filename);
  e->line = 0;
  e->alloced = 1;
  e->stack = stack;
  e->eator = eator;
}

sc3_error_t        *
sc3_error_args_new (sc3_allocator_t * eator, sc3_error_args_t ** eap)
{
  sc3_error_args_t   *ea;
  sc3_error_t        *v;

  SC3A_RETVAL (eap, NULL);
  if (eator == NULL)
    eator = sc3_allocator_nocount ();
  SC3E (sc3_allocator_ref (eator));

  SC3E_ALLOCATOR_MALLOC (eator, sc3_error_args_t, 1, ea);
  ea->used = 0;

  SC3E_ALLOCATOR_MALLOC (eator, sc3_error_t, 1, v);
  sc3_error_defaults (v, NULL, eator);
  ea->values = v;

  *eap = ea;
  return NULL;
}

static sc3_error_t *
sc3_error_unalloc (sc3_error_t * e)
{
  sc3_allocator_t    *eator;

  SC3A_CHECK (e != NULL);
  eator = e->eator;

  if (e->stack != NULL)
    SC3E (sc3_error_unref (&e->stack));

  SC3E_ALLOCATOR_FREE (eator, sc3_error_t, e);
  SC3E (sc3_allocator_unref (&eator));

  return NULL;
}

sc3_error_t        *
sc3_error_args_destroy (sc3_error_args_t ** eap)
{
  sc3_error_args_t   *ea;
  sc3_allocator_t    *eator;

  SC3A_INOUTP (eap, ea);
  SC3A_CHECK (ea->values != NULL);
  eator = ea->values->eator;

  if (!ea->used)
    SC3E (sc3_error_unalloc (ea->values));
  else
    SC3A_CHECK (ea->values == NULL);

  SC3E_ALLOCATOR_FREE (eator, sc3_error_args_t, ea);
  *eap = NULL;

  SC3E (sc3_allocator_unref (&eator));
  return NULL;
}

sc3_error_t        *
sc3_error_args_set_stack (sc3_error_args_t * ea, sc3_error_t * stack)
{
  SC3A_CHECK (ea != NULL);
  SC3A_CHECK (!ea->used && ea->values != NULL);

  if (ea->values->stack != NULL)
    SC3E (sc3_error_unref (&ea->values->stack));
  ea->values->stack = stack;

  return NULL;
}

sc3_error_t        *
sc3_error_args_set_msg (sc3_error_args_t * ea, const char *errmsg)
{
  SC3A_CHECK (ea != NULL);
  SC3A_CHECK (!ea->used && ea->values != NULL);

  SC3_BUFCOPY (ea->values->errmsg, errmsg);
  return NULL;
}

#if 0
void                sc3_error_args_set_severity (sc3_error_args_t * ea,
                                                 sc3_error_severity_t sev);
void                sc3_error_args_set_sync (sc3_error_args_t * ea,
                                             sc3_error_sync_t syn);
void                sc3_error_args_set_file (sc3_error_args_t * ea,
                                             const char *filename);
void                sc3_error_args_set_line (sc3_error_args_t * ea, int line);
void                sc3_error_args_set_msgf (sc3_error_args_t * ea,
                                             const char *errfmt, ...)
  __attribute__ ((format (printf, 2, 3)));
#endif

sc3_error_t        *
sc3_error_new (sc3_error_args_t ** eap, sc3_error_t ** ep)
{
  sc3_error_args_t   *ea;
  sc3_error_t        *e;
  sc3_allocator_t    *eator;

  SC3A_INOUTP (eap, ea);
  SC3A_CHECK (!ea->used && ea->values != NULL);
  eator = ea->values->eator;
  SC3A_RETVAL (ep, NULL);

  e = ea->values;
  ea->values = NULL;
  ea->used = 1;
  sc3_refcount_init (&e->rc);
  SC3E (sc3_allocator_ref (eator));

  SC3E (sc3_error_args_destroy (eap));
  *ep = e;
  return NULL;
}

sc3_error_t        *
sc3_error_ref (sc3_error_t * e)
{
  SC3A_CHECK (e != NULL);
  if (e->alloced)
    SC3E (sc3_refcount_ref (&e->rc));
  return NULL;
}

sc3_error_t        *
sc3_error_unref (sc3_error_t ** ep)
{
  int                 waslast;
  sc3_error_t        *e;

  SC3A_INOUTP (ep, e);
  if (!e->alloced) {
    /* It is our convention that non-alloced errors must not have a stack. */
    SC3A_CHECK (e->stack == NULL);
    return NULL;
  }

  SC3E (sc3_refcount_unref (&e->rc, &waslast));
  if (waslast) {
    SC3E (sc3_error_unalloc (e));
    *ep = NULL;
  }
  return NULL;
}

sc3_error_t        *
sc3_error_destroy (sc3_error_t ** ep)
{
  SC3E (sc3_error_unref (ep));
  SC3E_DEMAND (*ep == NULL);
  return NULL;
}

sc3_error_t        *
sc3_error_new_fatal (const char *filename, int line, const char *errmsg)
{
  sc3_error_t        *e;
  sc3_allocator_t    *ea;

  /* Avoid infinite loop when out of memory. */

  if (filename == NULL || errmsg == NULL)
    return &bug;

  /* Any allocated allocator would have to be ref'd here */
  ea = sc3_allocator_nocount ();
  e = (sc3_error_t *) sc3_allocator_malloc_noerr (ea, sizeof (sc3_error_t));
  if (e == NULL)
    return &nom;
  sc3_error_defaults (e, NULL, ea);

  SC3_BUFCOPY (e->errmsg, errmsg);
  SC3_BUFCOPY (e->filename, filename);
  e->line = line;

  /* This function returns the new error object and has no error code. */
  return e;
}

/** This function takes over one reference to stack. */
sc3_error_t        *
sc3_error_new_stack (sc3_error_t * stack, const char *filename,
                     int line, const char *errmsg)
{
  sc3_error_t        *e;
  sc3_allocator_t    *ea;

  /* Avoid infinite loop when out of memory. */

  if (stack == NULL || !sc3_refcount_is_valid (&stack->rc))
    return &bug;
  if (filename == NULL || errmsg == NULL)
    return stack;

  /* Any allocated allocator would have to be ref'd here */
  ea = sc3_allocator_nocount ();
  e = (sc3_error_t *) sc3_allocator_malloc_noerr (ea, sizeof (sc3_error_t));
  if (e == NULL) {
    return stack;
  }
  sc3_error_defaults (e, stack, ea);

  SC3_BUFCOPY (e->errmsg, errmsg);
  SC3_BUFCOPY (e->filename, filename);
  e->line = line;

  /* This function returns the new error object and has no error code. */
  return e;
}

int
sc3_error_is_fatal (sc3_error_t * e)
{
  return e != NULL && sc3_refcount_is_valid (&e->rc) &&
    e->sev == SC3_ERROR_FATAL;
}
