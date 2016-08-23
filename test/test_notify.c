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

#include <sc_notify.h>

int
main (int argc, char **argv)
{
  int                 i;
  int                 mpiret;
  int                 mpisize, mpirank;
  int                *senders, num_senders;
  int                *senders2, num_senders2;
  int                *receivers, num_receivers;
  double              elapsed_allgather;
  double              elapsed_native;
  sc_MPI_Comm         mpicomm;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpicomm = sc_MPI_COMM_WORLD;
  mpiret = sc_MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  sc_init (mpicomm, 1, 1, NULL, SC_LP_DEFAULT);

  num_receivers = (mpirank * (mpirank % 100)) % 7;
  num_receivers = SC_MIN (num_receivers, mpisize);
  receivers = SC_ALLOC (int, num_receivers);
  for (i = 0; i < num_receivers; ++i) {
    receivers[i] = (3 * mpirank + i) % mpisize;
  }
  qsort (receivers, num_receivers, sizeof (int), sc_int_compare);

  SC_GLOBAL_INFO ("Testing sc_notify_allgather\n");
  senders = SC_ALLOC (int, mpisize);
  elapsed_allgather = -sc_MPI_Wtime ();
  mpiret = sc_notify_allgather (receivers, num_receivers,
                                senders, &num_senders, mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_allgather += sc_MPI_Wtime ();

  SC_GLOBAL_INFO ("Testing native sc_notify\n");
  senders2 = SC_ALLOC (int, mpisize);
  elapsed_native = -sc_MPI_Wtime ();
  mpiret = sc_notify (receivers, num_receivers,
                      senders2, &num_senders2, mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_native += sc_MPI_Wtime ();

  SC_CHECK_ABORT (num_senders == num_senders2, "Mismatched sender numbers");
  for (i = 0; i < num_senders; ++i) {
    SC_CHECK_ABORTF (senders[i] == senders2[i], "Mismatched sender %d", i);
  }

  SC_FREE (receivers);
  SC_FREE (senders);
  SC_FREE (senders2);

  SC_GLOBAL_STATISTICSF ("   notify_allgather %g\n", elapsed_allgather);
  SC_GLOBAL_STATISTICSF ("   notify           %g\n", elapsed_native);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
