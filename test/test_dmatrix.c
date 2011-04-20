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

#if defined(SC_BLAS) && defined(SC_LAPACK)

static const double eps = 2.220446049250313e-16;

static void
test_zero_sizes (void)
{
  sc_dmatrix_t      *m1, *m2, *m3;

  m1 = sc_dmatrix_new (0, 3);
  sc_dmatrix_set_value (m1, -5.);

  m2 = sc_dmatrix_clone (m1);
  sc_dmatrix_fabs (m1, m2);
  sc_dmatrix_resize (m2, 3, 0);

  m3 = sc_dmatrix_new (0, 0);
  sc_dmatrix_multiply (SC_NO_TRANS, SC_NO_TRANS, 1., m1, m2, 0., m3);

  sc_dmatrix_destroy (m1);
  sc_dmatrix_destroy (m2);
  sc_dmatrix_destroy (m3);
}

#endif

int
main (int argc, char **argv)
{
  int                 num_failed_tests = 0;
#if defined(SC_BLAS) && defined(SC_LAPACK)
  int                 j;
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

  sc_init (MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

  A = sc_dmatrix_new_data (3, 3, A_data);
  b = sc_dmatrix_new_data (1, 3, b_data);
  xexact = sc_dmatrix_new_data (1, 3, xexact_data);
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

  bT = sc_dmatrix_new_data (3, 1, b_data);
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

  test_zero_sizes ();

  sc_finalize ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);
#endif

  return num_failed_tests;
}
