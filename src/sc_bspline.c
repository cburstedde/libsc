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

int
sc_bspline_min_number_points (int n)
{
  return 2 * n + 2;
}

double             *
sc_bspline_knots_uniform (int n, sc_dmatrix_t * points)
{
  const int           d = points->n;
  const int           p = points->m - 1;
  const int           m = n + p + 1;
  const int           b = n + 1;
  const int           l = m - 2 * n;
  int                 i;
  double             *knots;

  SC_ASSERT (n >= 0 && m >= 1 && d >= 1 && l >= 1);

  knots = SC_ALLOC (double, m + 1);

  for (i = 0; i < b; ++i) {
    knots[i] = 0.;
    knots[m - i] = 1.;
  }
  for (i = 0; i < l; ++i) {
    knots[b + i] = (i + 1) / (double) l;
  }

  return knots;
}

sc_bspline_t       *
sc_bspline_new (int n, sc_dmatrix_t * points, double *knots)
{
  sc_bspline_t       *bs;

  bs = SC_ALLOC_ZERO (sc_bspline_t, 1);
  bs->d = points->n;
  bs->p = points->m - 1;
  bs->n = n;
  bs->m = bs->n + bs->p + 1;
  bs->b = bs->n + 1;
  bs->l = bs->m - 2 * bs->n;

  SC_ASSERT (n >= 0 && bs->m >= 1 && bs->d >= 1 && bs->l >= 1);

  bs->points = points;
  if (knots == NULL) {
    bs->knots = sc_bspline_knots_uniform (n, points);
  }
  else {
    bs->knots = SC_ALLOC (double, bs->m + 1);
    memcpy (bs->knots, knots, sizeof (double) * (bs->m + 1));
  }
  bs->works = sc_dmatrix_new (bs->n * (bs->n + 1) / 2, bs->d);

  return bs;
}

void
sc_bspline_evaluate (sc_bspline_t * bs, double t, double *result)
{
  int                 i, k, n;
  int                 iguess;
  int                 toffset;
  double              t0, tm;
  double             *wfrom, *wto;

  t0 = bs->knots[0];
  tm = bs->knots[bs->m];
  SC_ASSERT (t >= t0 && t <= tm);

  iguess = (int) floor ((t - t0) / (tm - t0) * bs->l);
  iguess = bs->n + SC_MIN (iguess, bs->l - 1);

  /* SC_LDEBUGF ("Evaluate %g at guess %d\n", t, iguess); */
  SC_CHECK_ABORT ((bs->knots[iguess] <= t && t < bs->knots[iguess + 1]) ||
                  (t >= tm && iguess == bs->n + bs->l - 1),
                  "Non-uniform knot vectors not implemented yet");

  toffset = 0;
  wfrom = wto = bs->points->e[iguess - bs->n];
  for (n = bs->n; n > 0; --n) {
    wto = bs->works->e[toffset];

    /* SC_LDEBUGF ("For %d at offset %d\n", n, toffset); */
    for (i = 0; i < n; ++i) {
      const double tleft = bs->knots[iguess + i - n + 1];
      const double tright = bs->knots[iguess + i + 1];
      const double tdiff = tright - tleft;
      /* SC_LDEBUGF ("Tdiff %g %g %g\n", tleft, tright, tdiff); */
      SC_ASSERT (tdiff > 0);
      for (k = 0; k < bs->d; ++k) {
        wto[bs->d * i + k] =
          ((t - tleft) * wfrom[bs->d * (i + 1) + k] +
           (tright - t) * wfrom[bs->d * i + k]) / tdiff;
      }
    }

    wfrom = wto;
    toffset += n;
  }
  SC_ASSERT (toffset == bs->n * (bs->n + 1) / 2);

  memcpy (result, wfrom, sizeof (double) * bs->d);
}

void
sc_bspline_destroy (sc_bspline_t * bs)
{
  sc_dmatrix_destroy (bs->works);

  SC_FREE (bs->knots);
  SC_FREE (bs);
}
