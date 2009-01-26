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

#include <sc.h>
#include <sc_bspline.h>

sc_bspline_t       *
sc_bspline_new (sc_dmatrix_t * points, sc_dmatrix_t * knots)
{
  sc_bspline_t       *bs;

  SC_ASSERT (points->m >= 1 && points->n >= 1);
  SC_ASSERT (knots->m >= 2 && knots->n == 1);

  bs = SC_ALLOC_ZERO (sc_bspline_t, 1);
  bs->d = points->n;
  bs->p = points->m - 1;
  bs->m = knots->m - 1;
  bs->n = bs->m - bs->p - 1;
  bs->b = bs->n + 1;
  bs->l = bs->m - 2 * bs->n;

  SC_ASSERT (bs->n >= 0);
  SC_ASSERT (bs->l >= 1);

  bs->points = points;
  bs->knots = knots;

  return bs;
}

void
sc_bspline_destroy (sc_bspline_t * bs)
{
  SC_FREE (bs);
}
