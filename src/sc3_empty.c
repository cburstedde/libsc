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
  sc3_allocator_t    *alloc;
  int                 setup;

  /* parameters set before and fixed after setup */
  int                 dummy;

  /* member variables initialized during setup */
  int                *member;
};

int
sc3_empty_is_valid (const sc3_empty_t * yy, char *reason)
{
  SC3E_TEST (yy != NULL, reason);
  SC3E_IS (sc3_refcount_is_valid, &yy->rc, reason);
  SC3E_IS (sc3_allocator_is_setup, yy->alloc, reason);

  if (!yy->setup) {
    SC3E_TEST (yy->member == NULL, reason);
  }
  else {
    SC3E_TEST (yy->member != NULL && *yy->member == yy->dummy, reason);
  }
  SC3E_YES (reason);
}

int
sc3_empty_is_new (const sc3_empty_t * yy, char *reason)
{
  SC3E_IS (sc3_empty_is_valid, yy, reason);
  SC3E_TEST (!yy->setup, reason);
  SC3E_YES (reason);
}

int
sc3_empty_is_setup (const sc3_empty_t * yy, char *reason)
{
  SC3E_IS (sc3_empty_is_valid, yy, reason);
  SC3E_TEST (yy->setup, reason);
  SC3E_YES (reason);
}

int
sc3_empty_is_dummy (const sc3_empty_t * yy, char *reason)
{
  SC3E_IS (sc3_empty_is_setup, yy, reason);
  SC3E_TEST (yy->dummy, reason);
  SC3E_YES (reason);
}

sc3_error_t        *
sc3_empty_new (sc3_allocator_t * alloc, sc3_empty_t ** yyp)
{
  sc3_empty_t        *yy;

  SC3E_RETVAL (yyp, NULL);

  if (alloc == NULL) {
    alloc = sc3_allocator_new_static ();
  }
  SC3A_IS (sc3_allocator_is_setup, alloc);

  SC3E (sc3_allocator_ref (alloc));
  SC3E (sc3_allocator_calloc (alloc, 1, sizeof (sc3_empty_t), &yy));
  SC3E (sc3_refcount_init (&yy->rc));
  yy->alloc = alloc;
  SC3A_IS (sc3_empty_is_new, yy);

  *yyp = yy;
  return NULL;
}

sc3_error_t        *
sc3_empty_set_dummy (sc3_empty_t * yy, int dummy)
{
  SC3A_IS (sc3_empty_is_new, yy);
  yy->dummy = dummy;
  return NULL;
}

sc3_error_t        *
sc3_empty_setup (sc3_empty_t * yy)
{
  SC3A_IS (sc3_empty_is_new, yy);

  /* allocate internal state */
  SC3E (sc3_allocator_malloc (yy->alloc, sizeof (int), &yy->member));
  *yy->member = yy->dummy;

  /* done with setup */
  yy->setup = 1;
  SC3A_IS (sc3_empty_is_setup, yy);
  return NULL;
}

sc3_error_t        *
sc3_empty_ref (sc3_empty_t * yy)
{
  SC3A_IS (sc3_empty_is_setup, yy);
  SC3E (sc3_refcount_ref (&yy->rc));
  return NULL;
}

sc3_error_t        *
sc3_empty_unref (sc3_empty_t ** yyp)
{
  int                 waslast;
  sc3_allocator_t    *alloc;
  sc3_empty_t        *yy;

  SC3E_INOUTP (yyp, yy);
  SC3A_IS (sc3_empty_is_valid, yy);
  SC3E (sc3_refcount_unref (&yy->rc, &waslast));
  if (waslast) {
    *yyp = NULL;

    alloc = yy->alloc;
    if (yy->setup) {
      /* deallocate internal state */
      SC3E (sc3_allocator_free (alloc, &yy->member));
    }
    SC3E (sc3_allocator_free (alloc, &yy));
    SC3E (sc3_allocator_unref (&alloc));
  }
  return NULL;
}

sc3_error_t        *
sc3_empty_destroy (sc3_empty_t ** yyp)
{
  sc3_empty_t        *yy;

  SC3E_INULLP (yyp, yy);
  SC3E_DEMIS (sc3_refcount_is_last, &yy->rc, SC3_ERROR_REF);
  SC3E (sc3_empty_unref (&yy));

  SC3A_CHECK (yy == NULL);
  return NULL;
}

sc3_error_t        *
sc3_empty_get_dummy (const sc3_empty_t * yy, int *dummy)
{
  SC3E_RETVAL (dummy, 0);
  SC3A_IS (sc3_empty_is_setup, yy);

  *dummy = yy->dummy;
  return NULL;
}
