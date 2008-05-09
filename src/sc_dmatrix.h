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

/* sc.h should be included before this header file */
#include <sc_blas.h>

typedef struct sc_dmatrix
{
  double            **e;
  int                 m, n;
}
sc_dmatrix_t;

/*
 * The sc_dmatrix_new/clone functions abort on allocation errors.
 * There is no need to check the return value.
 */
sc_dmatrix_t       *sc_dmatrix_new (int m, int n);
sc_dmatrix_t       *sc_dmatrix_new_zero (int m, int n);
sc_dmatrix_t       *sc_dmatrix_clone (sc_dmatrix_t * dmatrix);

void                sc_dmatrix_reshape (sc_dmatrix_t * dmatrix, int m, int n);

void                sc_dmatrix_destroy (sc_dmatrix_t * dmatrix);

/*
 * Check matrix for symmetry.
 * The tolerance measures the absolute value of the maximum difference.
 */
bool                sc_dmatrix_is_symmetric (sc_dmatrix_t * A,
                                             double tolerance);

void                sc_dmatrix_set_zero (sc_dmatrix_t * dmatrix);
void                sc_dmatrix_set_value (sc_dmatrix_t * dmatrix,
                                          double value);

void                sc_dmatrix_scale (double alpha, sc_dmatrix_t * X);

void                sc_dmatrix_copy (sc_dmatrix_t * X, sc_dmatrix_t * Y);

/*! \brief Matrix Matrix Add (AXPY)  \c Y = alpha X + Y
 */
void                sc_dmatrix_add (double alpha, sc_dmatrix_t * X,
                                    sc_dmatrix_t * Y);

/**
 * Perform matrix-vector multiplication Y = alpha * A * X + beta * Y.
 * \param [in] transa    Transpose operation for matrix A.
 * \param [in] A         Matrix.
 * \param [in] X, Y      Column or row vectors (or one each).
 */
void                sc_dmatrix_vector (sc_trans_t transa,
                                       double alpha, sc_dmatrix_t * A,
                                       sc_dmatrix_t * X, double beta,
                                       sc_dmatrix_t * Y);

/*! \brief Matrix Matrix Multiply  \c C = alpha * A * B + beta * C
 *
 *   \param A matrix
 *   \param B matrix
 *   \param C matrix
 */
void                sc_dmatrix_multiply (sc_trans_t transa,
                                         sc_trans_t transb, double alpha,
                                         sc_dmatrix_t * A,
                                         sc_dmatrix_t * B, double beta,
                                         sc_dmatrix_t * C);

/** \brief Prints a matrix to \c fp
 *
 *   \param dmatrix Pointer to matrix to print
 *   \param fp      Pointer to file to print to
 */
void                sc_dmatrix_print (sc_dmatrix_t * dmatrix, FILE * fp);

#endif /* !SC_DMATRIX_H */
