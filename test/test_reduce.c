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
  int                 i, j;
  char                cvalue, cresult;
  int                 ivalue, iresult;
  unsigned short      usvalue, usresult;
  long                lvalue, lresult;
  float               fvalue[3], fresult[3], fexpect[3];
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

  /* test allreduce int max */
  ivalue = mpirank;
  sc_allreduce (&ivalue, &iresult, 1, MPI_INT, MPI_MAX, mpicomm);
  SC_CHECK_ABORT (iresult == mpisize - 1, "Allreduce mismatch");

  /* test reduce float max */
  fvalue[0] = (float) mpirank;
  fexpect[0] = (float) (mpisize - 1);
  fvalue[1] = (float) (mpirank % 9 - 4);
  fexpect[1] = (float) (mpisize >= 9 ? 4 : (mpisize - 1) % 9 - 4);
  fvalue[2] = (float) (mpirank % 6);
  fexpect[2] = (float) (mpisize >= 6 ? 5 : (mpisize - 1) % 6);
  for (i = 0; i < mpisize; ++i) {
    sc_reduce (fvalue, fresult, 3, MPI_FLOAT, MPI_MAX, i, mpicomm);
    if (i == mpirank) {
      for (j = 0; j < 3; ++j) {
	SC_CHECK_ABORTF (fresult[j] == fexpect[j],         /* ok */
			 "Reduce mismatch in %d", j);
      }
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

  /* test allreduce long sum */
  lvalue = (long) mpirank;
  sc_allreduce (&lvalue, &lresult, 1, MPI_LONG, MPI_SUM, mpicomm);
  SC_CHECK_ABORT (lresult == ((long) (mpisize - 1)) * mpisize / 2,
		  "Allreduce mismatch");

  /* test reduce double sum */
  dvalue = (double) mpirank;
  for (i = 0; i < mpisize; ++i) {
    sc_reduce (&dvalue, &dresult, 1, MPI_DOUBLE, MPI_SUM, i, mpicomm);
    if (i == mpirank) {
      SC_CHECK_ABORT (dresult ==
		      ((double) (mpisize - 1)) * mpisize / 2.,  /* ok */
		      "Reduce mismatch");
    }
  }

  sc_finalize ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
