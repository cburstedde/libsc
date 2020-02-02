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

struct sc3_allocator
{
  sc3_refcount_t      rc;
  sc3_allocator_t    *oa;
  int                 setup;

  size_t              align;
  int                 alloced;
  int                 counting;

  long                num_malloc, num_calloc, num_free;
  size_t              total_size;
};

typedef union sc3_alloc_item
{
  void               *ptr;
  size_t              siz;
}
sc3_alloc_item_t;

/** This allocator is thread-safe since it is not counting anything. */
static sc3_allocator_t nca =
  { {SC3_REFCOUNT_MAGIC, 1}, NULL, 1, 0, 0, 0, 0, 0, 0, 0 };

/** This allocator is not thread-safe since it is counting and not locked. */
static sc3_allocator_t nta =
  { {SC3_REFCOUNT_MAGIC, 1}, NULL, 1, 0, 0, 1, 0, 0, 0, 0 };

int
sc3_allocator_is_valid (sc3_allocator_t * a, char *reason)
{
  SC3E_TEST (a != NULL, reason);
  SC3E_IS (sc3_refcount_is_valid, &a->rc, reason);
  SC3E_TEST (!a->alloced == (a->oa == NULL), reason);
  if (a->oa != NULL) {
    /* this goes into a recursion up the allocator tree */
    SC3E_IS (sc3_allocator_is_setup, a->oa, reason);
  }
  if (!a->setup) {
    SC3E_TEST (a->num_malloc == 0 && a->num_calloc == 0 && a->num_free == 0,
               reason);
    SC3E_TEST (a->total_size == 0, reason);
  }
  else {
    SC3E_TEST (a->num_malloc >= 0 && a->num_calloc >= 0 && a->num_free >= 0,
               reason);
    SC3E_TEST (a->num_malloc + a->num_calloc >= a->num_free, reason);
  }
  SC3E_YES (reason);
}

int
sc3_allocator_is_new (sc3_allocator_t * a, char *reason)
{
  SC3E_IS (sc3_allocator_is_valid, a, reason);
  SC3E_TEST (!a->setup, reason);
  SC3E_YES (reason);
}

int
sc3_allocator_is_setup (sc3_allocator_t * a, char *reason)
{
  SC3E_IS (sc3_allocator_is_valid, a, reason);
  SC3E_TEST (a->setup, reason);
  SC3E_YES (reason);
}

int
sc3_allocator_is_free (sc3_allocator_t * a, char *reason)
{
  SC3E_IS (sc3_allocator_is_setup, a, reason);
  SC3E_TEST (a->num_malloc + a->num_calloc == a->num_free, reason);
  SC3E_TEST (a->total_size == 0, reason);
  SC3E_YES (reason);
}

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
sc3_allocator_new (sc3_allocator_t * oa, sc3_allocator_t ** ap)
{
  sc3_allocator_t    *a;

  SC3E_RETVAL (ap, NULL);
  SC3A_IS (sc3_allocator_is_setup, oa);

  SC3E (sc3_allocator_ref (oa));
  SC3E_ALLOCATOR_CALLOC (oa, sc3_allocator_t, 1, a);
  SC3E (sc3_refcount_init (&a->rc));
  a->align = SC3_MAX (sizeof (void *), sizeof (sc3_alloc_item_t));
  a->alloced = 1;
  a->counting = 1;
  a->oa = oa;
  SC3A_IS (sc3_allocator_is_new, a);

  *ap = a;
  return NULL;
}

sc3_error_t        *
sc3_allocator_set_align (sc3_allocator_t * a, size_t align)
{
  SC3A_IS (sc3_allocator_is_new, a);
  SC3A_CHECK (align == 0 || SC3_ISPOWOF2 (align));

  a->align = align == 0 ? 0 : SC3_MAX (align, sizeof (sc3_alloc_item_t));
  return NULL;
}

sc3_error_t        *
sc3_allocator_setup (sc3_allocator_t * a)
{
  SC3A_IS (sc3_allocator_is_new, a);

  a->setup = 1;
  SC3A_IS (sc3_allocator_is_setup, a);
  return NULL;
}

sc3_error_t        *
sc3_allocator_ref (sc3_allocator_t * a)
{
  SC3A_IS (sc3_allocator_is_setup, a);
  if (a->alloced) {
    SC3E (sc3_refcount_ref (&a->rc));
  }
  return NULL;
}

sc3_error_t        *
sc3_allocator_unref (sc3_allocator_t ** ap)
{
  int                 waslast;
  sc3_allocator_t    *a, *oa;

  SC3E_INOUTP (ap, a);
  SC3A_IS (sc3_allocator_is_valid, a);

  if (!a->alloced) {
    return NULL;
  }

  SC3E (sc3_refcount_unref (&a->rc, &waslast));
  if (waslast) {
    *ap = NULL;

    if (a->counting) {
      SC3E_DEMAND (a->num_malloc + a->num_calloc == a->num_free,
                   "Memory allocation count");
      SC3E_DEMAND (a->total_size == 0, "Memory allocation size");
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
  sc3_allocator_t    *a;

  SC3E_INULLP (ap, a);
  SC3E_DEMIS (sc3_refcount_is_last, &a->rc);
  SC3E (sc3_allocator_unref (&a));

  SC3A_CHECK (a == NULL || !a->alloced);
  return NULL;
}

sc3_error_t        *
sc3_allocator_strdup (sc3_allocator_t * a, const char *src, char **dest)
{
  size_t              len;
  void               *p;

  SC3E_RETVAL (dest, NULL);
  SC3A_CHECK (src != NULL);

  len = strlen (src) + 1;
  SC3E (sc3_allocator_malloc (a, len, &p));
  memcpy (p, src, len);

  *dest = (char *) p;
  return NULL;
}

void               *
sc3_allocator_malloc_noerr (sc3_allocator_t * a, size_t size)
{
  char               *p;

  if (!sc3_allocator_is_setup (a, NULL) || a->align != 0) {
    return NULL;
  }

  p = SC3_MALLOC (char, size);
  if (size > 0 && p == NULL) {
    return NULL;
  }

  if (a->counting) {
    ++a->num_malloc;
  }

  return p;
}

static sc3_error_t *
sc3_allocator_alloc_aligned (sc3_allocator_t * a, size_t size, int initzero,
                             void **ptr)
{
  const size_t        hsize = 3 * sizeof (sc3_alloc_item_t);
  size_t              actual, shift;
  char               *p;
  sc3_alloc_item_t   *aitem;

  SC3A_IS (sc3_allocator_is_setup, a);
  SC3A_CHECK (ptr != NULL && *ptr == NULL);

  /* allocate bigger block and write debug and size info into header */
  actual = a->align + hsize + size;

  /* allocate memory big enough for shift and meta information */
  p = initzero ? SC3_CALLOC (char, actual) : SC3_MALLOC (char, actual);
  SC3E_DEMAND (p != NULL, "Allocation");

  /* record allocator's address, original pointer, and allocated size */
  shift = a->align - ((size_t) p + hsize) % a->align;
  SC3A_CHECK (0 < shift && shift <= a->align);
  memset (p, -1, shift);
  aitem = (sc3_alloc_item_t *) (p + shift);
  aitem[0].ptr = a;
  aitem[1].ptr = p;
  aitem[2].siz = size;
  p = (char *) &aitem[3];
  SC3A_CHECK (((size_t) p) % a->align == 0);

  /* keep track of total allocated size */
  if (a->counting) {
    if (initzero) {
      ++a->num_calloc;
    }
    else {
      ++a->num_malloc;
    }
    a->total_size += size;
  }

  /* return new memory */
  *ptr = p;
  return NULL;
}

sc3_error_t        *
sc3_allocator_malloc (sc3_allocator_t * a, size_t size, void **ptr)
{
  SC3E_RETVAL (ptr, NULL);
  SC3A_IS (sc3_allocator_is_setup, a);

  if (a->align == 0) {
    /* use system allocation */
    char               *p = SC3_MALLOC (char, size);
    SC3E_DEMAND (size == 0 || p != NULL, "Allocation by malloc");

    /* when allocating zero bytes we may obtain a NULL pointer */
    if (a->counting && p != NULL) {
      ++a->num_malloc;
    }
    *ptr = p;
  }
  else {
    SC3E (sc3_allocator_alloc_aligned (a, size, 0, ptr));
  }
  return NULL;
}

sc3_error_t        *
sc3_allocator_calloc (sc3_allocator_t * a, size_t nmemb, size_t size,
                      void **ptr)
{
  SC3E_RETVAL (ptr, NULL);
  SC3A_IS (sc3_allocator_is_setup, a);

  size *= nmemb;
  if (a->align == 0) {
    /* use system allocation */
    char               *p = SC3_CALLOC (char, size);
    SC3E_DEMAND (size == 0 || p != NULL, "Allocation by calloc");

    /* when allocating zero bytes we may obtain a NULL pointer */
    if (a->counting && p != NULL) {
      ++a->num_calloc;
    }
    *ptr = p;
  }
  else {
    SC3E (sc3_allocator_alloc_aligned (a, size, 1, ptr));
  }
  return NULL;
}

sc3_error_t        *
sc3_allocator_free (sc3_allocator_t * a, void *p)
{
  SC3A_IS (sc3_allocator_is_setup, a);

  /* It is legal to pass NULL values to free.  This is not counted. */
  if (p == NULL) {
    return NULL;
  }
  if (a->counting) {
    ++a->num_free;
  }

  if (a->align == 0) {
    /* use system allocation */
    SC3_FREE (p);
  }
  else {
    size_t              size;
    sc3_alloc_item_t   *aitem;

    /* verify that memory had been allocated by this allocator */
    aitem = ((sc3_alloc_item_t *) p) - 3;
    SC3A_CHECK (aitem[0].ptr == a);

    /* keep track of total allocated size */
    if (a->counting) {
      size = aitem[2].siz;
      SC3A_CHECK (size <= a->total_size);
      a->total_size -= size;
    }
    SC3_FREE (aitem[1].ptr);
  }
  return NULL;
}

sc3_error_t        *
sc3_allocator_realloc (sc3_allocator_t * a, size_t new_size, void **ptr)
{
  void               *p;

  SC3E_ONULLP (ptr, p);
  SC3A_IS (sc3_allocator_is_setup, a);

  if (p == NULL) {
    SC3E (sc3_allocator_malloc (a, new_size, ptr));
  }
  else if (new_size == 0) {
    SC3E (sc3_allocator_free (a, p));
  }
  else {
    if (a->align == 0) {
      *ptr = SC3_REALLOC (p, char, new_size);
      SC3E_DEMAND (*ptr != NULL, "Reallocation");
    }
    else {
      size_t              size;
      sc3_alloc_item_t   *aitem;

      /* verify that memory had been allocated by this allocator */
      aitem = ((sc3_alloc_item_t *) p) - 3;
      SC3A_CHECK (aitem[0].ptr == a);

      /* retrieve previous size of allocation */
      size = aitem[2].siz;
      if (size != new_size) {
        /* we have to manually allocate, copy, and free due to alignment */
        SC3E (sc3_allocator_malloc (a, new_size, ptr));
        memcpy (*ptr, p, SC3_MIN (size, new_size));
        SC3E (sc3_allocator_free (a, p));
      }
    }
  }
  return NULL;
}
