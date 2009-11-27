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

#include <sc_functions.h>

double
sc_function1_invert (sc_function1_t func, void *data,
                     double x_low, double x_high, double y, double rtol)
{
  const int           k_max = 100;
  int                 k;
  double              x, sign;
  double              y_target, y_low, y_high, y_tol;

  SC_ASSERT (x_low < x_high && rtol > 0.);

  y_target = y;
  if (func == NULL)
    return y_target;

  y_low = func (x_low, data);
  y_high = func (x_high, data);
  y_tol = rtol * fabs (y_high - y_low);
  sign = (y_low <= y_high) ? 1. : -1.;

  SC_ASSERT ((sign > 0. && y_low <= y_target && y_target <= y_high) ||
             (sign < 0. && y_high <= y_target && y_target <= y_low));

  for (k = 0; k < k_max; ++k) {
    x = x_low + (x_high - x_low) * (y_target - y_low) / (y_high - y_low);
    if (x <= x_low) {
      return x_low;
    }
    if (x >= x_high) {
      return x_high;
    }

    y = func (x, data);
    if (sign * (y - y_target) < -y_tol) {
      x_low = x;
      y_low = y;
    }
    else if (sign * (y - y_target) > y_tol) {
      x_high = x;
      y_high = y;
    }
    else
      return x;
  }
  SC_ABORTF ("sc_function1_invert did not converge after %d iterations", k);
}

double
sc_zero3 (double x, double y, double z, void *data)
{
  return 0.;
}

double
sc_one3 (double x, double y, double z, void *data)
{
  return 1.;
}

double
sc_two3 (double x, double y, double z, void *data)
{
  return 2.;
}

double
sc_ten3 (double x, double y, double z, void *data)
{
  return 10.;
}

double
sc_constant3 (double x, double y, double z, void *data)
{
  return *(double *) data;
}

double
sc_x3 (double x, double y, double z, void *data)
{
  return x;
}

double
sc_y3 (double x, double y, double z, void *data)
{
  return y;
}

double
sc_z3 (double x, double y, double z, void *data)
{
  return z;
}

double
sc_sum3 (double x, double y, double z, void *data)
{
  sc_function3_meta_t *meta = (sc_function3_meta_t *) data;

  SC_ASSERT (meta != NULL);
  return meta->f1 (x, y, z, meta->data) +
    ((meta->f2 != NULL) ? meta->f2 (x, y, z, meta->data) : meta->parameter2);
}

double
sc_product3 (double x, double y, double z, void *data)
{
  sc_function3_meta_t *meta = (sc_function3_meta_t *) data;

  SC_ASSERT (meta != NULL);
  return meta->f1 (x, y, z, meta->data) *
    ((meta->f2 != NULL) ? meta->f2 (x, y, z, meta->data) : meta->parameter2);
}

double
sc_tensor3 (double x, double y, double z, void *data)
{
  sc_function3_meta_t *meta = (sc_function3_meta_t *) data;

  SC_ASSERT (meta != NULL);
  return meta->f1 (x, y, z, meta->data) *
    meta->f2 (x, y, z, meta->data) * meta->f3 (x, y, z, meta->data);
}
