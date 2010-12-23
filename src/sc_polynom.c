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

#include <sc_polynom.h>

sc_polynom_t       *
sc_polynom_new (void)
{
  const double        zero = 0.;

  return sc_polynom_new_from_coefficients (0, &zero);
}

void
sc_polynom_destroy (sc_polynom_t * p)
{
  SC_ASSERT (p->degree < (int) p->c->elem_count);

  sc_array_destroy (p->c);
  SC_FREE (p);
}

sc_polynom_t       *
sc_polynom_new_from_coefficients (int degree, const double *coefficients)
{
  int                 i;
  double             *d;
  sc_polynom_t       *p;

  SC_ASSERT (degree >= 0);

  p = SC_ALLOC (sc_polynom_t, 1);
  p->c = sc_array_new_size (sizeof (double), (size_t) degree + 1);

  for (i = 0; i <= degree; ++i) {
    d = (double *) sc_array_index_int (p->c, i);
    *d = coefficients[i];
  }

  return p;
}

void
sc_polynom_set_degree (sc_polynom_t * p, int degree)
{
  int                 i;

  SC_ASSERT (degree >= 0);

#ifdef SC_DEBUG
  for (i = degree; i < p->degree; ++i) {
    *((double *) sc_array_index_int (p->c, i + 1)) = 0. / 0.;
  }
#endif
  sc_array_resize (p->c, (size_t) degree + 1);
  for (i = p->degree; i < degree; ++i) {
    *((double *) sc_array_index_int (p->c, i + 1)) = 0.;
  }
  p->degree = degree;
}

void
sc_polynom_set_value (sc_polynom_t * p, double value)
{
  sc_polynom_set_degree (p, 0);

  *((double *) sc_array_index (p->c, 0)) = value;
}

void
sc_polynom_scale (sc_polynom_t * p, int exponent, double factor)
{
  const int           degree = p->degree;
  int                 i;

  SC_ASSERT (exponent >= 0);

  if (exponent == 0) {
    for (i = 0; i <= degree; ++i) {
      *((double *) sc_array_index_int (p->c, i)) *= factor;
    }
  }
  else {
    sc_polynom_set_degree (p, degree + exponent);

    for (i = degree; i >= 0; --i) {
      *((double *) sc_array_index_int (p->c, i + exponent)) =
        factor * *((double *) sc_array_index_int (p->c, i));
    }
    for (i = 0; i < exponent; ++i) {
      *((double *) sc_array_index_int (p->c, i)) = 0.;
    }
  }
}
