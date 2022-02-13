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

#include <sc3_empty.h>
#include <sc3_refcount.h>

struct sc3_empty
{
  /* internal metadata */
  sc3_refcount_t      rc;
  sc3_allocator_t    *yator;
  int                 setup;

  /* parameters set before and fixed after setup */
  int                 dummy;

  /* member variables initialized during setup */
  int                *member;
};

int
sc3_empty_is_valid (const sc3_empty_t * y, char *reason)
{
  SC3E_TEST (y != NULL, reason);
  SC3E_IS (sc3_refcount_is_valid, &y->rc, reason);
  SC3E_IS (sc3_allocator_is_setup, y->yator, reason);

  if (!y->setup) {
    SC3E_TEST (y->member == NULL, reason);
  }
  else {
    SC3E_TEST (y->member != NULL && *y->member == y->dummy, reason);
  }
  SC3E_YES (reason);
}

int
sc3_empty_is_new (const sc3_empty_t * y, char *reason)
{
  SC3E_IS (sc3_empty_is_valid, y, reason);
  SC3E_TEST (!y->setup, reason);
  SC3E_YES (reason);
}

int
sc3_empty_is_setup (const sc3_empty_t * y, char *reason)
{
  SC3E_IS (sc3_empty_is_valid, y, reason);
  SC3E_TEST (y->setup, reason);
  SC3E_YES (reason);
}

int
sc3_empty_is_dummy (const sc3_empty_t * y, char *reason)
{
  SC3E_IS (sc3_empty_is_setup, y, reason);
  SC3E_TEST (y->dummy, reason);
  SC3E_YES (reason);
}

sc3_error_t        *
sc3_empty_new (sc3_allocator_t * yator, sc3_empty_t ** yp)
{
  sc3_empty_t        *y;

  SC3E_RETVAL (yp, NULL);

  if (yator == NULL) {
    yator = sc3_allocator_new_static ();
  }
  SC3A_IS (sc3_allocator_is_setup, yator);

  SC3E (sc3_allocator_ref (yator));
  SC3E (sc3_allocator_calloc (yator, 1, sizeof (sc3_empty_t), &y));
  SC3E (sc3_refcount_init (&y->rc));
  y->yator = yator;
  SC3A_IS (sc3_empty_is_new, y);

  *yp = y;
  return NULL;
}

sc3_error_t        *
sc3_empty_set_dummy (sc3_empty_t * y, int dummy)
{
  SC3A_IS (sc3_empty_is_new, y);
  y->dummy = dummy;
  return NULL;
}

sc3_error_t        *
sc3_empty_setup (sc3_empty_t * y)
{
  SC3A_IS (sc3_empty_is_new, y);

  /* allocate internal state */
  SC3E (sc3_allocator_malloc (y->yator, sizeof (int), &y->member));
  *y->member = y->dummy;

  /* done with setup */
  y->setup = 1;
  SC3A_IS (sc3_empty_is_setup, y);
  return NULL;
}

sc3_error_t        *
sc3_empty_ref (sc3_empty_t * y)
{
  SC3A_IS (sc3_empty_is_setup, y);
  SC3E (sc3_refcount_ref (&y->rc));
  return NULL;
}

sc3_error_t        *
sc3_empty_unref (sc3_empty_t ** yp)
{
  int                 waslast;
  sc3_allocator_t    *yator;
  sc3_empty_t        *y;

  SC3E_INOUTP (yp, y);
  SC3A_IS (sc3_empty_is_valid, y);
  SC3E (sc3_refcount_unref (&y->rc, &waslast));
  if (waslast) {
    *yp = NULL;

    yator = y->yator;
    if (y->setup) {
      /* deallocate internal state */
      SC3E (sc3_allocator_free (yator, &y->member));
    }
    SC3E (sc3_allocator_free (yator, &y));
    SC3E (sc3_allocator_unref (&yator));
  }
  return NULL;
}

sc3_error_t        *
sc3_empty_destroy (sc3_empty_t ** yp)
{
  sc3_empty_t        *y;

  SC3E_INULLP (yp, y);
  SC3E_DEMIS (sc3_refcount_is_last, &y->rc, SC3_ERROR_REF);
  SC3E (sc3_empty_unref (&y));

  SC3A_CHECK (y == NULL);
  return NULL;
}

sc3_error_t        *
sc3_empty_get_dummy (const sc3_empty_t * y, int *dummy)
{
  SC3E_RETVAL (dummy, 0);
  SC3A_IS (sc3_empty_is_setup, y);

  *dummy = y->dummy;
  return NULL;
}
