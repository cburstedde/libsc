/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2010 Carsten Burstedde, Lucas Wilcox.

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

#include <sc_reduce.h>

int
main (int argc, char ** argv)
{
  int                 mpiret;
  int                 mpirank, mpisize;
  int                 i;
  double              value, result;
  MPI_Comm            mpicomm;

  mpiret = MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  mpicomm = MPI_COMM_WORLD;
  mpiret = MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  sc_init (mpicomm, 1, 1, NULL, SC_LP_DEFAULT);

  for (i = 0; i < mpisize; ++i) {
    value = (double) mpirank;
    sc_reduce (&value, &result, 1, MPI_DOUBLE, MPI_MAX, i, mpicomm);

    if (i == mpirank) {
      SC_LDEBUGF ("Reduced on %d to %g\n", i, result);
    }
  }

  sc_finalize ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
