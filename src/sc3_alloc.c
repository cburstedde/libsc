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
#include <sc3_refcount_internal.h>

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

/** This allocator is not thread-safe since it is counting and not locked. */
static sc3_allocator_t nta =
  { {SC3_REFCOUNT_MAGIC, 1}, 0, 0, 1, 0, 0, 0, NULL };

sc3_allocator_t    *
sc3_allocator_nocount (void)
{
  return &nca;
}

sc3_allocator_t    *
sc3_allocator_nothread (void)
{
  return &nta;
}

sc3_error_t        *
sc3_allocator_args_new (sc3_allocator_t * oa, sc3_allocator_args_t ** aap)
{
  sc3_allocator_args_t *aa;

  SC3E_RETVAL (aap, NULL);
  SC3A_CHECK (oa != NULL);

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

  SC3E_INULLP (aap, aa);

  oa = aa->oa;
  SC3E_ALLOCATOR_FREE (oa, sc3_allocator_args_t, aa);
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

  SC3E_INULLP (aap, aa);
  SC3E_RETVAL (ap, NULL);

  SC3E_ALLOCATOR_MALLOC (aa->oa, sc3_allocator_t, 1, a);
  SC3E (sc3_refcount_init (&a->rc));
  a->align = aa->align;
  a->alloced = 1;
  a->counting = 1;
  a->num_malloc = a->num_calloc = a->num_free = 0;
  a->oa = aa->oa;
  SC3E (sc3_allocator_ref (a->oa));
  SC3E (sc3_allocator_args_destroy (&aa));

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

  SC3E_INOUTP (ap, a);
  if (!a->alloced) {
    return NULL;
  }

  SC3E (sc3_refcount_unref (&a->rc, &waslast));
  if (waslast) {
    *ap = NULL;

    if (a->counting) {
      SC3E_DEMAND (a->num_malloc + a->num_calloc == a->num_free);
    }

    oa = a->oa;
    SC3E_ALLOCATOR_FREE (oa, sc3_allocator_t, a);
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
sc3_allocator_strdup (sc3_allocator_t * a, const char *src, char **dest)
{
  char               *p;

  SC3A_CHECK (a != NULL);
  SC3A_CHECK (src != NULL);
  SC3E_RETVAL (dest, NULL);

  /* TODO: use same allocation mechanism as allocator_malloc below */

  p = SC3_STRDUP (src);
  SC3E_DEMAND (src == NULL || p != NULL);

  if (a->counting)
    ++a->num_malloc;

  *dest = p;
  return NULL;
}

void               *
sc3_allocator_malloc_noerr (sc3_allocator_t * a, size_t size)
{
  char               *p;

  /* TODO: use same allocation mechanism as allocator_malloc below */

  if (a == NULL)
    return NULL;

  p = SC3_MALLOC (char, size);
  if (size > 0 && p == NULL)
    return NULL;

  if (a->counting)
    ++a->num_malloc;

  return (void *) p;
}

sc3_error_t        *
sc3_allocator_malloc (sc3_allocator_t * a, size_t size, void **ptr)
{
  char               *p;

  SC3A_CHECK (a != NULL);
  SC3E_RETVAL (ptr, NULL);

  /* TODO: alloc bigger block and write align and debug info into beginning */

  p = SC3_MALLOC (char, size);
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
