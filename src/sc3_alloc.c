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

#include <sc3_alloc.h>
#include <sc3_refcount.h>

/*** TODO implement reference counting */

struct sc3_allocator_args
{
  int                 align;
  sc3_allocator_t    *oa;
};

struct sc3_allocator
{
  sc3_refcount_t      rc;
  int                 align;
  int                 alloced;
  int                 counting;
  long                num_malloc, num_calloc, num_free;
  sc3_allocator_t    *oa;
};

/** This allocator is thread-safe since it is not counting anything. */
static sc3_allocator_t nca =
  { {SC3_REFCOUNT_MAGIC, 1}, 0, 0, 0, 0, 0, 0, NULL };

sc3_allocator_t *
sc3_allocator_nocount (void)
{
  return &nca;
}

sc3_error_t        *
sc3_allocator_args_new (sc3_allocator_t * oa, sc3_allocator_args_t ** aap)
{
  sc3_allocator_args_t *aa;

  SC3A_RETVAL (aap, NULL);

  if (oa == NULL)
    oa = sc3_allocator_nocount ();
  SC3E (sc3_allocator_ref (oa));

  SC3E_ALLOCATOR_MALLOC (oa, sc3_allocator_args_t, 1, aa);
  aa->align = 0;
  aa->oa = oa;

  *aap = aa;
  return NULL;
}

sc3_error_t        *
sc3_allocator_args_destroy (sc3_allocator_args_t ** aap)
{
  sc3_allocator_args_t *aa;
  sc3_allocator_t    *oa;

  SC3A_INOUTP (aap, aa);

  oa = aa->oa;
  SC3E_ALLOCATOR_FREE (oa, sc3_allocator_args_t, aa);
  *aap = NULL;

  SC3E (sc3_allocator_unref (&oa));
  return NULL;
}

sc3_error_t        *
sc3_allocator_args_set_align (sc3_allocator_args_t * aa, int align)
{
  SC3A_CHECK (aa != NULL);
  SC3A_CHECK (align == 0 || SC3_ISPOWOF2 (align));

  aa->align = align;
  return NULL;
}

sc3_error_t        *
sc3_allocator_new (sc3_allocator_args_t ** aap, sc3_allocator_t ** ap)
{
  sc3_allocator_args_t *aa;
  sc3_allocator_t    *a;

  SC3A_INOUTP (aap, aa);
  SC3A_RETVAL (ap, NULL);

  SC3E_ALLOCATOR_MALLOC (aa->oa, sc3_allocator_t, 1, a);
  a->align = aa->align;
  a->alloced = 1;
  a->counting = 1;
  a->num_malloc = a->num_calloc = a->num_free = 0;
  sc3_refcount_init (&a->rc);
  a->oa = aa->oa;
  SC3E (sc3_allocator_ref (a->oa));

  SC3E (sc3_allocator_args_destroy (aap));
  *ap = a;
  return NULL;
}

sc3_error_t        *
sc3_allocator_ref (sc3_allocator_t * a)
{
  SC3A_CHECK (a != NULL);
  if (a->alloced)
    SC3E (sc3_refcount_ref (&a->rc));
  return NULL;
}

sc3_error_t        *
sc3_allocator_unref (sc3_allocator_t ** ap)
{
  int                 waslast;
  sc3_allocator_t    *a, *oa;

  SC3A_INOUTP (ap, a);
  if (!a->alloced)
    return NULL;

  SC3E (sc3_refcount_unref (&a->rc, &waslast));
  if (waslast) {
    if (a->counting)
      SC3E_DEMAND (a->num_malloc + a->num_calloc == a->num_free);

    oa = a->oa;
    SC3E_ALLOCATOR_FREE (oa, sc3_allocator_t, a);
    *ap = NULL;

    SC3E (sc3_allocator_unref (&oa));
  }
  return NULL;
}

sc3_error_t        *
sc3_allocator_destroy (sc3_allocator_t ** ap)
{
  SC3E (sc3_allocator_unref (ap));
  SC3E_DEMAND (*ap == NULL);
  return NULL;
}

sc3_error_t        *
sc3_allocator_strdup (sc3_allocator_t * a,
                      const char * src, char ** dest)
{
  char               *p;

  SC3A_CHECK (a != NULL);
  SC3A_CHECK (src != NULL);
  SC3A_RETVAL (dest, NULL);

  /* TODO: use same allocation mechanism as allocator_malloc below */

  p = SC3_STRDUP (src);
  SC3E_DEMAND (src == NULL || p != NULL);

  if (a->counting)
    ++a->num_malloc;

  *dest = p;
  return NULL;
}

sc3_error_t        *
sc3_allocator_malloc (sc3_allocator_t * a, size_t size, void **ptr)
{
  char               *p;

  SC3A_CHECK (a != NULL);
  SC3A_RETVAL (ptr, NULL);

  /* TODO: alloc bigger block and write align and debug info into beginning */

  p = SC3_MALLOC (char, size);

  /* TODO: when malloc fails, don't go into SC3E macros.
           Return some static error object that states this. */

  SC3E_DEMAND (size == 0 || p != NULL);

  if (a->counting)
    ++a->num_malloc;

  *ptr = (void *) p;
  return NULL;
}

sc3_error_t        *
sc3_allocator_calloc (sc3_allocator_t * a, size_t nmemb, size_t size,
                      void **ptr)
{
  /* TODO: adapt allocator_malloc function and call calloc inside */
  SC3E (sc3_allocator_malloc (a, nmemb * size, ptr));
  memset (*ptr, 0, nmemb * size);
  return NULL;
}

sc3_error_t        *
sc3_allocator_free (sc3_allocator_t * a, void *ptr)
{
  SC3A_CHECK (a != NULL);

  /* TODO: verify that ptr has been allocated by this allocator */

  if (a->counting)
    ++a->num_free;

  SC3_FREE (ptr);
  return NULL;
}
