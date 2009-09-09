/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2007-2009 Carsten Burstedde, Lucas Wilcox.

  The SC Library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with the SC Library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sc_containers.h>

/* #define THEBIGTEST */

static int
compar (const void *p1, const void *p2)
{
  int                 i1 = *(int *) p1;
  int                 i2 = *(int *) p2;

  return i1 - i2;
}

int
main (int argc, char **argv)
{
  int                 i, i1, i2, i3, i3last, i4, i4last, temp, count;
  size_t              s, swaps1, swaps2, swaps3, total1, total2, total3;
  ssize_t             searched;
  int                *pi;
  sc_array_t         *a1, *a2, *a3, *a4;
  int                 mpiret;
  double              start, elapsed_pqueue, elapsed_qsort;

  mpiret = MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

  a1 = sc_array_new (sizeof (int));
  a2 = sc_array_new (sizeof (int));
  a3 = sc_array_new (sizeof (int));
  a4 = sc_array_new (sizeof (int));

#ifdef THEBIGTEST
  count = 325323;
#else
  count = 3251;
#endif
  SC_INFOF ("Test pqueue with count %d\n", count);

  start = -MPI_Wtime ();

  swaps1 = swaps2 = swaps3 = 0;
  total1 = total2 = total3 = 0;
  for (i = 0; i < count; ++i) {
    *(int *) sc_array_push (a1) = i;
    s = sc_array_pqueue_add (a1, &temp, compar);
    swaps1 += ((s > 0) ? 1 : 0);
    total1 += s;

    *(int *) sc_array_push (a2) = count - i - 1;
    s = sc_array_pqueue_add (a2, &temp, compar);
    swaps2 += ((s > 0) ? 1 : 0);
    total2 += s;

    *(int *) sc_array_push (a3) = (15 * i) % 172;
    s = sc_array_pqueue_add (a3, &temp, compar);
    swaps3 += ((s > 0) ? 1 : 0);
    total3 += s;
  }
  SC_CHECK_ABORT (swaps1 == 0 && total1 == 0, "pqueue_add");
  SC_VERBOSEF ("   Swaps %lld %lld %lld Total %lld %lld %lld\n",
               (long long) swaps1, (long long) swaps2, (long long) swaps3,
               (long long) total1, (long long) total2, (long long) total3);

  temp = 52;
  searched = sc_array_bsearch (a1, &temp, compar);
  SC_CHECK_ABORT (searched != -1, "array_bsearch_index");
  pi = sc_array_index_ssize_t (a1, searched);
  SC_CHECK_ABORT (*pi == temp, "array_bsearch");

  i3last = -1;
  swaps1 = swaps2 = swaps3 = 0;
  total1 = total2 = total3 = 0;
  for (i = 0; i < count; ++i) {
    s = sc_array_pqueue_pop (a1, &i1, compar);
    swaps1 += ((s > 0) ? 1 : 0);
    total1 += s;

    s = sc_array_pqueue_pop (a2, &i2, compar);
    swaps2 += ((s > 0) ? 1 : 0);
    total2 += s;

    s = sc_array_pqueue_pop (a3, &i3, compar);
    swaps3 += ((s > 0) ? 1 : 0);
    total3 += s;

    SC_CHECK_ABORT (i == i1 && i == i2, "pqueue_pop");
    SC_CHECK_ABORT (i3 >= i3last, "pqueue_pop");
    i3last = i3;
  }
  SC_VERBOSEF ("   Swaps %lld %lld %lld Total %lld %lld %lld\n",
               (long long) swaps1, (long long) swaps2, (long long) swaps3,
               (long long) total1, (long long) total2, (long long) total3);

  elapsed_pqueue = start + MPI_Wtime ();

  sc_array_destroy (a1);
  sc_array_destroy (a2);
  sc_array_destroy (a3);

  SC_INFOF ("Test array sort with count %d\n", count);

  start = -MPI_Wtime ();

  /* the resize is done to be comparable with the above procedure */
  for (i = 0; i < count; ++i) {
    *(int *) sc_array_push (a4) = (15 * i) % 172;
  }
  sc_array_sort (a4, compar);

  i4last = -1;
  for (i = 0; i < count; ++i) {
    i4 = *(int *) sc_array_index_int (a4, i);

    SC_CHECK_ABORT (i4 >= i4last, "array_sort");
    i4last = i4;
  }
  sc_array_resize (a4, 0);

  elapsed_qsort = start + MPI_Wtime ();
  SC_STATISTICSF ("Test timings pqueue %g qsort %g\n",
                  elapsed_pqueue, 3. * elapsed_qsort);

  sc_array_destroy (a4);
  sc_finalize ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
