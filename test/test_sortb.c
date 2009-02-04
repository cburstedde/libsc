/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2008,2009 Carsten Burstedde, Lucas Wilcox.

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

const size_t        size = 24;

int
the_compare (const void *v1, const void *v2)
{
  return memcmp (v1, v2, size);
}

int
main (int argc, char **argv)
{
  int                 mpiret;
  int                 rank, num_procs;
  size_t              nmemb[3], lsize, total;
  size_t              zz;
  char               *ldata;
  MPI_Comm            mpicomm;

  mpiret = MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpicomm = MPI_COMM_WORLD;
  mpiret = MPI_Comm_size (mpicomm, &num_procs);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Comm_rank (mpicomm, &rank);
  SC_CHECK_MPI (mpiret);

  sc_init (mpicomm, true, true, NULL, SC_LP_DEFAULT);

  if (num_procs != 3) {
    SC_GLOBAL_PRODUCTION ("This test will test things only for np = 3\n");
    goto donothing;
  }

  nmemb[0] = 7239;
  nmemb[1] = 7240;
  nmemb[2] = 7240;
  lsize = nmemb[rank];
  total = nmemb[0] + nmemb[1] + nmemb[2];

  srand (17);
  ldata = SC_ALLOC (char, lsize * size);
  for (zz = 0; zz < lsize * size; ++zz) {
    ldata[zz] = (char) (-50. + (300. * rand () / (RAND_MAX + 1.0)));
  }
  sc_psort (mpicomm, ldata, nmemb, size, the_compare);
  // qsort (ldata, nmemb[rank], size, the_compare);

  for (zz = 1; zz < lsize; ++zz) {
    SC_CHECK_ABORT (the_compare (ldata + size * (zz - 1),
                                 ldata + size * zz) <= 0, "Sort");
  }

  /* clean up and exit */
  SC_FREE (ldata);

donothing:
  sc_finalize ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
