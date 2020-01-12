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
  int                 resizable;
  size_t              esize, ecount, ealloc;

  /* member variables initialized in setup call */

};

int
sc3_array_is_valid (sc3_array_t * a)
{
  if (a == NULL || !sc3_refcount_is_valid (&a->rc)) {
    return 0;
  }
  if (!sc3_allocator_is_valid (a->aator)) {
    return 0;
  }

  /* TODO check internal allocation logic depending on setup status */

  return 1;
}

int
sc3_array_is_new (sc3_array_t * a)
{
  return sc3_array_is_valid (a) && !a->setup;
}

int
sc3_array_is_setup (sc3_array_t * a)
{
  return sc3_array_is_valid (a) && a->setup;
}

sc3_error_t        *
sc3_array_new (sc3_allocator_t * aator, sc3_array_t ** ap)
{
  sc3_array_t        *a;

  SC3E_RETVAL (ap, NULL);
  SC3A_CHECK (sc3_allocator_is_valid (aator));

  SC3E (sc3_allocator_ref (aator));
  SC3E_ALLOCATOR_CALLOC (aator, sc3_array_t, 1, a);
  SC3E (sc3_refcount_init (&a->rc));
  a->esize = 1;
  a->ealloc = 8;
  a->resizable = 1;
  a->aator = aator;
  SC3A_CHECK (sc3_array_is_new (a));

  *ap = a;
  return NULL;
}

sc3_error_t        *
sc3_array_set_elem_size (sc3_array_t * a, size_t esize)
{
  SC3A_CHECK (sc3_array_is_new (a));
  a->esize = esize;
  return NULL;
}

sc3_error_t        *
sc3_array_set_elem_count (sc3_array_t * a, size_t ecount)
{
  SC3A_CHECK (sc3_array_is_new (a));
  a->ecount = ecount;
  return NULL;
}

sc3_error_t        *
sc3_array_set_elem_alloc (sc3_array_t * a, size_t ealloc)
{
  SC3A_CHECK (sc3_array_is_new (a));
  a->ealloc = ealloc;
  return NULL;
}

sc3_error_t        *
sc3_array_set_resizable (sc3_array_t * a, int resizable)
{
  SC3A_CHECK (sc3_array_is_new (a));
  a->resizable = resizable;
  return NULL;
}

sc3_error_t        *
sc3_array_setup (sc3_array_t * a)
{
  SC3A_CHECK (sc3_array_is_new (a));

  /* TODO allocate array storage */

  a->setup = 1;
  SC3A_CHECK (sc3_array_is_setup (a));
  return NULL;
}

sc3_error_t        *
sc3_array_ref (sc3_array_t * a)
{
  SC3A_CHECK (sc3_array_is_setup (a));
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
  SC3A_CHECK (sc3_array_is_valid (a));
  SC3E (sc3_refcount_unref (&a->rc, &waslast));
  if (waslast) {
    *ap = NULL;

    /* TODO deallocate array storage if setup */

    aator = a->aator;
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
  SC3E_DEMAND (sc3_refcount_is_last (&a->rc));
  SC3E (sc3_array_unref (&a));

  SC3A_CHECK (a == NULL);
  return NULL;
}
