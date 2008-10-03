/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

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

#ifndef SC_DMATRIX_H
#define SC_DMATRIX_H

#include <sc_blas.h>

SC_EXTERN_C_BEGIN;

typedef struct sc_dmatrix
{
  double            **e;
  sc_bint_t           m, n;
  bool                view;
}
sc_dmatrix_t;

/*
 * The sc_dmatrix_new/clone functions abort on allocation errors.
 * There is no need to check the return value.
 */
sc_dmatrix_t       *sc_dmatrix_new (sc_bint_t m, sc_bint_t n);
sc_dmatrix_t       *sc_dmatrix_new_zero (sc_bint_t m, sc_bint_t n);
sc_dmatrix_t       *sc_dmatrix_clone (const sc_dmatrix_t * dmatrix);
sc_dmatrix_t       *sc_dmatrix_view (sc_bint_t m, sc_bint_t n, double *data);

void                sc_dmatrix_reshape (sc_dmatrix_t * dmatrix, sc_bint_t m,
                                        sc_bint_t n);

void                sc_dmatrix_destroy (sc_dmatrix_t * dmatrix);

/*
 * Check matrix for symmetry.
 * The tolerance measures the absolute value of the maximum difference.
 */
bool                sc_dmatrix_is_symmetric (const sc_dmatrix_t * A,
                                             double tolerance);

void                sc_dmatrix_set_zero (sc_dmatrix_t * dmatrix);
void                sc_dmatrix_set_value (sc_dmatrix_t * dmatrix,
                                          double value);

void                sc_dmatrix_scale (double alpha, sc_dmatrix_t * X);

/** Perform elementwise multiplcations Y = Y .* X.
 */
void                sc_dmatrix_dotmult (const sc_dmatrix_t * X,
                                        sc_dmatrix_t * Y);

/** Perform elementwise divison from a scalar X = alpha ./ X.
 */
void                sc_dmatrix_alphadotdivide (double alpha,
                                               sc_dmatrix_t * X);

void                sc_dmatrix_copy (const sc_dmatrix_t * X,
                                     sc_dmatrix_t * Y);

void                sc_dmatrix_transpose (const sc_dmatrix_t * X,
                                          sc_dmatrix_t * Y);

/*! \brief Matrix Matrix Add (AXPY)  \c Y = alpha X + Y
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

/*! \brief Matrix Matrix Multiply  \c C = alpha * A * B + beta * C
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

/** \brief Prints a matrix to \c fp
 *
 *   \param dmatrix Pointer to matrix to print
 *   \param fp      Pointer to file to print to
 */
void                sc_dmatrix_print (const sc_dmatrix_t * dmatrix,
                                      FILE * fp);

SC_EXTERN_C_END;

#endif /* !SC_DMATRIX_H */
