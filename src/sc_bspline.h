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
  int                 l; /** Number of internal intervals l = m - 2 * n > 0 */
  sc_dmatrix_t       *points;   /* (p + 1) x d array of points, not owned */
  sc_dmatrix_t       *knots;    /* m + 1 array of knots */
  bool                knots_owned;
  sc_dmatrix_t       *works;    /* Workspace ((n + 1) * (n + 1)) x d */
  bool                works_owned;
}
sc_bspline_t;

/** Compute the minimum required number of points for a certain degree.
 * \param [in] n    Polynomial degree of the spline functions, n >= 0.
 * \return          Return minimum point number 2 * n + 2.
 */
int                 sc_bspline_min_number_points (int n);

/** Create a uniform B-spline knot vector.
 * \param [in] n        Polynomial degree of the spline functions, n >= 0.
 * \param [in] points   (p + 1) x d array of points in R^d, p >= 0, d >= 1.
 * \return              (n + p + 2) x 1 array of knots.
 */
sc_dmatrix_t       *sc_bspline_knots_new (int n, sc_dmatrix_t * points);

/** Create a B-spline knots array roughly proportional to the arc length.
 * This works only for at least linear B-splines, n >= 1.
 * \param [in] n        Polynomial degree of the spline functions, n >= 1.
 * \param [in] points   (p + 1) x d array of points in R^d, p >= 0, d >= 1.
 * \return              (n + p + 2) x 1 array of knots.
 */
sc_dmatrix_t       *sc_bspline_knots_new_length (int n,
                                                 sc_dmatrix_t * points);

/** Create workspace for B-spline evaluation.
 * \param [in] n        Polynomial degree of the spline functions, n >= 0.
 * \param [in] d        Dimension of the control points in R^d, d >= 1.
 * \return              Workspace ((n + 1) * (n + 1)) x d.
 */
sc_dmatrix_t       *sc_bspline_workspace_new (int n, int d);

/** Create a new B-spline structure.
 * \param [in] n        Polynomial degree of the spline functions, n >= 0.
 * \param [in] points   (p + 1) x d array of points in R^d, p >= 0, d >= 1.
 *                      Borrowed, matrix is not copied so it must not be
 *                      destroyed while the B-spline structure is in use.
 * \param [in] knots    (n + p + 2) x 1 array of knots.  Borrowed.
 *                      If NULL the knots are computed equidistantly.
 * \param [in] works    Workspace ((n + 1) * (n + 1)) x d.  Borrowed.
 *                      If NULL the workspace is allocated internally.
 */
sc_bspline_t       *sc_bspline_new (int n, sc_dmatrix_t * points,
                                    sc_dmatrix_t *knots, sc_dmatrix_t *works);

/** Destroy a B-spline structure.
 */
void                sc_bspline_destroy (sc_bspline_t * bs);

/** Evaluate a B-spline at a certain point.
 * \param [in] bs       B-spline structure.
 * \param [in] t        Value that must be within the range of the knots.
 * \param [out] result  The computed point in R^d is placed here.
 */
void                sc_bspline_evaluate (sc_bspline_t * bs,
                                         double t, double *result);

/** Evaluate a B-spline derivative at a certain point.
 * \param [in] bs       B-spline structure.
 * \param [in] t        Value that must be within the range of the knots.
 * \param [out] result  The computed derivative in R^d is placed here.
 */
void                sc_bspline_derivative (sc_bspline_t * bs,
                                           double t, double *result);

SC_EXTERN_C_END;

#endif /* !SC_BSPLINE_H */
