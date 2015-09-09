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

#define TEST_DMATRIX_M 4
#define TEST_DMATRIX_N 13

#if defined(SC_WITH_BLAS) && defined(SC_WITH_LAPACK)
static const double eps = 2.220446049250313e-16;
#endif

/**
 * Generates a random number uniformly distributed in `[alpha,beta)`.
 * \return  random number.
 */
static double
test_dmatrix_get_random_uniform (const double alpha, const double beta)
{
  return alpha + ( rand () / (RAND_MAX/(beta - alpha)) );
}

/**
 * Fills a dmatrix with random numbers.
 */
static void
test_dmatrix_set_random (sc_dmatrix_t * mat, const double alpha,
                         const double beta)
{
  const sc_bint_t     totalsize = mat->m * mat->n;
  double             *mat_data = mat->e[0];
  sc_bint_t           i;

  for (i = 0; i < totalsize; ++i) {
    mat_data[i] = test_dmatrix_get_random_uniform (alpha, beta);
  }
}

/**
 * Checks entries of a matrix comparing to a reference matrix.
 * \return  number of non-identical entries.
 */
static sc_bint_t
test_dmatrix_check_error_identical (const sc_dmatrix_t * mat_chk,
                                    const sc_dmatrix_t * mat_ref)
{
  const sc_bint_t     totalsize = mat_chk->m * mat_chk->n;
  double             *mat_chk_data = mat_chk->e[0];
  double             *mat_ref_data = mat_ref->e[0];
  sc_bint_t           i;
  sc_bint_t           error_count = 0;

  SC_ASSERT (totalsize == mat_ref->m * mat_ref->n);

  for (i = 0; i < totalsize; ++i) {
    if (DBL_MIN < fabs (mat_chk_data[i] - mat_ref_data[i])) {
      error_count++;
    }
  }

  return error_count;
}

#if defined(SC_WITH_BLAS) && defined(SC_WITH_LAPACK)
/**
 * Tests multiplication with matrices of zero number of rows or columns or both.
 */
static void
test_zero_sizes (void)
{
  sc_dmatrix_t       *m1, *m2, *m3;

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

/**
 * Tests function
 *   sc_dmatrix_scale_shift
 * against
 *   sc_dmatrix_scale -> sc_dmatrix_shift
 *
 * \return  number of entries with errors.
 */
static int
test_scale_shift ()
{
  const double        scale = M_PI;
  const double        shift = M_E;
  sc_dmatrix_t       *mat_chk, *mat_ref;
  sc_bint_t           n_err_entries;

  /* create & fill matrices with random values */
  mat_chk = sc_dmatrix_new (TEST_DMATRIX_M, TEST_DMATRIX_N);
  mat_ref = sc_dmatrix_new (TEST_DMATRIX_M, TEST_DMATRIX_N);
  test_dmatrix_set_random (mat_chk, 0.0, 1.0);
  sc_dmatrix_copy (mat_chk, mat_ref);

  /* compute via function that's being tested */
  sc_dmatrix_scale_shift (scale, shift, mat_chk);

  /* compute reference */
  sc_dmatrix_scale (scale, mat_ref);
  sc_dmatrix_shift (shift, mat_ref);

  /* check error */
  n_err_entries = test_dmatrix_check_error_identical (mat_chk, mat_ref);

  /* destroy */
  sc_dmatrix_destroy (mat_chk);
  sc_dmatrix_destroy (mat_ref);

  /* return number of entries with errors */
  return (int) n_err_entries;
}

/**
 * Tests function
 *   sc_dmatrix_dotmultiply_add
 * against
 *   sc_dmatrix_dotmultiply -> sc_dmatrix_add
 *
 * \return  number of entries with errors.
 */
static int
test_dotmultiply_add ()
{
  sc_dmatrix_t       *mat_in, *mat_mult;
  sc_dmatrix_t       *mat_chk, *mat_ref;
  sc_bint_t           n_err_entries;

  /* create & fill matrices with random values */
  mat_in = sc_dmatrix_new (TEST_DMATRIX_M, TEST_DMATRIX_N);
  mat_mult = sc_dmatrix_new (TEST_DMATRIX_M, TEST_DMATRIX_N);
  mat_chk = sc_dmatrix_new (TEST_DMATRIX_M, TEST_DMATRIX_N);
  mat_ref = sc_dmatrix_new (TEST_DMATRIX_M, TEST_DMATRIX_N);
  test_dmatrix_set_random (mat_in, 0.0, 1.0);
  test_dmatrix_set_random (mat_mult, 0.0, 1.0);
  test_dmatrix_set_random (mat_chk, 0.0, 1.0);
  sc_dmatrix_copy (mat_chk, mat_ref);

  /* compute via function that's being tested */
  sc_dmatrix_dotmultiply_add (mat_mult, mat_in, mat_chk);

  /* compute reference */
  sc_dmatrix_dotmultiply (mat_in, mat_mult);
  sc_dmatrix_add (1.0, mat_mult, mat_ref);

  /* check error */
  n_err_entries = test_dmatrix_check_error_identical (mat_chk, mat_ref);

  /* destroy */
  sc_dmatrix_destroy (mat_in);
  sc_dmatrix_destroy (mat_mult);
  sc_dmatrix_destroy (mat_chk);
  sc_dmatrix_destroy (mat_ref);

  /* return number of entries with errors */
  return (int) n_err_entries;
}

/**
 * Runs all dmatrix tests.
 */
int
main (int argc, char **argv)
{
  int                 num_failed_tests = 0;
  int                 mpiret, testret;
#if defined(SC_WITH_BLAS) && defined(SC_WITH_LAPACK)
  int                 j;
  sc_dmatrix_t       *A, *x, *xexact, *b, *bT, *xT, *xTexact, *A2, *b2;
  double              xmaxerror = 0.0;
  double              A_data[] = { 8.0, 1.0, 6.0,
    3.0, 5.0, 7.0,
    4.0, 9.0, 2.0
  };
  double              b_data[] = { 1.0, 2.0, 3.0 };
  double              xexact_data[] = { -0.1 / 3.0, 1.4 / 3.0, -0.1 / 3.0 };
#endif

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

#if defined(SC_WITH_BLAS) && defined(SC_WITH_LAPACK)
  A = sc_dmatrix_new_data (3, 3, A_data);
  A2 = sc_dmatrix_clone (A);
  b = sc_dmatrix_new_data (1, 3, b_data);
  b2 = sc_dmatrix_clone (b);
  xexact = sc_dmatrix_new_data (1, 3, xexact_data);
  x = sc_dmatrix_new (1, 3);

  /* Test 1: solve with A from the right */

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

  /* Test 2: solve with A^T from the left */

  sc_dmatrix_solve_transpose_inplace (A2, b2);

  sc_dmatrix_add (-1.0, xexact, b2);

  xmaxerror = 0.0;
  for (j = 0; j < 3; ++j) {
    xmaxerror = SC_MAX (xmaxerror, fabs (x->e[0][j]));
  }

  SC_LDEBUGF ("xmaxerror = %g\n", xmaxerror);

  if (xmaxerror > 100.0 * eps) {
    ++num_failed_tests;
  }

  /* Test 3: solve with A^T from the right */

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

  /* Test 4: solve with A from the left */

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
  sc_dmatrix_destroy (A2);
  sc_dmatrix_destroy (b2);

  test_zero_sizes ();
#endif

  /* Test 5: scale & shift */
  testret = test_scale_shift ();
  SC_LDEBUGF ("test_scale_shift: #entries with errors = %i\n", testret);
  if (testret != 0) {
    ++num_failed_tests;
  }

  /* Test 6: dotmultiply & add */
  testret = test_dotmultiply_add ();
  SC_LDEBUGF ("test_dotmultiply_add: #entries with errors = %i\n", testret);
  if (testret != 0) {
    ++num_failed_tests;
  }

  /* finalize sc */
  sc_finalize ();

  /* finalize mpi */
  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  /* return number of failed tests */
  return num_failed_tests;
}
