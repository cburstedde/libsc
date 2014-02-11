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

#include <sc_allgather.h>

int
main (int argc, char **argv)
{
  sc_MPI_Comm         mpicomm;
  int                 mpiret;
  int                 mpisize;
  int                 mpirank;
  int                 i;
  int                *idata;
  double              elapsed_alltoall = 0.;
  double              elapsed_recursive;
  double              dsend;
  double             *ddata1;
  double             *ddata2;
  double              elapsed_allgather;
  double              elapsed_replacement;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpicomm = sc_MPI_COMM_WORLD;
  mpiret = sc_MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  sc_init (mpicomm, 1, 1, NULL, SC_LP_DEFAULT);

  idata = SC_ALLOC (int, mpisize);

  if (mpisize <= 64) {
    SC_GLOBAL_INFO ("Testing sc_allgather_alltoall\n");

    for (i = 0; i < mpisize; ++i) {
      idata[i] = (i == mpirank) ? mpirank : -1;
    }
    elapsed_alltoall = -sc_MPI_Wtime ();
    sc_allgather_alltoall (mpicomm, (char *) idata, (int) sizeof (int),
                           mpisize, mpirank, mpirank);
    elapsed_alltoall += sc_MPI_Wtime ();
    for (i = 0; i < mpisize; ++i) {
      SC_ASSERT (idata[i] == i);
    }
  }

  SC_GLOBAL_INFO ("Testing sc_allgather_recursive\n");

  for (i = 0; i < mpisize; ++i) {
    idata[i] = (i == mpirank) ? mpirank : -1;
  }
  elapsed_recursive = -sc_MPI_Wtime ();
  sc_allgather_recursive (mpicomm, (char *) idata, (int) sizeof (int),
                          mpisize, mpirank, mpirank);
  elapsed_recursive += sc_MPI_Wtime ();
  for (i = 0; i < mpisize; ++i) {
    SC_ASSERT (idata[i] == i);
  }

  SC_FREE (idata);

  ddata1 = SC_ALLOC (double, mpisize);
  ddata2 = SC_ALLOC (double, mpisize);

  SC_GLOBAL_INFO ("Testing allgather and replacement\n");

  dsend = M_PI;

  mpiret = sc_MPI_Barrier (mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_allgather = -sc_MPI_Wtime ();
  mpiret = sc_MPI_Allgather (&dsend, 1, sc_MPI_DOUBLE, ddata1, 1,
                             sc_MPI_DOUBLE, mpicomm);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Barrier (mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_allgather += sc_MPI_Wtime ();

  mpiret = sc_MPI_Barrier (mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_replacement = -sc_MPI_Wtime ();
  mpiret = sc_allgather (&dsend, 1, sc_MPI_DOUBLE, ddata2, 1, sc_MPI_DOUBLE,
                         mpicomm);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Barrier (mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_replacement += sc_MPI_Wtime ();

  for (i = 0; i < mpisize; ++i) {
    SC_ASSERT (ddata1[i] == ddata2[i]); /* exact match wanted */
  }
  SC_ASSERT (ddata1[mpirank] == dsend); /* exact match wanted */

  SC_FREE (ddata1);
  SC_FREE (ddata2);

  SC_GLOBAL_STATISTICSF ("Timings with threshold %d on %d cores\n",
                         SC_AG_ALLTOALL_MAX, mpisize);
  SC_GLOBAL_STATISTICSF ("   alltoall %g\n", elapsed_alltoall);
  SC_GLOBAL_STATISTICSF ("   recursive %g\n", elapsed_recursive);
  SC_GLOBAL_STATISTICSF ("   allgather %g\n", elapsed_allgather);
  SC_GLOBAL_STATISTICSF ("   replacement %g\n", elapsed_replacement);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
