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

#include <sc_reduce.h>

int
main (int argc, char **argv)
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
  sc_MPI_Comm         mpicomm;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  mpicomm = sc_MPI_COMM_WORLD;
  mpiret = sc_MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  sc_init (mpicomm, 1, 1, NULL, SC_LP_DEFAULT);

  /* test allreduce int max */
  ivalue = mpirank;
  sc_allreduce (&ivalue, &iresult, 1, sc_MPI_INT, sc_MPI_MAX, mpicomm);
  SC_CHECK_ABORT (iresult == mpisize - 1, "Allreduce mismatch");

  /* test reduce float max */
  fvalue[0] = (float) mpirank;
  fexpect[0] = (float) (mpisize - 1);
  fvalue[1] = (float) (mpirank % 9 - 4);
  fexpect[1] = (float) (mpisize >= 9 ? 4 : (mpisize - 1) % 9 - 4);
  fvalue[2] = (float) (mpirank % 6);
  fexpect[2] = (float) (mpisize >= 6 ? 5 : (mpisize - 1) % 6);
  for (i = 0; i < mpisize; ++i) {
    sc_reduce (fvalue, fresult, 3, sc_MPI_FLOAT, sc_MPI_MAX, i, mpicomm);
    if (i == mpirank) {
      for (j = 0; j < 3; ++j) {
        SC_CHECK_ABORTF (fresult[j] == fexpect[j],      /* ok */
                         "Reduce mismatch in %d", j);
      }
    }
  }

  /* test allreduce char min */
  cvalue = (char) (mpirank % 127);
  sc_allreduce (&cvalue, &cresult, 1, sc_MPI_CHAR, sc_MPI_MIN, mpicomm);
  SC_CHECK_ABORT (cresult == 0, "Allreduce mismatch");

  /* test reduce unsigned short min */
  usvalue = (unsigned short) (mpirank % 32767);
  for (i = 0; i < mpisize; ++i) {
    sc_reduce (&usvalue, &usresult, 1, sc_MPI_UNSIGNED_SHORT, sc_MPI_MIN, i,
               mpicomm);
    if (i == mpirank) {
      SC_CHECK_ABORT (usresult == 0, "Reduce mismatch");
    }
  }

  /* test allreduce long sum */
  lvalue = (long) mpirank;
  sc_allreduce (&lvalue, &lresult, 1, sc_MPI_LONG, sc_MPI_SUM, mpicomm);
  SC_CHECK_ABORT (lresult == ((long) (mpisize - 1)) * mpisize / 2,
                  "Allreduce mismatch");

  /* test reduce double sum */
  dvalue = (double) mpirank;
  for (i = 0; i < mpisize; ++i) {
    sc_reduce (&dvalue, &dresult, 1, sc_MPI_DOUBLE, sc_MPI_SUM, i, mpicomm);
    if (i == mpirank) {
      SC_CHECK_ABORT (dresult == ((double) (mpisize - 1)) * mpisize / 2.,       /* ok */
                      "Reduce mismatch");
    }
  }

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
