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
#include <sc3_refcount_internal.h>

struct sc3_array
{
  sc3_refcount_t      rc;
  sc3_allocator_t    *aator;
  int                 setup;

  /* parameters fixed after setup call */
  int                 resizable, initzero;
  int                 ecount, ealloc;
  size_t              esize;

  /* member variables initialized in setup call */
  char               *mem;
};

int
sc3_array_is_valid (sc3_array_t * a, char *reason)
{
  SC3E_TEST (a != NULL, reason);
  SC3E_IS (sc3_refcount_is_valid, &a->rc, reason);
  SC3E_IS (sc3_allocator_is_setup, a->aator, reason);
  SC3E_TEST (a->ecount >= 0 && a->ealloc >= 0, reason);

  /* check internal allocation logic depending on setup status */
  if (!a->setup) {
    SC3E_TEST (a->mem == NULL, reason);
  }
  else {
    SC3E_TEST (a->mem != NULL || a->ecount * a->esize == 0, reason);
    SC3E_TEST (SC3_ISPOWOF2 (a->ealloc), reason);
    SC3E_TEST (a->ecount <= a->ealloc, reason);
  }
  SC3E_YES (reason);
}

int
sc3_array_is_new (sc3_array_t * a, char *reason)
{
  SC3E_IS (sc3_array_is_valid, a, reason);
  SC3E_TEST (!a->setup, reason);
  SC3E_YES (reason);
}

int
sc3_array_is_setup (sc3_array_t * a, char *reason)
{
  SC3E_IS (sc3_array_is_valid, a, reason);
  SC3E_TEST (a->setup, reason);
  SC3E_YES (reason);
}

int
sc3_array_is_resizable (sc3_array_t * a, char *reason)
{
  SC3E_IS (sc3_array_is_setup, a, reason);
  SC3E_TEST (a->resizable, reason);
  SC3E_YES (reason);
}

sc3_error_t        *
sc3_array_new (sc3_allocator_t * aator, sc3_array_t ** ap)
{
  sc3_array_t        *a;

  SC3E_RETVAL (ap, NULL);
  SC3A_IS (sc3_allocator_is_setup, aator);

  SC3E (sc3_allocator_ref (aator));
  SC3E_ALLOCATOR_CALLOC (aator, sc3_array_t, 1, a);
  SC3E (sc3_refcount_init (&a->rc));
  a->esize = 1;
  a->ealloc = 8;
  a->resizable = 1;
  a->aator = aator;
  SC3A_IS (sc3_array_is_new, a);

  *ap = a;
  return NULL;
}

sc3_error_t        *
sc3_array_set_elem_size (sc3_array_t * a, size_t esize)
{
  SC3A_IS (sc3_array_is_new, a);
  a->esize = esize;
  return NULL;
}

sc3_error_t        *
sc3_array_set_elem_count (sc3_array_t * a, int ecount)
{
  SC3A_IS (sc3_array_is_new, a);
  SC3A_CHECK (0 <= ecount && ecount <= SC3_INT_HPOW);
  a->ecount = ecount;
  return NULL;
}

sc3_error_t        *
sc3_array_set_elem_alloc (sc3_array_t * a, int ealloc)
{
  SC3A_IS (sc3_array_is_new, a);
  SC3A_CHECK (0 <= ealloc && ealloc <= SC3_INT_HPOW);
  a->ealloc = ealloc;
  return NULL;
}

sc3_error_t        *
sc3_array_set_resizable (sc3_array_t * a, int resizable)
{
  SC3A_IS (sc3_array_is_new, a);
  a->resizable = resizable;
  return NULL;
}

sc3_error_t        *
sc3_array_set_initzero (sc3_array_t * a, int initzero)
{
  SC3A_IS (sc3_array_is_new, a);
  a->initzero = initzero;
  return NULL;
}

sc3_error_t        *
sc3_array_setup (sc3_array_t * a)
{
  const int           ib = SC3_INT_BITS;
  int                 lg;
  size_t              abytes;

  SC3A_IS (sc3_array_is_new, a);

  /* set a->ealloc to a fitting power of 2 */
  lg = sc3_log2_ceil (SC3_MAX (a->ealloc, a->ecount), ib - 1);
  SC3A_CHECK (0 <= lg && lg < ib - 1);
  SC3A_CHECK (a->ecount <= (1 << lg));
  SC3A_CHECK (a->ealloc <= (1 << lg));
  abytes = (a->ealloc = 1 << lg) * a->esize;

  /* allocate array storage */
  if (!a->initzero) {
    SC3E_ALLOCATOR_MALLOC (a->aator, char, abytes, a->mem);
  }
  else {
    SC3E_ALLOCATOR_CALLOC (a->aator, char, abytes, a->mem);
  }

  /* set array to setup state */
  a->setup = 1;
  SC3A_IS (sc3_array_is_setup, a);
  return NULL;
}

sc3_error_t        *
sc3_array_ref (sc3_array_t * a)
{
  SC3A_IS (sc3_array_is_setup, a);
  SC3E (sc3_refcount_ref (&a->rc));
  return NULL;
}

sc3_error_t        *
sc3_array_unref (sc3_array_t ** ap)
{
  int                 waslast;
  sc3_allocator_t    *aator;
  sc3_array_t        *a;

  SC3E_INOUTP (ap, a);
  SC3A_IS (sc3_array_is_valid, a);
  SC3E (sc3_refcount_unref (&a->rc, &waslast));
  if (waslast) {
    *ap = NULL;

    aator = a->aator;
    if (a->setup) {
      /* deallocate element storage */
      SC3E_ALLOCATOR_FREE (aator, char, a->mem);
    }
    SC3E_ALLOCATOR_FREE (aator, sc3_array_t, a);
    SC3E (sc3_allocator_unref (&aator));
  }
  return NULL;
}

sc3_error_t        *
sc3_array_destroy (sc3_array_t ** ap)
{
  sc3_array_t        *a;

  SC3E_INULLP (ap, a);
  SC3E_DEMIS (sc3_refcount_is_last, &a->rc);
  SC3E (sc3_array_unref (&a));

  SC3A_CHECK (a == NULL);
  return NULL;
}

sc3_error_t        *
sc3_array_resize (sc3_array_t * a, int new_ecount)
{
  SC3A_IS (sc3_array_is_resizable, a);
  SC3A_CHECK (0 <= new_ecount && new_ecount <= SC3_INT_HPOW);

  /* query whether the allocation is sufficient */
  if (new_ecount > a->ealloc) {

    /* we need to enlarge allocation */
    do {
      a->ealloc *= 2;
    }
    while (new_ecount > a->ealloc);
    SC3E_ALLOCATOR_REALLOC (a->aator, char, a->ealloc * a->esize, a->mem);
  }

  /* record new element count */
  a->ecount = new_ecount;
  return NULL;
}

sc3_error_t        *
sc3_array_push_count (sc3_array_t * a, int n, void **p)
{
  SC3E_RETVAL (p, NULL);
  SC3A_IS (sc3_array_is_resizable, a);
  SC3A_CHECK (0 <= n && a->ecount + n <= SC3_INT_HPOW);

  if (n > 0) {
    int                 old_ecount = a->ecount;
    SC3E (sc3_array_resize (a, old_ecount + n));
    SC3E (sc3_array_index (a, old_ecount, p));
  }
  return NULL;
}

sc3_error_t        *
sc3_array_push (sc3_array_t * a, void **p)
{
  SC3E (sc3_array_push_count (a, 1, p));
  return NULL;
}

void               *
sc3_array_push_noerr (sc3_array_t * a)
{
#ifdef SC_ENABLE_DEBUG
  if (!sc3_array_is_resizable (a, NULL) || a->ecount >= SC3_INT_HPOW) {
    return NULL;
  }
#endif

  /* we may need to enlarge allocation */
  if (a->ecount == a->ealloc) {
    void               *p;
    sc3_error_t        *e;

    p = a->mem;
    if ((e = sc3_allocator_realloc
         (a->aator, (a->ealloc *= 2) * a->esize, &p)) != NULL) {
      (void *) sc3_error_destroy (&e);
      return NULL;
    }
    a->mem = (char *) p;
  }

  /* record new element count */
  return a->mem + a->ecount++ * a->esize;
}

sc3_error_t        *
sc3_array_index (sc3_array_t * a, int i, void **p)
{
  SC3A_ONULL (p);
  SC3A_IS (sc3_array_is_setup, a);
  SC3A_CHECK (0 <= i && i < a->ecount);

  *p = a->mem + i * a->esize;
  return NULL;
}

void               *
sc3_array_index_noerr (sc3_array_t * a, int i)
{
#ifdef SC_ENABLE_DEBUG
  if (!sc3_array_is_setup (a, NULL) || i < 0 || i >= a->ecount) {
    return NULL;
  }
#endif
  return a->mem + i * a->esize;
}
