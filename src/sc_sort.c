/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2008 Carsten Burstedde, Lucas Wilcox.

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

/* sc.h comes first in every compilation unit */
#include <sc.h>
#include <sc_sort.h>

typedef struct sc_psort
{
  MPI_Comm            mpicomm;
  int                 num_procs, rank;
  size_t              size;
  size_t              my_lo, my_hi, total;
  size_t             *gmemb;
  char               *my_base;
}
sc_psort_t;

/* qsort is not reentrant, so we do the inverse static */
static int          (*sc_compare) (const void *, const void *);
static int
sc_icompare (const void *v1, const void *v2)
{
  return sc_compare (v2, v1);
}

static void
sc_merge_bitonic (sc_psort_t * pst, size_t lo, size_t hi, bool dir)
{
  const size_t        n = hi - lo;

  if (n > 1 && pst->my_hi > lo && pst->my_lo < hi) {
    size_t              k, n2;
    size_t              lo_end, hi_beg;

    for (k = 1; k < n;) {
      k = k << 1;
    }
    n2 = k >> 1;
    SC_ASSERT (n2 >= n / 2 && n2 < n);

    lo_end = lo + n - n2;
    hi_beg = lo + n2;
    SC_ASSERT (lo_end <= hi_beg && lo_end - lo == hi - hi_beg);

    /*
       for (int i=lo; i<lo+n-n2; i++)
       compare(i, i+n2, dir);
     */

    sc_merge_bitonic (pst, lo, lo + n2, dir);
    sc_merge_bitonic (pst, lo + n2, hi, dir);
  }
}

static void
sc_psort_bitonic (sc_psort_t * pst, size_t lo, size_t hi, bool dir)
{
  const size_t        n = hi - lo;

  if (n > 1 && pst->my_hi > lo && pst->my_lo < hi) {
    if (lo >= pst->my_lo && hi <= pst->my_hi) {
      qsort (pst->my_base + (lo - pst->my_lo) * pst->size,
             n, pst->size, dir ? sc_compare : sc_icompare);
    }
    else {
      const size_t        n2 = n / 2;

      sc_psort_bitonic (pst, lo, lo + n2, !dir);
      sc_psort_bitonic (pst, lo + n2, hi, dir);
      sc_merge_bitonic (pst, lo, hi, dir);
    }
  }
}

/** Sort a distributed set of values in parallel.
 * This algorithm uses bitonic sort between processors and qsort locally.
 * The partition of the data can be arbitrary and is not changed.
 * \param [in] mpicomm          Communicator to use.
 * \param [in] base             Pointer to the local subset of data.
 * \param [in] nmemb            Array of mpisize counts of local data.
 * \param [in] size             Size in bytes of each data value.
 * \param [in] compar           Comparison function to use.
 */
void
sc_psort (MPI_Comm mpicomm, void *base, size_t * nmemb, size_t size,
          int (*compar) (const void *, const void *))
{
  int                 mpiret;
  int                 num_procs, rank;
  int                 i;
  size_t             *gmemb;
  sc_psort_t          pst;

  SC_ASSERT (sc_compare == NULL);

  /* get basic MPI information */
  mpiret = MPI_Comm_size (mpicomm, &num_procs);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Comm_rank (mpicomm, &rank);
  SC_CHECK_MPI (mpiret);

  /* alloc global offset array */
  gmemb = SC_ALLOC (size_t, num_procs + 1);
  gmemb[0] = 0;
  for (i = 0; i < num_procs; ++i) {
    gmemb[i + 1] = gmemb[i] + nmemb[i];
  }

  /* set up internal state and call recursion */
  pst.mpicomm = mpicomm;
  pst.num_procs = num_procs;
  pst.rank = rank;
  pst.size = size;
  pst.my_lo = gmemb[rank];
  pst.my_hi = gmemb[rank + 1];
  pst.total = gmemb[num_procs];
  pst.gmemb = gmemb;
  pst.my_base = (char *) base;
  sc_compare = compar;
  SC_GLOBAL_LDEBUGF ("Total values to sort %lld\n", (long long) pst.total);
  sc_psort_bitonic (&pst, 0, pst.total, true);

  /* clean up and free memory */
  sc_compare = NULL;
  SC_FREE (gmemb);
}

/*
    private void compare(int i, int j, boolean dir)
    {
        if (dir==(a[i]>a[j]))
            exchange(i, j);
    }

    private void exchange(int i, int j)
    {
        int t=a[i];
        a[i]=a[j];
        a[j]=t;
    }
*/

/* EOF sc_sort.c */
