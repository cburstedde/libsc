/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System

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

#include <sc_containers.h>

static              ssize_t
sc_array_bsearch_range (sc_array_t * array, size_t begin, size_t end,
                        const void *key, int (*compar) (const void *,
                                                        const void *))
{
  ssize_t             result;
  sc_array_t         *view;

  view = sc_array_new_view (array, begin, end - begin);
  result = sc_array_bsearch (view, key, compar);
  sc_array_destroy (view);

  return result < 0 ? result : (ssize_t) begin + result;
}

static void
test_new_size (sc_array_t * a)
{
  const size_t        S = a->elem_size;
  const size_t        N = a->elem_count;
  sc_array_t         *v, *w;

  v = sc_array_new_size (S, N);
  SC_CHECK_ABORT (v->elem_size == S && S == sizeof (int), "Size mismatch");
  SC_CHECK_ABORT (v->elem_count == N && N > 0, "Count mismatch");
  SC_CHECK_ABORT (v->byte_alloc <= a->byte_alloc, "Alloc mismatch");

  w = sc_array_new (S);
  sc_array_copy (w, v);
  SC_CHECK_ABORT (sc_array_is_equal (v, w), "Array mismatch");
  sc_array_destroy (w);

  memcpy (v->array, a->array, N * S);
  SC_CHECK_ABORT (sc_array_is_sorted (v, sc_int_compare), "Sort failed");
  sc_array_destroy (v);
}

static void
test_new_view (sc_array_t * a)
{
  const size_t        N = a->elem_count;
  sc_array_t         *v;

  v = sc_array_new_view (a, 0, N);
  SC_CHECK_ABORT (sc_array_is_sorted (v, sc_int_compare), "Sort failed");
  sc_array_destroy (v);

  v = sc_array_new_view (a, N / 2, N / 2);
  SC_CHECK_ABORT (sc_array_is_sorted (v, sc_int_compare), "Sort failed");
  sc_array_destroy (v);

  v = sc_array_new_view (a, N / 5, 3 * N / 4);
  SC_CHECK_ABORT (sc_array_is_sorted (v, sc_int_compare), "Sort failed");
  sc_array_destroy (v);
}

static void
test_new_data (sc_array_t * a)
{
  const size_t        s = a->elem_size;
  const size_t        N = a->elem_count;
  sc_array_t         *v;

  v = sc_array_new_data (a->array, s, N);
  SC_CHECK_ABORT (sc_array_is_sorted (v, sc_int_compare), "Sort failed");
  sc_array_destroy (v);

  v = sc_array_new_data (a->array + s * (N / 2), s, N / 2);
  SC_CHECK_ABORT (sc_array_is_sorted (v, sc_int_compare), "Sort failed");
  sc_array_destroy (v);

  v = sc_array_new_data (a->array + s * (N / 5), s, 3 * N / 4);
  SC_CHECK_ABORT (sc_array_is_sorted (v, sc_int_compare), "Sort failed");
  sc_array_destroy (v);
}

int
main (int argc, char **argv)
{
  const int           N = 29;
  int                 i, j, s, c;
  int                *pe;
  size_t              b1, b2;
  ssize_t             result, r1, r2, r3, t;
  sc_array_t         *a, *p;
  size_t             *perm;
  int                *data;

  sc_init (sc_MPI_COMM_NULL, 1, 1, NULL, SC_LP_DEFAULT);

  a = sc_array_new (sizeof (int));
  sc_array_resize (a, (size_t) N);

  SC_GLOBAL_INFOF ("Sizeof size_t %lld long %lld sc_array_t %lld\n",
                   (long long) sizeof (size_t),
                   (long long) sizeof (long),
                   (long long) sizeof (sc_array_t));
  SC_GLOBAL_INFOF ("Array byte size %lld\n", (long long)
                   sc_array_memory_used (a, 1));

  for (i = 0; i < N; ++i) {
    pe = (int *) sc_array_index_int (a, i);
    SC_CHECK_ABORT (sc_array_position (a, pe) == (size_t) i,
                    "Position failed");

    *pe = (i + N / 2) * (N - i);        /* can create duplicates */
  }
  sc_array_sort (a, sc_int_compare);
  SC_CHECK_ABORT (sc_array_is_sorted (a, sc_int_compare), "Sort failed");
  for (i = 0; i < N; ++i) {
    s = (i + N / 2) * (N - i);
    result = sc_array_bsearch (a, &s, sc_int_compare);
    SC_CHECK_ABORT (0 <= result && result < (ssize_t) N, "Result failed");
  }

  test_new_size (a);
  test_new_view (a);
  test_new_data (a);

  for (i = 0; i < N; ++i) {
    pe = (int *) sc_array_index_int (a, i);
    *pe = 1 + i + i * i;        /* is already sorted */
  }
  for (i = 0; i < N; ++i) {
    s = 1 + i + i * i;

    b1 = b2 = (size_t) i;
    result = sc_array_bsearch_range (a, b1, b2, &s, sc_int_compare);
    SC_CHECK_ABORT (result == -1, "Empty range failed");

    b1 = (size_t) (N / 2);
    b2 = (size_t) (3 * N / 4);
    r1 = sc_array_bsearch_range (a, 0, b1, &s, sc_int_compare);
    r2 = sc_array_bsearch_range (a, b1, b2, &s, sc_int_compare);
    r3 = sc_array_bsearch_range (a, b2, (size_t) N, &s, sc_int_compare);

    c = 0;
    t = -1;
    if (r1 < 0)
      ++c;
    else
      t = r1;
    if (r2 < 0)
      ++c;
    else
      t = r2;
    if (r3 < 0)
      ++c;
    else
      t = r3;
    SC_CHECK_ABORT (c == 2 && t == (ssize_t) i, "Combined ranges failed");
  }

  sc_array_destroy (a);

  /* test permute in place */

  data = SC_ALLOC (int, 2 * N);

  memset (data, -1, 2 * N * sizeof (int));

  /* create permuted pairs */

  data[0] = 0;
  data[1] = 1;
  for (i = 1; i < N; i++) {
    j = (int) (rand () % (i + 1));
    data[2 * i] = data[2 * j];
    data[2 * i + 1] = data[2 * j + 1];
    data[2 * j] = 2 * i;
    data[2 * j + 1] = 2 * i + 1;
  }

  for (i = 0; i < N; i++) {
    SC_ASSERT (data[2 * i] >= 0);
    SC_ASSERT (data[2 * i + 1] >= 0);
    SC_ASSERT (data[2 * i + 1] - data[2 * i] == 1);
  }

  a = sc_array_new_data (data, 2 * sizeof (int), (size_t) N);

  perm = SC_ALLOC (size_t, N);

  /* perm everything back to its proper place */

  for (i = 0; i < N; i++) {
    perm[i] = (size_t) data[2 * i] / 2;
  }

  p = sc_array_new_data (perm, sizeof (size_t), (size_t) N);
  sc_array_permute (a, p, 0);

  for (i = 0; i < N; i++) {
    SC_CHECK_ABORT (data[2 * i] == 2 * i, "Permutation failure");
    SC_CHECK_ABORT (data[2 * i + 1] == 2 * i + 1, "Permutation failure");
    SC_CHECK_ABORT ((int) perm[i] == i, "Permutation failure");
  }

  sc_array_destroy (p);
  sc_array_destroy (a);
  SC_FREE (perm);
  SC_FREE (data);

  sc_finalize ();

  return 0;
}
