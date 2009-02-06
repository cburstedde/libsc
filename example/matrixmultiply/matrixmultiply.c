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

/* Note this is based on an example provided by Tim Warburton to test
 * matrix matrix multiply optimizations.
 */

#include <sc_dmatrix.h>

#ifdef SC_HAVE_TIME_H
#include <time.h>
#endif

#define gnr 160
#define gnc 160

static inline void
matrixset (sc_dmatrix_t * A, int r, int c, double val)
{
  A->e[0][r * gnc + c] = val;
}

static inline double
matrixget (sc_dmatrix_t * A, int r, int c)
{
  return A->e[0][r * gnc + c];
}

void
matrixsetrandom (sc_dmatrix_t * A)
{
  int                 r, c;
  for (r = 0; r < gnr; r = r + 1) {
    for (c = 0; c < gnc; c = c + 1) {
      matrixset (A, r, c, rand () / (RAND_MAX + 1.));
    }
  }
}

void
matrixmultiply_nonopt (sc_dmatrix_t * A, sc_dmatrix_t * B, sc_dmatrix_t * C)
{
  int                 r, c, n;
  double              d;
  for (r = 0; r < gnr; r = r + 1) {
    for (c = 0; c < gnc; c = c + 1) {
      d = 0.0;
      for (n = 0; n < gnc; ++n) {
        d = d + matrixget (A, r, n) * matrixget (B, n, c);
      }
      matrixset (C, r, c, d);
    }
  }
}

void
matrixmultiply_sse (sc_dmatrix_t * A, sc_dmatrix_t * B, sc_dmatrix_t * C)
{
  register int        r, c, n;

  const double       *restrict fA = A->e[0];
  const double       *restrict fB = B->e[0];
  double             *restrict fC = C->e[0];

  for (r = 0; r < gnr; ++r) {
    for (n = 0; n < gnc; ++n) {
      const double        fArn = fA[gnc * r + n];
      for (c = 0; c < gnc; c = c + 1) {
        fC[gnc * r + c] += fArn * fB[gnc * n + c];
      }
    }
  }
}

static void
time_matrix_multiply ()
{
  int                 Nloops, loop;
  sc_dmatrix_t       *A, *B, *C;
  double              alpha, beta, a;

  long int            t0, t1;

  SC_ASSERT (gnr == gnc);

  Nloops = 1 + (int) (500 * 500 * 500 / (double) (gnr * gnr * gnr));

  A = sc_dmatrix_new (gnr, gnc);
  B = sc_dmatrix_new (gnr, gnc);
  C = sc_dmatrix_new (gnr, gnc);

  t0 = (long int) clock ();
  for (loop = 0; loop < Nloops; ++loop)
    matrixmultiply_nonopt (A, B, C);
  t1 = (long int) clock ();

  /* make sure the multiply gets done */
  a = matrixget (C, 2, 2);

  SC_PRODUCTIONF ("unoptimized time taken = %lg for %d x %d\n",
                  (double) (t1 - t0) / (Nloops * CLOCKS_PER_SEC), gnr, gnc);

  t0 = (long int) clock ();
  for (loop = 0; loop < Nloops; ++loop)
    matrixmultiply_sse (A, B, C);
  t1 = (long int) clock ();

  /* make sure the multiply gets done */
  a = matrixget (C, 2, 2);

  SC_PRODUCTIONF ("optimized time taken = %lg for %d x %d\n",
                  (double) (t1 - t0) / (Nloops * CLOCKS_PER_SEC), gnr, gnc);

  alpha = 1.0;
  beta = 0.0;

  t0 = (long int) clock ();
  for (loop = 0; loop < Nloops; ++loop)
    sc_dmatrix_multiply (SC_NO_TRANS, SC_NO_TRANS, alpha, A, B, beta, C);
  t1 = (long int) clock ();

  /* make sure the multiply gets done */
  a = matrixget (C, 2, 2);

  SC_PRODUCTIONF ("blas time taken = %lg for %d x %d\n",
                  (double) (t1 - t0) / (Nloops * CLOCKS_PER_SEC), gnr, gnc);

  sc_dmatrix_destroy (C);
  sc_dmatrix_destroy (B);
  sc_dmatrix_destroy (A);
}

int
main (int argc, char **argv)
{
  sc_init (MPI_COMM_NULL, true, true, NULL, SC_LP_DEFAULT);

  time_matrix_multiply ();

  sc_finalize ();

  return 0;
}
