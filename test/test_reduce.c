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
  char                cvalue, cresult;
  unsigned short      usvalue, usresult;
  long                lvalue, lresult;
  double              dvalue, dresult;
  MPI_Comm            mpicomm;

  mpiret = MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  mpicomm = MPI_COMM_WORLD;
  mpiret = MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  sc_init (mpicomm, 1, 1, NULL, SC_LP_DEFAULT);

  /* test allreduce long max */
  lvalue = (long) mpirank;
  sc_allreduce (&lvalue, &lresult, 1, MPI_LONG, MPI_MAX, mpicomm);
  SC_CHECK_ABORT (lresult == (long) (mpisize - 1), "Allreduce mismatch");

  /* test reduce double max */
  dvalue = (double) mpirank;
  for (i = 0; i < mpisize; ++i) {
    sc_reduce (&dvalue, &dresult, 1, MPI_DOUBLE, MPI_MAX, i, mpicomm);
    if (i == mpirank) {
      SC_CHECK_ABORT (dresult == (double) (mpisize - 1),        /* ok */
		      "Reduce mismatch");
    }
  }

  /* test allreduce char min */
  cvalue = (char) (mpirank % 127);
  sc_allreduce (&cvalue, &cresult, 1, MPI_CHAR, MPI_MIN, mpicomm);
  SC_CHECK_ABORT (cresult == 0, "Allreduce mismatch");

  /* test reduce unsigned short min */
  usvalue = (unsigned short) (mpirank % 32767);
  for (i = 0; i < mpisize; ++i) {
    sc_reduce (&usvalue, &usresult, 1, MPI_UNSIGNED_SHORT, MPI_MIN, i, mpicomm);
    if (i == mpirank) {
      SC_CHECK_ABORT (usresult == 0, "Reduce mismatch");
    }
  }

  sc_finalize ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
