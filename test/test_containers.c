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

#include <sc_random.h>
#include <sc3_array.h>
#include <sc3_log.h>
#include <sc3_memstamp.h>

/** If a condition \a x is not met, return an assertion error.
 * Its message is set to the failed condition and argument \a s.
 *
 * Do we like this macro?  Shall we include it in sc3_error.h?
 */
#define SC3E_CONDITION(x,s) do {                                        \
  if (!(x)) {                                                           \
    char _errmsg[SC3_BUFSIZE];                                          \
    sc3_snprintf (_errmsg, SC3_BUFSIZE, "%s: %s", s, #x);               \
    return sc3_error_new_assert (__FILE__, __LINE__, _errmsg);          \
  }} while (0)

static sc3_error_t *
test_allocations (void)
{
  const size_t        munit[3] = { 0, 17, 5138 };
  const size_t        msize[3] = { 20, 37, 537 };
  int                 i, j;
  size_t              isize;
  char               *pc, **backup;
  sc3_mstamp_t       *mst;
  sc3_allocator_t    *alloc;
  sc3_array_t        *items;

  SC3E (sc3_allocator_new (NULL, &alloc));
  SC3E (sc3_allocator_set_align (alloc, 32));
  SC3E (sc3_allocator_setup (alloc));

  SC3E (sc3_array_new (alloc, &items));
  SC3E (sc3_array_set_elem_size (items, sizeof (char *)));
  SC3E (sc3_array_setup (items));

  for (j = 0; j < 3; ++j) {

    /* zero data size */
    SC3E (sc3_mstamp_new (alloc, &mst));
    SC3E (sc3_mstamp_set_stamp_size (mst, munit[j]));
    SC3E (sc3_mstamp_set_elem_size (mst, 0));
    SC3E (sc3_mstamp_setup (mst));
    for (i = 0; i < 8 + 3 * j; ++i) {
      SC3E (sc3_mstamp_alloc (mst, &pc));
      SC3E_CONDITION (pc == NULL, "Mstamp alloc NULL");
    }
    SC3E (sc3_mstamp_destroy (&mst));

    /* proper data size */
    isize = msize[j];
    SC3E (sc3_mstamp_new (alloc, &mst));
    SC3E (sc3_mstamp_set_stamp_size (mst, munit[j]));
    SC3E (sc3_mstamp_set_elem_size (mst, isize));
    SC3E (sc3_mstamp_setup (mst));
    for (i = 0; i < 7829; ++i) {
      SC3E (sc3_mstamp_alloc (mst, &pc));
      memset (pc, -1, isize);
    }
    SC3E (sc3_mstamp_destroy (&mst));

    /* allocate, free, and allocate again */
    SC3E (sc3_mstamp_new (alloc, &mst));
    SC3E (sc3_mstamp_set_stamp_size (mst, munit[j]));
    SC3E (sc3_mstamp_set_elem_size (mst, isize));
    SC3E (sc3_mstamp_setup (mst));
    for (i = 0; i < 3124; ++i) {
      SC3E (sc3_mstamp_alloc (mst, &pc));
      memset (pc, -1, isize);
      SC3E (sc3_array_push (items, &backup));
      *backup = pc;
    }
    for (i = 0; i < 3124; ++i) {
      SC3E (sc3_array_index (items, i, &backup));
      pc = *backup;
      SC3E (sc3_mstamp_free (mst, &pc));
    }
    for (i = 0; i < 3124; ++i) {
      SC3E (sc3_mstamp_alloc (mst, &pc));
      memset (pc, -1, isize);
    }
    SC3E (sc3_array_resize (items, 0));
    SC3E (sc3_mstamp_destroy (&mst));
  }

  SC3E (sc3_array_destroy (&items));
  SC3E (sc3_allocator_destroy (&alloc));

  return NULL;
}

static sc3_error_t *
test_correctness (void)
{
  const int           nelems = 7829;
  const int           per_stamp = 3;
  int                 i, ecount;
  size_t              isize;
  void               *pc;
  sc3_mstamp_t       *mst;
  long               *tv;

  isize = sizeof (long);

  SC3E (sc3_mstamp_new (NULL, &mst));
  SC3E (sc3_mstamp_set_stamp_size (mst, per_stamp * isize + 1));
  SC3E (sc3_mstamp_set_elem_size (mst, isize));
  SC3E (sc3_mstamp_set_initzero (mst, 1));
  SC3E (sc3_mstamp_setup (mst));
  for (i = 0; i < nelems; ++i) {
    SC3E (sc3_mstamp_alloc (mst, &pc));
    tv = (long *) pc;
    SC3E_CONDITION (*tv == 0L, "initzero doesn't work");
    *tv = 42L;
    SC3E_CONDITION (*(long *) pc == 42L, "wrong stamp access");
  }
  SC3E (sc3_mstamp_get_elem_count (mst, &ecount));
  SC3E_CONDITION (ecount == nelems, "wrong number of elements");

#if 0
  /* pc needs to be a separate valid item in each iteration */
  for (i = 0; i < nelems - shift; ++i) {
    SC3E (sc3_mstamp_free (mst, &pc));
  }
  SC3E (sc3_mstamp_get_elem_count (mst, &ecount));
  SC3E_CONDITION (ecount == shift, "wrong number of elements after freeing");
#endif

  for (i = 0; i < nelems; ++i) {
    SC3E (sc3_mstamp_alloc (mst, &pc));
    tv = (long *) pc;
    SC3E_CONDITION (*tv == 0L, "initzero doesn't work after freeing");
    *tv = 42L;
    SC3E_CONDITION (*(long *) pc == 42L, "wrong stamp access after freeing");
  }
  SC3E (sc3_mstamp_get_elem_count (mst, &ecount));
  SC3E_CONDITION (ecount == 2 * nelems, "wrong number of elements");
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
  SC3E (sc3_allocator_new (NULL, &alloc));
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
    SC3E_CONDITION (*(int *) ptr == *(int *) ptr_view && ptr == ptr_view,
                    "the view points to the wrong memory");
  }
  SC3E (sc3_array_renew_view (view, a, offset / 2, length / 2));
  for (i = 0; i < length / 2; ++i) {
    SC3E (sc3_array_index (a, i + offset / 2, &ptr));
    SC3E (sc3_array_index (view, i, &ptr_view));
    SC3E_CONDITION (*(int *) ptr == *(int *) ptr_view && ptr == ptr_view,
                    "the view points to the wrong memory");
  }

  SC3E (sc3_array_destroy (&view));
  SC3E (sc3_array_destroy (&a));

  /*make a new view of data */
  SC3E (sc3_array_new_data (alloc, &view, data, isize, offset, length));
  for (i = 0; i < length; ++i) {
    SC3E (sc3_array_index (view, i, &ptr_view));
    SC3E_CONDITION (data[i + offset] == *(int *) ptr_view,
                    "the view points to the wrong memory");
  }
  SC3E (sc3_array_renew_data (view, data, isize, offset / 2, length / 2));
  for (i = 0; i < length / 2; ++i) {
    SC3E (sc3_array_index (view, i, &ptr_view));
    SC3E_CONDITION (data[i + offset / 2] == *(int *) ptr_view,
                    "the view points to the wrong memory");
  }
  /* renew view with a NULL data */
  SC3E (sc3_array_renew_data (view, NULL, isize, offset, 0));

  /*destroy the view and free the data */
  SC3E (sc3_allocator_free (alloc, &data));
  SC3E (sc3_array_destroy (&view));
  SC3E (sc3_allocator_destroy (&alloc));
  return NULL;
}

static sc3_error_t *
test_all (void)
{
  SC3E (test_allocations ());
  SC3E (test_correctness ());
  SC3E (test_view ());
  return NULL;
}

int
main (int argc, char **argv)
{
  sc3_MPI_Comm_t      mpicomm = SC3_MPI_COMM_WORLD;
  int                 mpirank;

  SC3X (sc3_MPI_Init (&argc, &argv));
  SC3X (sc3_MPI_Comm_rank (mpicomm, &mpirank));
  if (mpirank == 0) {
    SC3X (test_all ());
  }
  SC3X (sc3_MPI_Finalize ());
  return EXIT_SUCCESS;
}
