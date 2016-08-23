/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System
  Additional copyright (C) 2011 individual authors

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
  sc_MPI_Comm         mpicomm;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpicomm = sc_MPI_COMM_WORLD;
  mpiret = sc_MPI_Comm_size (mpicomm, &num_procs);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &rank);
  SC_CHECK_MPI (mpiret);

  sc_init (mpicomm, 1, 1, NULL, SC_LP_DEFAULT);

  if (num_procs != 3) {
    SC_GLOBAL_PRODUCTION ("This test will test things only for np = 3\n");
    goto donothing;
  }

  nmemb[0] = 7239;
  nmemb[1] = 7240;
  nmemb[2] = 7240;
  lsize = nmemb[rank];
  total = nmemb[0] + nmemb[1] + nmemb[2];

  srand (17 + (int) total);
  ldata = SC_ALLOC (char, lsize * size);
  for (zz = 0; zz < lsize * size; ++zz) {
    ldata[zz] = (char) (-50. + (300. * rand () / (RAND_MAX + 1.0)));
  }
  sc_psort (mpicomm, ldata, nmemb, size, the_compare);
  /* qsort (ldata, nmemb[rank], size, the_compare); */

  for (zz = 1; zz < lsize; ++zz) {
    SC_CHECK_ABORT (the_compare (ldata + size * (zz - 1),
                                 ldata + size * zz) <= 0, "Sort");
  }

  /* clean up and exit */
  SC_FREE (ldata);

donothing:
  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
