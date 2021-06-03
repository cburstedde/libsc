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

#include <sc.h>
#include <sc3_memstamp.h>
#include <sc3_array.h>
#include <sc_random.h>

static sc3_error_t *
test_allocations (void)
{
  const size_t        munit[3] = { 0, 17, 5138 };
  const size_t        msize[3] = { 20, 37, 537 };
  int                 i, j;
  size_t              isize;
  void               *pc;
  sc3_mstamp_t       *mst;

  for (j = 0; j < 3; ++j) {
    /* zero data size */
    SC3E (sc3_mstamp_new (sc3_allocator_nocount (), &mst));
    SC3E (sc3_mstamp_set_stamp_size (mst, munit[j]));
    SC3E (sc3_mstamp_set_elem_size (mst, 0));
    SC3E (sc3_mstamp_setup (mst));
    for (i = 0; i < 8 + 3 * j; ++i) {
      SC3E (sc3_mstamp_alloc (mst, &pc));
      SC_CHECK_ABORT (pc == NULL, "Mstamp alloc NULL");
    }
    SC3E (sc3_mstamp_destroy (&mst));
    /* proper data size */
    isize = msize[j];
    SC3E (sc3_mstamp_new (sc3_allocator_nocount (), &mst));
    SC3E (sc3_mstamp_set_stamp_size (mst, munit[j]));
    SC3E (sc3_mstamp_set_elem_size (mst, isize));
    SC3E (sc3_mstamp_setup (mst));
    for (i = 0; i < 7829; ++i) {
      SC3E (sc3_mstamp_alloc (mst, &pc));
      memset (pc, -1, isize);
    }
    SC3E (sc3_mstamp_destroy (&mst));

    SC3E (sc3_mstamp_new (sc3_allocator_nocount (), &mst));
    SC3E (sc3_mstamp_set_stamp_size (mst, munit[j]));
    SC3E (sc3_mstamp_set_elem_size (mst, isize));
    SC3E (sc3_mstamp_setup (mst));
    for (i = 0; i < 3124; ++i) {
      SC3E (sc3_mstamp_alloc (mst, &pc));
      memset (pc, -1, isize);
    }
    for (i = 0; i < 3124; ++i) {
      SC3E (sc3_mstamp_free (mst, pc));
    }
    for (i = 0; i < 3124; ++i) {
      SC3E (sc3_mstamp_alloc (mst, &pc));
      memset (pc, -1, isize);
    }
    SC3E (sc3_mstamp_destroy (&mst));
  }
  return NULL;
}

static sc3_error_t *
test_correctness (void)
{
  const int           nelems = 7829;
  const int           shift = 1;
  const int           per_stamp = 3;
  int                 i, ecount;
  size_t              isize;
  void               *pc;
  sc3_mstamp_t       *mst;
  long               *tv;

  isize = sizeof (long);

  SC3E (sc3_mstamp_new (sc3_allocator_nocount (), &mst));
  SC3E (sc3_mstamp_set_stamp_size (mst, per_stamp * isize + 1));
  SC3E (sc3_mstamp_set_elem_size (mst, isize));
  SC3E (sc3_mstamp_set_initzero (mst, 1));
  SC3E (sc3_mstamp_setup (mst));
  for (i = 0; i < nelems; ++i) {
    SC3E (sc3_mstamp_alloc (mst, &pc));
    tv = (long *) pc;
    SC3E_DEMAND (*tv == 0L, "initzero doesn't work");
    *tv = 42L;
    SC3E_DEMAND (*(long *) pc == 42L, "wrong stamp access");
  }
  SC3E (sc3_mstamp_get_elem_count (mst, &ecount));
  SC3E_DEMAND (ecount == nelems, "wrong number of elements");

  for (i = 0; i < nelems - shift; ++i) {
    SC3E (sc3_mstamp_free (mst, pc));
    *(long *) pc = 21L;
  }
  SC3E (sc3_mstamp_get_elem_count (mst, &ecount));
  SC3E_DEMAND (ecount == shift, "wrong number of elements after freeing");

  for (i = 0; i < nelems; ++i) {
    SC3E (sc3_mstamp_alloc (mst, &pc));
    tv = (long *) pc;
    SC3E_DEMAND (*tv == 0L, "initzero doesn't work after freeing");
    *tv = 42L;
    SC3E_DEMAND (*(long *) pc == 42L, "wrong stamp access after freeing");
  }
  SC3E (sc3_mstamp_get_elem_count (mst, &ecount));
  SC3E_DEMAND (ecount == nelems + shift, "wrong number of elements");
  SC3E (sc3_mstamp_destroy (&mst));

  return NULL;
}

static sc3_error_t *
test_view (void)
{
  const int           nelems = 7829;
  const size_t        isize = sizeof (int);
  const int           offset = nelems / 3 - 1;
  const int           length = 2 * offset;
  int                 i;
  sc3_array_t        *a, *view;
  int                *iptr, *data;
  void               *ptr, *ptr_view;
  sc_rand_state_t     rs = 203, *prs = &rs;
  sc3_allocator_t    *alloc;

  /* create a toplevel allocator */
  SC3E (sc3_allocator_new (sc3_allocator_nocount (), &alloc));
  SC3E (sc3_allocator_setup (alloc));

  /*create and fill a simple sc3_array and c-array */
  SC3E (sc3_array_new (alloc, &a));
  SC3E (sc3_array_set_elem_size (a, isize));
  SC3E (sc3_array_set_elem_count (a, nelems));
  SC3E (sc3_array_set_resizable (a, 0));
  SC3E (sc3_array_setup (a));
  SC3E (sc3_allocator_malloc (alloc, isize * nelems, &data));

  for (i = 0; i < nelems; ++i) {
    SC3E (sc3_array_index (a, i, &iptr));
    *iptr = sc_rand_poisson (prs, INT_MAX * 0.5);
    data[i] = *iptr;
  }
  SC3E (sc3_array_new_view (alloc, &view, a, offset, length));

  for (i = 0; i < length; ++i) {
    SC3E (sc3_array_index (a, i + offset, &ptr));
    SC3E (sc3_array_index (view, i, &ptr_view));
    SC3E_DEMAND (*(int *) ptr == *(int *) ptr_view && ptr == ptr_view,
                 "the view points to the wrong memory");
  }
  SC3E (sc3_array_renew_view (&view, a, offset / 2, length / 2));
  for (i = 0; i < length / 2; ++i) {
    SC3E (sc3_array_index (a, i + offset / 2, &ptr));
    SC3E (sc3_array_index (view, i, &ptr_view));
    SC3E_DEMAND (*(int *) ptr == *(int *) ptr_view && ptr == ptr_view,
                 "the view points to the wrong memory");
  }

  SC3E (sc3_array_destroy (&view));
  SC3E (sc3_array_destroy (&a));

  /*make a new view of data */
  SC3E (sc3_array_new_data (alloc, &view, data, isize, offset, length));
  for (i = 0; i < length; ++i) {
    SC3E (sc3_array_index (view, i, &ptr_view));
    SC3E_DEMAND (data[i + offset] == *(int *) ptr_view,
                 "the view points to the wrong memory");
  }
  SC3E (sc3_array_renew_data (&view, data, isize, offset / 2, length / 2));
  for (i = 0; i < length / 2; ++i) {
    SC3E (sc3_array_index (view, i, &ptr_view));
    SC3E_DEMAND (data[i + offset / 2] == *(int *) ptr_view,
                 "the view points to the wrong memory");
  }
  /* renew view with a NULL data */
  SC3E (sc3_array_renew_data (&view, NULL, isize, offset, 0));

  /*destroy the view and free the data */
  SC3E (sc3_allocator_free (alloc, data));
  SC3E (sc3_array_destroy (&view));
  SC3E (sc3_allocator_destroy (&alloc));
  return NULL;
}

static sc3_error_t *
output_error (sc3_error_t ** pe)
{
  int                 eline;
  int                 depth;
  const char         *efile;
  const char         *emsg;
  sc3_error_kind_t    ekind;
  sc3_error_t        *e, *s;

  SC3E_DEMAND (pe != NULL && *pe != NULL, "Misuse of output_error");
  depth = 0;
  e = *pe;
  *pe = NULL;
  while (e != NULL) {
    SC3E (sc3_error_access_location (e, &efile, &eline));
    SC3E (sc3_error_access_message (e, &emsg));
    SC3E (sc3_error_get_kind (e, &ekind));

    fprintf (stderr, "Error %d %s:%d %c: %s\n",
             depth, efile, eline, sc3_error_kind_char[ekind], emsg);

    SC3E (sc3_error_restore_location (e, efile, eline));
    SC3E (sc3_error_restore_message (e, emsg));
    SC3E (sc3_error_ref_stack (e, &s));
    SC3E (sc3_error_destroy (&e));
    e = s;
    ++depth;
  }
  return NULL;
}

static void
report_error (sc3_error_t ** pe)
{
  sc3_error_t        *e;

  if (pe != NULL && *pe != NULL) {
    e = output_error (pe);
    if (e != NULL || pe == NULL || *pe != NULL) {
      /* This error should never occur.  Something is wrong on the inside. */
      SC_ABORT ("Internal error inconsistency\n");
    }
    /* We reached some error before this function that has now been printed. */
    SC_ABORT ("sc3 container tests failed\n");
  }
}

int
main (int argc, char **argv)
{
  sc3_error_t        *e;
  e = test_allocations ();
  report_error (&e);
  e = test_correctness ();
  report_error (&e);
  e = test_view ();
  report_error (&e);
  return 0;
}
