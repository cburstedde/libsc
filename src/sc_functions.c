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
sc_zero (double x, double y, double z, void *data)
{
  return 0.;
}

double
sc_one (double x, double y, double z, void *data)
{
  return 1.;
}

double
sc_two (double x, double y, double z, void *data)
{
  return 2.;
}

double
sc_ten (double x, double y, double z, void *data)
{
  return 10.;
}

double
sc_constant (double x, double y, double z, void *data)
{
  return *(double *) data;
}

double
sc_x (double x, double y, double z, void *data)
{
  return x;
}

double
sc_y (double x, double y, double z, void *data)
{
  return y;
}

double
sc_z (double x, double y, double z, void *data)
{
  return z;
}

double
sc_sum (double x, double y, double z, void *data)
{
  sc_function3_meta_t *meta = (sc_function3_meta_t *) data;

  SC_ASSERT (meta != NULL);
  return meta->f1 (x, y, z, meta->data) +
    ((meta->f2 != NULL) ? meta->f2 (x, y, z, meta->data) : meta->parameter2);
}

double
sc_product (double x, double y, double z, void *data)
{
  sc_function3_meta_t *meta = (sc_function3_meta_t *) data;

  SC_ASSERT (meta != NULL);
  return meta->f1 (x, y, z, meta->data) *
    ((meta->f2 != NULL) ? meta->f2 (x, y, z, meta->data) : meta->parameter2);
}

double
sc_tensor (double x, double y, double z, void *data)
{
  sc_function3_meta_t *meta = (sc_function3_meta_t *) data;

  SC_ASSERT (meta != NULL);
  return meta->f1 (x, y, z, meta->data) *
    meta->f2 (x, y, z, meta->data) * meta->f3 (x, y, z, meta->data);
}
