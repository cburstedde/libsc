/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2007-2009 Carsten Burstedde, Lucas Wilcox.

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
#include <sc_lapack.h>

static void
sc_dmatrix_new_e (sc_dmatrix_t * rdm, sc_bint_t m, sc_bint_t n, double *data)
{
  sc_bint_t           i;

  SC_ASSERT (m >= 0 && n >= 0);
  SC_ASSERT (rdm != NULL);

  rdm->e = SC_ALLOC (double *, m + 1);
  rdm->e[0] = data;

  if (m > 0) {
    for (i = 1; i < m; ++i)
      rdm->e[i] = rdm->e[i - 1] + n;

    rdm->e[m] = NULL;           /* safeguard */
  }

  rdm->m = m;
  rdm->n = n;
}

static sc_dmatrix_t *
sc_dmatrix_new_internal (sc_bint_t m, sc_bint_t n, bool init_zero)
{
  sc_dmatrix_t       *rdm;
  double             *data;
  size_t              size = (size_t) (m * n);

  SC_ASSERT (m >= 0 && n >= 0);

  rdm = SC_ALLOC (sc_dmatrix_t, 1);

  if (init_zero) {
    data = SC_ALLOC_ZERO (double, size);
  }
  else {
    data = SC_ALLOC (double, size);
  }

  sc_dmatrix_new_e (rdm, m, n, data);
  rdm->view = false;

  return rdm;
}

sc_dmatrix_t       *
sc_dmatrix_new (sc_bint_t m, sc_bint_t n)
{
  return sc_dmatrix_new_internal (m, n, false);
}

sc_dmatrix_t       *
sc_dmatrix_new_zero (sc_bint_t m, sc_bint_t n)
{
  return sc_dmatrix_new_internal (m, n, true);
}

sc_dmatrix_t       *
sc_dmatrix_new_data (sc_bint_t m, sc_bint_t n, double *data)
{
  sc_dmatrix_t       *rdm;

  SC_ASSERT (m >= 0 && n >= 0);

  rdm = SC_ALLOC (sc_dmatrix_t, 1);
  sc_dmatrix_new_e (rdm, m, n, data);
  rdm->view = true;

  return rdm;
}

sc_dmatrix_t       *
sc_dmatrix_new_view (sc_bint_t m, sc_bint_t n, sc_dmatrix_t * orig)
{
  sc_dmatrix_t       *rdm;

  SC_ASSERT (m >= 0 && n >= 0);
  SC_ASSERT (m * n <= orig->m * orig->n);

  rdm = SC_ALLOC (sc_dmatrix_t, 1);
  sc_dmatrix_new_e (rdm, m, n, orig->e[0]);
  rdm->view = true;

  return rdm;
}

sc_dmatrix_t       *
sc_dmatrix_clone (const sc_dmatrix_t * X)
{
  const sc_bint_t     totalsize = X->m * X->n;
  const double       *Xdata = X->e[0];
  double             *Ydata;
  sc_dmatrix_t       *clone;

  clone = sc_dmatrix_new (X->m, X->n);
  Ydata = clone->e[0];

  memcpy (Ydata, Xdata, totalsize * sizeof (double));

  return clone;
}

void
sc_dmatrix_reshape (sc_dmatrix_t * dmatrix, sc_bint_t m, sc_bint_t n)
{
  double             *data;

  SC_ASSERT (dmatrix->e != NULL);
  SC_ASSERT (dmatrix->m * dmatrix->n == m * n);

  data = dmatrix->e[0];
  SC_FREE (dmatrix->e);
  sc_dmatrix_new_e (dmatrix, m, n, data);
}

void
sc_dmatrix_resize (sc_dmatrix_t * dmatrix, sc_bint_t m, sc_bint_t n)
{
  double             *data;
  sc_bint_t           size, newsize;

  SC_ASSERT (dmatrix->e != NULL);
  SC_ASSERT (m >= 0 && n >= 0);

  size = dmatrix->m * dmatrix->n;
  newsize = m * n;

  if (!dmatrix->view && size != newsize) {
#ifdef SC_USE_REALLOC
    data = SC_REALLOC (dmatrix->e[0], double, newsize);
#else
    data = SC_ALLOC (double, newsize);
    memcpy (data, dmatrix->e[0], (size_t) SC_MIN (newsize, size));
    SC_FREE (dmatrix->e[0]);
#endif
  }
  else {
    /* for views you must know that data is large enough */
    data = dmatrix->e[0];
  }
  SC_FREE (dmatrix->e);
  sc_dmatrix_new_e (dmatrix, m, n, data);
}

void
sc_dmatrix_destroy (sc_dmatrix_t * dmatrix)
{
  if (!dmatrix->view) {
    SC_FREE (dmatrix->e[0]);
  }
  SC_FREE (dmatrix->e);

  SC_FREE (dmatrix);
}

bool
sc_dmatrix_is_valid (const sc_dmatrix_t * A)
{
  return sc_darray_is_valid (A->e[0], A->m * A->n);
}

bool
sc_dmatrix_is_symmetric (const sc_dmatrix_t * A, double tolerance)
{
  sc_bint_t           i, j;
  double              diff;

  SC_ASSERT (A->m == A->n);

  for (i = 0; i < A->n; ++i) {
    for (j = i + 1; j < A->n; ++j) {
      diff = fabs (A->e[i][j] - A->e[j][i]);
      if (diff > tolerance) {
#ifdef SC_DEBUG
        fprintf (stderr, "sc dmatrix not symmetric by %g\n", diff);
#endif
        return false;
      }
    }
  }

  return true;
}

void
sc_dmatrix_set_zero (sc_dmatrix_t * X)
{
  sc_dmatrix_set_value (X, 0.0);
}

void
sc_dmatrix_set_value (sc_dmatrix_t * X, double value)
{
  sc_bint_t           i;
  const sc_bint_t     totalsize = X->m * X->n;
  double             *data = X->e[0];

  for (i = 0; i < totalsize; ++i)
    data[i] = value;
}

void
sc_dmatrix_scale (double alpha, sc_dmatrix_t * X)
{
  sc_bint_t           i;
  const sc_bint_t     totalsize = X->m * X->n;
  double             *data = X->e[0];

  for (i = 0; i < totalsize; ++i)
    data[i] *= alpha;
}

void
sc_dmatrix_dotmult (const sc_dmatrix_t * X, sc_dmatrix_t * Y)
{
  sc_bint_t           i;
  const sc_bint_t     totalsize = X->m * X->n;
  const double       *Xdata = X->e[0];
  double             *Ydata = Y->e[0];

  SC_ASSERT (X->m == Y->m && X->n == Y->n);

  for (i = 0; i < totalsize; ++i)
    Ydata[i] *= Xdata[i];
}

void
sc_dmatrix_alphadotdivide (double alpha, sc_dmatrix_t * X)
{
  sc_bint_t           i;
  const sc_bint_t     totalsize = X->m * X->n;
  double             *Xdata = X->e[0];

  for (i = 0; i < totalsize; ++i)
    Xdata[i] = alpha / Xdata[i];
}

void
sc_dmatrix_copy (const sc_dmatrix_t * X, sc_dmatrix_t * Y)
{
  const sc_bint_t     totalsize = X->m * X->n;
  const double       *Xdata = X->e[0];
  double             *Ydata = Y->e[0];

  SC_ASSERT (X->m == Y->m && X->n == Y->n);

  memmove (Ydata, Xdata, totalsize * sizeof (double));
}

void
sc_dmatrix_transpose (const sc_dmatrix_t * X, sc_dmatrix_t * Y)
{
  sc_bint_t           i, j, Xrows, Xcols, Xstride, Ystride;
  double             *Ydata;
  const double       *Xdata = X->e[0];

  SC_ASSERT (X->m == Y->n && X->n == Y->m);

  Xrows = X->m;
  Xcols = X->n;
  Xstride = X->n;
  Ystride = Y->n;
  Ydata = Y->e[0];

  for (i = 0; i < Xrows; i++) {
    for (j = 0; j < Xcols; j++) {
      Ydata[j * Ystride + i] = Xdata[i * Xstride + j];
    }
  }
}

void
sc_dmatrix_add (double alpha, const sc_dmatrix_t * X, sc_dmatrix_t * Y)
{
  sc_bint_t           totalsize, inc;

  SC_ASSERT (X->m == Y->m && X->n == Y->n);

  totalsize = X->m * X->n;

  inc = 1;
  BLAS_DAXPY (&totalsize, &alpha, X->e[0], &inc, Y->e[0], &inc);
}

void
sc_dmatrix_vector (sc_trans_t transa, sc_trans_t transx, sc_trans_t transy,
                   double alpha, const sc_dmatrix_t * A,
                   const sc_dmatrix_t * X, double beta, sc_dmatrix_t * Y)
{
  sc_bint_t           inc = 1;

#ifdef SC_DEBUG
  sc_bint_t           dimX = (transx == SC_NO_TRANS) ? X->m : X->n;
  sc_bint_t           dimY = (transy == SC_NO_TRANS) ? Y->m : Y->n;
  sc_bint_t           dimX1 = (transx == SC_NO_TRANS) ? X->n : X->m;
  sc_bint_t           dimY1 = (transy == SC_NO_TRANS) ? Y->n : Y->m;

  sc_bint_t           Arows = (transa == SC_NO_TRANS) ? A->m : A->n;
  sc_bint_t           Acols = (transa == SC_NO_TRANS) ? A->n : A->m;
#endif

  SC_ASSERT (Acols != 0 && Arows != 0);
  SC_ASSERT (Acols == dimX && Arows == dimY);
  SC_ASSERT (dimX1 == 1 && dimY1 == 1);

  BLAS_DGEMV (&sc_antitranschar[transa], &A->n, &A->m, &alpha,
              A->e[0], &A->n, X->e[0], &inc, &beta, Y->e[0], &inc);
}

void
sc_dmatrix_multiply (sc_trans_t transa, sc_trans_t transb, double alpha,
                     const sc_dmatrix_t * A, const sc_dmatrix_t * B,
                     double beta, sc_dmatrix_t * C)
{
  sc_bint_t           Acols, Crows, Ccols;
#ifdef SC_DEBUG
  sc_bint_t           Arows, Brows, Bcols;

  Arows = (transa == SC_NO_TRANS) ? A->m : A->n;
  Brows = (transb == SC_NO_TRANS) ? B->m : B->n;
  Bcols = (transb == SC_NO_TRANS) ? B->n : B->m;
#endif

  Acols = (transa == SC_NO_TRANS) ? A->n : A->m;
  Crows = C->m;
  Ccols = C->n;

  SC_ASSERT (Acols == Brows && Arows == Crows && Bcols == Ccols);
  SC_ASSERT (transa == SC_NO_TRANS || transa == SC_TRANS);
  SC_ASSERT (transb == SC_NO_TRANS || transb == SC_TRANS);

  SC_ASSERT (Acols != 0 && Crows != 0 && Ccols != 0);

  BLAS_DGEMM (&sc_transchar[transb], &sc_transchar[transa], &Ccols,
              &Crows, &Acols, &alpha, B->e[0], &B->n, A->e[0], &A->n, &beta,
              C->e[0], &C->n);
}

void
sc_dmatrix_ldivide (sc_trans_t transa, const sc_dmatrix_t * A,
                    const sc_dmatrix_t * B, sc_dmatrix_t * C)
{
  sc_dmatrix_t       *BT;
  sc_trans_t          invtransa =
    (transa == SC_NO_TRANS) ? SC_TRANS : SC_NO_TRANS;

#ifdef SC_DEBUG
  sc_bint_t           A_nrows = (transa == SC_NO_TRANS) ? A->m : A->n;
  sc_bint_t           A_ncols = (transa == SC_NO_TRANS) ? A->n : A->m;
  sc_bint_t           B_nrows = B->m;
  sc_bint_t           B_ncols = B->n;
  sc_bint_t           C_nrows = C->m;
  sc_bint_t           C_ncols = C->n;
#endif

  SC_ASSERT ((C_nrows == A_ncols) && (B_nrows == A_nrows)
             && (B_ncols == C_ncols));

  BT = sc_dmatrix_new (B->n, B->m);
  sc_dmatrix_transpose (B, BT);

  sc_dmatrix_rdivide (invtransa, BT, A, BT);

  sc_dmatrix_transpose (BT, C);

  sc_dmatrix_destroy (BT);
}

void
sc_dmatrix_rdivide (sc_trans_t transb, const sc_dmatrix_t * A,
                    const sc_dmatrix_t * B, sc_dmatrix_t * C)
{
  sc_bint_t           A_nrows = A->m;
  sc_bint_t           B_nrows = (transb == SC_NO_TRANS) ? B->m : B->n;
  sc_bint_t           B_ncols = (transb == SC_NO_TRANS) ? B->n : B->m;
#ifdef SC_DEBUG
  sc_bint_t           A_ncols = A->n;
  sc_bint_t           C_nrows = C->m;
  sc_bint_t           C_ncols = C->n;
#endif
  sc_bint_t           M = B_ncols, N = B_nrows, Nrhs = A_nrows, info = 0;

  SC_ASSERT ((C_nrows == A_nrows) && (B_nrows == C_ncols)
             && (B_ncols == A_ncols));

  if (M == N) {
    sc_dmatrix_t       *lu = sc_dmatrix_clone (B);
    sc_bint_t          *ipiv = SC_ALLOC (sc_bint_t, N);

    // Perform an LU factorization of B.
    LAPACK_DGETRF (&N, &N, lu->e[0], &N, ipiv, &info);

    SC_ASSERT (info == 0);

    // Solve the linear system.
    sc_dmatrix_copy (A, C);
    LAPACK_DGETRS (&sc_transchar[transb], &N, &Nrhs, lu->e[0], &N,
                   ipiv, C->e[0], &N, &info);

    SC_ASSERT (info == 0);

    SC_FREE (ipiv);
    sc_dmatrix_destroy (lu);
  }
  else {
    SC_CHECK_ABORT (0, "Only square A's work right now\n");
  }
}

void
sc_dmatrix_write (const sc_dmatrix_t * dmatrix, FILE * fp)
{
  sc_bint_t           i, j, m, n;

  m = dmatrix->m;
  n = dmatrix->n;

  for (i = 0; i < m; ++i) {
    for (j = 0; j < n; ++j) {
      fprintf (fp, " %16.8e", dmatrix->e[i][j]);
    }
    fprintf (fp, "\n");
  }
}

sc_dmatrix_pool_t  *
sc_dmatrix_pool_new (int m, int n)
{
  sc_dmatrix_pool_t  *dmpool;

  SC_ASSERT (m >= 0 && n >= 0);

  dmpool = SC_ALLOC (sc_dmatrix_pool_t, 1);

  dmpool->m = m;
  dmpool->n = n;
  dmpool->elem_count = 0;
  sc_array_init (&dmpool->freed, sizeof (sc_dmatrix_t *));

  return dmpool;
}

void
sc_dmatrix_pool_destroy (sc_dmatrix_pool_t * dmpool)
{
  size_t              zz;
  sc_dmatrix_t      **pdm;

  SC_ASSERT (dmpool->elem_count == 0);

  for (zz = 0; zz < dmpool->freed.elem_count; ++zz) {
    pdm = sc_array_index (&dmpool->freed, zz);
    sc_dmatrix_destroy (*pdm);
  }
  sc_array_reset (&dmpool->freed);

  SC_FREE (dmpool);
}

sc_dmatrix_t       *
sc_dmatrix_pool_alloc (sc_dmatrix_pool_t * dmpool)
{
  sc_dmatrix_t       *dm;

  ++dmpool->elem_count;

  if (dmpool->freed.elem_count > 0) {
    dm = *(sc_dmatrix_t **) sc_array_pop (&dmpool->freed);
  }
  else {
    dm = sc_dmatrix_new (dmpool->m, dmpool->n);
  }

#ifdef SC_DEBUG
  sc_dmatrix_set_value (dm, -1.);
#endif

  return dm;
}

void
sc_dmatrix_pool_free (sc_dmatrix_pool_t * dmpool, sc_dmatrix_t * dm)
{
  SC_ASSERT (dmpool->elem_count > 0);
  SC_ASSERT (dm->m == dmpool->m && dm->n == dmpool->n);

  --dmpool->elem_count;

  *(sc_dmatrix_t **) sc_array_push (&dmpool->freed) = dm;
}
