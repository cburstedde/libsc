/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2007,2008 Carsten Burstedde, Lucas Wilcox.

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
#include <sc_dmatrix.h>

static const double eps = 2.220446049250313e-16;

int
main (int argc, char **argv)
{
  int                 num_failed_tests = 0;
  int                 j;
  int                 rank;
  int                 mpiret;
  sc_dmatrix_t       *A, *x, *xexact, *b, *bT, *xT, *xTexact;
  double              xmaxerror = 0.0;
  double              A_data[] = { 8.0, 1.0, 6.0,
    3.0, 5.0, 7.0,
    4.0, 9.0, 2.0
  };
  double              b_data[] = { 1.0, 2.0, 3.0 };
  double              xexact_data[] = { -0.1 / 3.0, 1.4 / 3.0, -0.1 / 3.0 };

  mpiret = MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  SC_CHECK_MPI (mpiret);

  sc_init (rank, NULL, NULL, NULL, SC_LP_DEFAULT);

  A = sc_dmatrix_view (3, 3, A_data);
  b = sc_dmatrix_view (1, 3, b_data);
  xexact = sc_dmatrix_view (1, 3, xexact_data);
  x = sc_dmatrix_new (1, 3);

  sc_dmatrix_rdivide (SC_NO_TRANS, b, A, x);

  sc_dmatrix_add (-1.0, xexact, x);

  xmaxerror = 0.0;
  for (j = 0; j < 3; ++j) {
    xmaxerror = SC_MAX (xmaxerror, fabs (x->e[0][j]));
  }

  SC_LDEBUGF ("xmaxerror = %g\n", xmaxerror);

  if (xmaxerror > 100.0 * eps) {
    ++num_failed_tests;
  }

  xexact->e[0][0] = 0.05;
  xexact->e[0][1] = 0.3;
  xexact->e[0][2] = 0.05;

  sc_dmatrix_rdivide (SC_TRANS, b, A, x);

  sc_dmatrix_add (-1.0, xexact, x);

  xmaxerror = 0.0;
  for (j = 0; j < 3; ++j) {
    xmaxerror = SC_MAX (xmaxerror, fabs (x->e[0][j]));
  }

  SC_LDEBUGF ("xmaxerror = %g\n", xmaxerror);

  if (xmaxerror > 100.0 * eps) {
    ++num_failed_tests;
  }

  bT = sc_dmatrix_view (3, 1, b_data);
  xT = sc_dmatrix_new (3, 1);
  xTexact = sc_dmatrix_new (3, 1);

  xTexact->e[0][0] = 0.05;
  xTexact->e[1][0] = 0.3;
  xTexact->e[2][0] = 0.05;

  sc_dmatrix_ldivide (SC_NO_TRANS, A, bT, xT);

  sc_dmatrix_add (-1.0, xTexact, xT);

  xmaxerror = 0.0;
  for (j = 0; j < 3; ++j) {
    xmaxerror = SC_MAX (xmaxerror, fabs (xT->e[0][j]));
  }

  SC_LDEBUGF ("xTmaxerror = %g\n", xmaxerror);

  if (xmaxerror > 100.0 * eps) {
    ++num_failed_tests;
  }

  sc_dmatrix_destroy (xTexact);
  sc_dmatrix_destroy (xT);
  sc_dmatrix_destroy (bT);
  sc_dmatrix_destroy (A);
  sc_dmatrix_destroy (b);
  sc_dmatrix_destroy (x);
  sc_dmatrix_destroy (xexact);

  sc_finalize ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return num_failed_tests;
}

/* EOF test_dmatrix.c */
