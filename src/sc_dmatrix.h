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

#ifndef SC_DMATRIX_H
#define SC_DMATRIX_H

#include <sc_blas.h>
#include <sc_containers.h>

SC_EXTERN_C_BEGIN;

typedef struct sc_dmatrix
{
  double            **e;
  sc_bint_t           m, n;
  int                 view;
}
sc_dmatrix_t;

/** Check whether a double array is free of NaN entries.
 * \param [in] darray   Array of doubles.
 * \param [in] nelem    Number of doubles in the array.
 * \return              Return false if at least one entry is NaN.
 */
int                 sc_darray_is_valid (const double *darray, size_t nelem);

/** Check whether the values in a double array are in a certain range.
 * \param [in] darray   Array of doubles.
 * \param [in] nelem    Number of doubles in the array.
 * \param [in] low      Lowest allowed value in the array.
 * \param [in] high     Highest allowed value in the array.
 * \return              Return false if at least one entry is out of range.
 */
int                 sc_darray_is_range (const double *darray, size_t nelem,
                                        double low, double high);

/** Calculate the memory used by a dmatrix.
 * \param [in] array       The dmatrix.
 * \return                 Memory used in bytes.
 */
size_t              sc_dmatrix_memory_used (sc_dmatrix_t * dmatrix);

/*
 * The sc_dmatrix_new/clone functions abort on allocation errors.
 * There is no need to check the return value.
 */
sc_dmatrix_t       *sc_dmatrix_new (sc_bint_t m, sc_bint_t n);
sc_dmatrix_t       *sc_dmatrix_new_zero (sc_bint_t m, sc_bint_t n);
sc_dmatrix_t       *sc_dmatrix_clone (const sc_dmatrix_t * dmatrix);

/** Create a matrix view on an existing data array.
 * The data array must have been previously allocated and large enough.
 * The data array must not be deallocated while the view is in use.
 */
sc_dmatrix_t       *sc_dmatrix_new_data (sc_bint_t m, sc_bint_t n,
                                         double *data);

/** Create a matrix view on an existing sc_dmatrix_t.
 * The original matrix must have greater equal as many elements as the view.
 * The original matrix must not be destroyed or resized while view is in use.
 */
sc_dmatrix_t       *sc_dmatrix_new_view (sc_bint_t m, sc_bint_t n,
                                         sc_dmatrix_t * orig);

/** Create a matrix view on an existing sc_dmatrix_t.
 * The start of the view is offset by a number of rows.
 * The original matrix must have greater equal as many elements as view end.
 * The original matrix must not be destroyed or resized while view is in use.
 * \param[in] o     Number of rows that the view is offset.
 *                  Requires (o + m) * n <= orig->m * orig->n.
 */
sc_dmatrix_t       *sc_dmatrix_new_view_offset (sc_bint_t o,
                                                sc_bint_t m, sc_bint_t n,
                                                sc_dmatrix_t * orig);

/** Reshape a matrix to different m and n without changing m * n.
 */
void                sc_dmatrix_reshape (sc_dmatrix_t * dmatrix, sc_bint_t m,
                                        sc_bint_t n);

/** Change the matrix dimensions.
 * For views it must be known that the new size is permitted.
 * For non-views the data will be realloced if necessary.
 * The entries are unchanged to the minimum of the old and new sizes.
 */
void                sc_dmatrix_resize (sc_dmatrix_t * dmatrix,
                                       sc_bint_t m, sc_bint_t n);

/** Change the matrix dimensions, while keeping the subscripts in place, i.e.
 * dmatrix->e[i][j] will have the same value before and after, as long as
 * (i, j) is still a valid subscript.
 * This is not valid for views.
 * For non-views the data will be realloced if necessary.
 * The entries are unchanged to the minimum of the old and new sizes.
 */
void                sc_dmatrix_resize_in_place (sc_dmatrix_t * dmatrix,
                                                sc_bint_t m, sc_bint_t n);

/** Destroy a dmatrix and all allocated memory */
void                sc_dmatrix_destroy (sc_dmatrix_t * dmatrix);

/** Check whether a dmatrix is free of NaN entries.
 * \return          true if the dmatrix does not contain any NaN entries.
 */
int                 sc_dmatrix_is_valid (const sc_dmatrix_t * A);

/** Check a square dmatrix for symmetry.
 * \param [in] tolerance    measures the absolute value of the max difference.
 * \return                  true if matrix is numerically symmetric.
 */
int                 sc_dmatrix_is_symmetric (const sc_dmatrix_t * A,
                                             double tolerance);

void                sc_dmatrix_set_zero (sc_dmatrix_t * dmatrix);
void                sc_dmatrix_set_value (sc_dmatrix_t * dmatrix,
                                          double value);

/** Perform element-wise multiplication with a scalar, X := alpha .* X.
 */
void                sc_dmatrix_scale (double alpha, sc_dmatrix_t * X);

/** Perform element-wise addition with a scalar, X := X + alpha.
 */
void                sc_dmatrix_shift (double alpha, sc_dmatrix_t * X);

/** Perform element-wise divison with a scalar, X := alpha ./ X.
 */
void                sc_dmatrix_alphadivide (double alpha, sc_dmatrix_t * X);

/** Perform element-wise exponentiation with a scalar, X := X ^ alpha.
 */
void                sc_dmatrix_pow (double exponent, sc_dmatrix_t * X);

/** Perform element-wise absolute value, Y := fabs(X).
 */
void                sc_dmatrix_fabs (const sc_dmatrix_t * X,
                                     sc_dmatrix_t * Y);

/** Perform element-wise square root, Y := sqrt(X).
 */
void                sc_dmatrix_sqrt (const sc_dmatrix_t * X,
                                     sc_dmatrix_t * Y);

/** Extract the element-wise sign of a matrix, Y := (X >= 0 ? 1 : -1)
 */
void                sc_dmatrix_getsign (const sc_dmatrix_t * X,
                                        sc_dmatrix_t * Y);

/** Compare a matrix element-wise against a bound, Y := (X >= bound ? 1 : 0)
 */
void                sc_dmatrix_greaterequal (const sc_dmatrix_t * X,
                                             double bound, sc_dmatrix_t * Y);

/** Compare a matrix element-wise against a bound, Y := (X <= bound ? 1 : 0)
 */
void                sc_dmatrix_lessequal (const sc_dmatrix_t * X,
                                          double bound, sc_dmatrix_t * Y);

/** Assign element-wise maximum, Y_i := (X_i > Y_i ? X_i : Y_i)
 */
void                sc_dmatrix_maximum (const sc_dmatrix_t * X,
                                        sc_dmatrix_t * Y);

/** Assign element-wise minimum, Y_i := (X_i < Y_i ? X_i : Y_i)
 */
void                sc_dmatrix_minimum (const sc_dmatrix_t * X,
                                        sc_dmatrix_t * Y);

/** Perform element-wise multiplication, Y := Y .* X.
 */
void                sc_dmatrix_dotmultiply (const sc_dmatrix_t * X,
                                            sc_dmatrix_t * Y);

/** Perform element-wise division, Y := Y ./ X.
 */
void                sc_dmatrix_dotdivide (const sc_dmatrix_t * X,
                                          sc_dmatrix_t * Y);

void                sc_dmatrix_copy (const sc_dmatrix_t * X,
                                     sc_dmatrix_t * Y);

void                sc_dmatrix_transpose (const sc_dmatrix_t * X,
                                          sc_dmatrix_t * Y);

/*! \brief Matrix Matrix Add (AXPY)  \c Y := alpha X + Y
 */
void                sc_dmatrix_add (double alpha, const sc_dmatrix_t * X,
                                    sc_dmatrix_t * Y);

/**
 * Perform matrix-vector multiplication Y = alpha * A * X + beta * Y.
 * \param [in] transa    Transpose operation for matrix A.
 * \param [in] transx    Transpose operation for matrix X.
 * \param [in] transy    Transpose operation for matrix Y.
 * \param [in] A         Matrix.
 * \param [in] X, Y      Column or row vectors (or one each).
 */
void                sc_dmatrix_vector (sc_trans_t transa,
                                       sc_trans_t transx,
                                       sc_trans_t transy,
                                       double alpha, const sc_dmatrix_t * A,
                                       const sc_dmatrix_t * X, double beta,
                                       sc_dmatrix_t * Y);

/*! \brief Matrix Matrix Multiply  \c C := alpha * A * B + beta * C
 *
 *   \param A matrix
 *   \param B matrix
 *   \param C matrix
 */
void                sc_dmatrix_multiply (sc_trans_t transa,
                                         sc_trans_t transb, double alpha,
                                         const sc_dmatrix_t * A,
                                         const sc_dmatrix_t * B, double beta,
                                         sc_dmatrix_t * C);

/** \brief Left Divide \c A \ \c B.
 * The matrices cannot have 0 rows or columns.
 * Solves  \c A \c C = \c B or \c A' \c C = \c B.
 *
 *   \param transa Use the transpose of \c A
 *   \param A matrix
 *   \param B matrix
 *   \param C matrix
 */
void                sc_dmatrix_ldivide (sc_trans_t transa,
                                        const sc_dmatrix_t * A,
                                        const sc_dmatrix_t * B,
                                        sc_dmatrix_t * C);

/** \brief Right Divide \c A / \c B.
 * The matrices cannot have 0 rows or columns.
 * Solves  \c A = \c C \c B or \c A = \c C \c B'.
 *
 *   \param transb Use the transpose of \c B
 *   \param A matrix
 *   \param B matrix
 *   \param C matrix
 */
void                sc_dmatrix_rdivide (sc_trans_t transb,
                                        const sc_dmatrix_t * A,
                                        const sc_dmatrix_t * B,
                                        sc_dmatrix_t * C);

/** \brief Writes a matrix to an opened stream.
 *
 *   \param dmatrix Pointer to matrix to write
 *   \param fp      Pointer to file to write to
 */
void                sc_dmatrix_write (const sc_dmatrix_t * dmatrix,
                                      FILE * fp);

/*
 * The sc_dmatrix_pool recycles matrices of the same size.
 */
typedef struct sc_dmatrix_pool
{
  int                 m, n;
  size_t              elem_count;
  sc_array_t          freed;    /* buffers the freed elements */
}
sc_dmatrix_pool_t;

/** Create a new dmatrix pool.
 * \param [in] m    Row count of the stored matrices.
 * \param [in] n    Column count of the stored matrices.
 * \return          Returns a dmatrix pool that is ready to use.
 */
sc_dmatrix_pool_t  *sc_dmatrix_pool_new (int m, int n);

/** Destroy a dmatrix pool.
 * This will also destroy all matrices stored for reuse.
 * Requires all allocated matrices to be returned to the pool previously.
 * \param [in]      The dmatrix pool to destroy.
 */
void                sc_dmatrix_pool_destroy (sc_dmatrix_pool_t * dmpool);

/** Allocate a dmatrix from the pool.
 * Reuses a matrix previously returned to the pool, or allocated a fresh one.
 * \param [in] pool   The dmatrix pool to use.
 * \return            Returns a dmatrix of size pool->m by pool->n.
 */
sc_dmatrix_t       *sc_dmatrix_pool_alloc (sc_dmatrix_pool_t * dmpool);

/** Return a dmatrix to the pool.
 * The matrix is stored internally for reuse and not freed in this function.
 * \param [in] pool   The dmatrix pool to use.
 * \param [in] dm     The dmatrix pool to return to the pool.
 */
void                sc_dmatrix_pool_free (sc_dmatrix_pool_t * dmpool,
                                          sc_dmatrix_t * dm);

SC_EXTERN_C_END;

#endif /* !SC_DMATRIX_H */
