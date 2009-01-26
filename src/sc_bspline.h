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
  int                 p; /** Number of control points is p+1 */
  int                 m; /** Number of knots is m+1 */
  int                 n; /** Degree is n = m - p - 1 */
  int                 b; /** Number of identical boundary knots b = n + 1 */
  int                 l; /** Number of internal intervals l = m - 2 * n */
  sc_dmatrix_t       *points;   /* (p+1) x d array of points, not owned */
  sc_dmatrix_t       *knots;    /* (m+1) x 1 array of knots, not owned */
}
sc_bspline_t;

/** Create a new B-spline structure.
 * \param [in] points   (p+1) x d array of points in R^d.
 * \param [in] knots    (m+1) x 1 array of knots in R.
 */
sc_bspline_t       *sc_bspline_new (sc_dmatrix_t * points,
                                    sc_dmatrix_t * knots);

/** Destroy a B-spline structure.
 * The points and knots arrays are left alone.
 */
void                sc_bspline_destroy (sc_bspline_t * bs);

SC_EXTERN_C_END;

#endif /* !SC_BSPLINE_H */
