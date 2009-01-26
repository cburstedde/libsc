/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2009 Carsten Burstedde, Lucas Wilcox.

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

#ifndef SC_BSPLINE_H
#define SC_BSPLINE_H

#ifndef SC_H
#error "sc.h should be included before this header file"
#endif

#include <sc_dmatrix.h>

SC_EXTERN_C_BEGIN;

typedef struct
{
  int                 d; /** Dimensionality of control points */
  int                 p; /** Number of control points is p + 1 */
  int                 n; /** Polynomial degree is n >= 0 */
  int                 m; /** Number of knots is m + 1 =  n + p + 2 */
  int                 b; /** Number of identical boundary knots b = n + 1 */
  int                 l; /** Number of internal intervals l = m - 2 * n > 0 */
  sc_dmatrix_t       *points;   /* (p + 1) x d array of points, not owned */
  double             *knots;    /* m + 1 array of knots, not owned */
}
sc_bspline_t;

/** Compute the minimum required number of points for a certain degree.
 * \param [in] n    Polynomial degree of the spline functions, n >= 0.
 * \return          Return minimum point number 2 * n + 2.
 */
int                 sc_bspline_min_number_points (int n);

/** Create uniform B-spline knots array.
 * \param [in] n        Polynomial degree of the spline functions, n >= 0.
 * \param [in] points   (p + 1) x d array of points in R^d, p >= 0, d >= 1.
 * \return              An SC_ALLOC'ed knot array with n + p + 2 entries.
 */
double             *sc_bspline_knots_uniform (int n, sc_dmatrix_t * points);

/** Create a new B-spline structure.
 * \param [in] n        Polynomial degree of the spline functions, n >= 0.
 * \param [in] points   (p + 1) x d matrix of points in R^d, p >= 0, d >= 1.
 *                      Matrix is not copied so it must not be altered
 *                      while the B-spline structure is alive.
 * \param [in] knots    n + p + 2 double array of knots.  A copy is made.
 *                      If NULL the knots are computed equidistantly.
 */
sc_bspline_t       *sc_bspline_new (int n,
                                    sc_dmatrix_t * points, double *knots);

/** Evaluate a B-spline at a certain point.
 * \param [in] bs       B-spline structure.
 * \param [in] t        Value that must be within the range of the knots.
 * \param [out] result  The computed point in R^d is placed here.
 */
void                sc_bspline_evaluate (sc_bspline_t * bs,
                                         double t, double *result);

/** Destroy a B-spline structure.
 * The points and knots arrays are left alone.
 */
void                sc_bspline_destroy (sc_bspline_t * bs);

SC_EXTERN_C_END;

#endif /* !SC_BSPLINE_H */
