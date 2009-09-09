/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2008,2009 Carsten Burstedde, Lucas Wilcox.

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

#include <sc_dmatrix.h>

static void
check_matrix_vector (void)
{
  int                 i, j, k;
  double              d[23];
  sc_dmatrix_t       *A, *X3, *X4, *Y3a, *Y3b, *Y4a, *Y4b;

  for (k = 0; k < 23; ++k) {
    d[k] = (k + 3) * (k - 3 * k) + sqrt (2. + sin (k * 1.22345));
  }

  A = sc_dmatrix_new (3, 4);
  X3 = sc_dmatrix_new (3, 1);
  X4 = sc_dmatrix_new (1, 4);
  Y3a = sc_dmatrix_new (3, 1);
  Y3b = sc_dmatrix_new (1, 3);
  Y4a = sc_dmatrix_new (4, 1);
  Y4b = sc_dmatrix_new (1, 4);

  for (i = 0; i < 3; ++i) {
    X3->e[i][0] = i + 1.;
  }
  for (j = 0; j < 4; ++j) {
    X4->e[0][j] = j + 1.;
  }

  k = 0;
  for (i = 0; i < 3; ++i) {
    for (j = 0; j < 4; ++j) {
      A->e[i][j] = d[k];
      k = (k + 1) % 23;
    }
  }

  sc_dmatrix_vector (SC_NO_TRANS, SC_TRANS, SC_NO_TRANS, 1., A, X4, 0., Y3a);
  sc_dmatrix_vector (SC_NO_TRANS, SC_TRANS, SC_NO_TRANS, 2., A, X4, 2., Y3a);
  sc_dmatrix_vector (SC_NO_TRANS, SC_TRANS, SC_TRANS, 4., A, X4, 0., Y3b);
  for (i = 0; i < 3; ++i) {
    Y3b->e[0][i] -= Y3a->e[i][0];
  }
  sc_dmatrix_multiply (SC_NO_TRANS, SC_TRANS, 8., A, X4, -2., Y3a);
  printf ("0 =\n");
  sc_dmatrix_write (Y3a, stdout);
  printf ("0 =\n");
  sc_dmatrix_write (Y3b, stdout);

  sc_dmatrix_vector (SC_TRANS, SC_NO_TRANS, SC_NO_TRANS, 1., A, X3, 0., Y4a);
  sc_dmatrix_vector (SC_TRANS, SC_NO_TRANS, SC_NO_TRANS, 1., A, X3, 2., Y4a);
  sc_dmatrix_vector (SC_TRANS, SC_NO_TRANS, SC_TRANS, 3., A, X3, 0., Y4b);
  for (j = 0; j < 4; ++j) {
    Y4b->e[0][j] -= Y4a->e[j][0];
  }
  sc_dmatrix_multiply (SC_TRANS, SC_NO_TRANS, 3., A, X3, -1., Y4a);
  printf ("0 =\n");
  sc_dmatrix_write (Y4a, stdout);
  printf ("0 =\n");
  sc_dmatrix_write (Y4b, stdout);

  sc_dmatrix_destroy (A);
  sc_dmatrix_destroy (X3);
  sc_dmatrix_destroy (X4);
  sc_dmatrix_destroy (Y3a);
  sc_dmatrix_destroy (Y3b);
  sc_dmatrix_destroy (Y4a);
  sc_dmatrix_destroy (Y4b);
}

static void
check_matrix_multiply ()
{
  sc_dmatrix_t       *A, *B, *C, *D, *E, *vA;
  double              alpha, beta;

  A = sc_dmatrix_new (3, 2);
  B = sc_dmatrix_new (2, 3);
  C = sc_dmatrix_new (3, 3);

  A->e[0][0] = 1;
  A->e[0][1] = 2;
  A->e[1][0] = 3;
  A->e[1][1] = 4;
  A->e[2][0] = 5;
  A->e[2][1] = 6;
  printf ("A =\n");
  sc_dmatrix_write (A, stdout);

  B->e[0][0] = 1;
  B->e[0][1] = 7;
  B->e[0][2] = 2;
  B->e[1][0] = 3;
  B->e[1][1] = 5;
  B->e[1][2] = 4;
  printf ("B =\n");
  sc_dmatrix_write (B, stdout);

  alpha = 1.0;
  beta = 0.0;

  sc_dmatrix_multiply (SC_NO_TRANS, SC_NO_TRANS, alpha, A, B, beta, C);
  printf ("C =\n");
  sc_dmatrix_write (C, stdout);

  D = sc_dmatrix_new (2, 3);

  sc_dmatrix_multiply (SC_TRANS, SC_NO_TRANS, alpha, A, C, beta, D);
  printf ("D =\n");
  sc_dmatrix_write (D, stdout);

  E = sc_dmatrix_new (2, 2);

  sc_dmatrix_multiply (SC_NO_TRANS, SC_TRANS, alpha, D, B, beta, E);
  printf ("E =\n");
  sc_dmatrix_write (E, stdout);

  vA = sc_dmatrix_new_view (3, 2, A);

  sc_dmatrix_multiply (SC_NO_TRANS, SC_NO_TRANS, alpha, vA, B, beta, C);
  printf ("C =\n");
  sc_dmatrix_write (C, stdout);

  sc_dmatrix_reshape (vA, 2, 3);
  printf ("reshape(2, 3, vA) =\n");
  sc_dmatrix_write (vA, stdout);

  sc_dmatrix_destroy (vA);
  sc_dmatrix_destroy (A);
  sc_dmatrix_destroy (B);
  sc_dmatrix_destroy (C);
  sc_dmatrix_destroy (D);
  sc_dmatrix_destroy (E);
}

int
main (int argc, char **argv)
{
  sc_init (MPI_COMM_NULL, 1, 1, NULL, SC_LP_DEFAULT);

  check_matrix_vector ();
  check_matrix_multiply ();

  sc_finalize ();

  return 0;
}
