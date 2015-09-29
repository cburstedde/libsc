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

#include <sc_dmatrix.h>

int
main (int argc, char **argv)
{
  const int           n_threads = 4;
  const int           n_blocks = 19;
  const int           n_entries = 31;
#ifdef SC_MEMALIGN_BYTES
  const int           memalign_bytes = SC_MEMALIGN_BYTES;
#else
  const int           memalign_bytes = 32;
#endif
  int                 mpiret;
  sc_darray_work_t   *work;
  double             *workd;
  int                 t, b, i;

  /* initialize mpi */
  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  /* initialize sc */
  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

  /* allocate workspace */
  work = sc_darray_work_new (n_threads, n_blocks, n_entries, memalign_bytes);

  /* check size of allocation */
  SC_CHECK_ABORTF (n_blocks == sc_darray_work_get_blockcount (work),
                   "Wrong number of blocks %i, should be %i\n",
                   sc_darray_work_get_blockcount (work), n_blocks);
  SC_CHECK_ABORTF (n_entries <= sc_darray_work_get_blocksize (work),
                   "Insufficient number of entries per block %i, should be at least %i\n",
                   sc_darray_work_get_blocksize (work), n_entries);

  /* write to all entries of workspace */
  for (t = 0; t < n_threads; t++) {
    for (b = 0; b < n_blocks; b++) {
      workd = sc_darray_work_get (work, t, b);
      for (i = 0; i < n_entries; i++) {
        workd[i] = (double) i;
      }
    }
  }

  /* destroy */
  sc_darray_work_destroy (work);

  /* finalize sc */
  sc_finalize ();

  /* finalize mpi */
  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
