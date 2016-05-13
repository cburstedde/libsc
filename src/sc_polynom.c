/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System
  Additional copyright (C) 2011 individual authors

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

#ifdef SC_ENABLE_DEBUG

static const void  *
sc_array_index_int_const (const sc_array_t * array, int i)
{
  SC_ASSERT (i >= 0 && (size_t) i < array->elem_count);

  return (const void *) (array->array + (array->elem_size * (size_t) i));
}

static int
sc_polynom_is_valid (const sc_polynom_t * p)
{
#ifdef SC_ENABLE_DEBUG
  int                 i;
#endif

  if (p == NULL || p->degree < 0) {
    return 0;
  }
  if (p->c->elem_size != sizeof (double) ||
      p->c->elem_count <= (size_t) p->degree) {
    return 0;
  }
#ifdef SC_ENABLE_DEBUG
  for (i = p->degree + 1; i < (int) p->c->elem_count; ++i) {
    if (*(const double *) sc_array_index_int_const (p->c, i) != -1.) {
      return 0;
    }
  }
#endif

  return 1;
}

#endif

static sc_polynom_t *
sc_polynom_new_uninitialized (int degree)
{
  sc_polynom_t       *p;

  SC_ASSERT (degree >= 0);

  p = SC_ALLOC (sc_polynom_t, 1);
  p->degree = degree;
  p->c = sc_array_new_size (sizeof (double), (size_t) degree + 1);

  SC_ASSERT (sc_polynom_is_valid (p));
  return p;
}

sc_polynom_t       *
sc_polynom_new (void)
{
  const double        zero = 0.;

  return sc_polynom_new_from_coefficients (0, &zero);
}

void
sc_polynom_destroy (sc_polynom_t * p)
{
  SC_ASSERT (sc_polynom_is_valid (p));

  sc_array_destroy (p->c);
  SC_FREE (p);
}

sc_polynom_t       *
sc_polynom_new_from_coefficients (int degree, const double *coefficients)
{
  sc_polynom_t       *p;

  p = sc_polynom_new_uninitialized (degree);
  memcpy (p->c->array, coefficients, p->c->elem_size * p->c->elem_count);

  SC_ASSERT (sc_polynom_is_valid (p));
  return p;
}

sc_polynom_t       *
sc_polynom_new_from_polynom (const sc_polynom_t * q)
{
  SC_ASSERT (sc_polynom_is_valid (q));

  return sc_polynom_new_from_coefficients (q->degree, (double *) q->c->array);
}

sc_polynom_t       *
sc_polynom_new_from_shift (const sc_polynom_t * q,
                           int exponent, double factor)
{
  sc_polynom_t       *p;

  p = sc_polynom_new_from_polynom (q);
  sc_polynom_shift (p, exponent, factor);

  return p;
}

sc_polynom_t       *
sc_polynom_new_from_scale (const sc_polynom_t * q,
                           int exponent, double factor)
{
  sc_polynom_t       *p;

  p = sc_polynom_new_from_polynom (q);
  sc_polynom_scale (p, exponent, factor);

  return p;
}

sc_polynom_t       *
sc_polynom_new_from_sum (const sc_polynom_t * q, const sc_polynom_t * r)
{
  sc_polynom_t       *p;

  SC_ASSERT (sc_polynom_is_valid (q));
  SC_ASSERT (sc_polynom_is_valid (r));

  if (q->degree >= r->degree) {
    p = sc_polynom_new_from_polynom (q);
    sc_polynom_add (p, r);
  }
  else {
    p = sc_polynom_new_from_polynom (r);
    sc_polynom_add (p, q);
  }

  SC_ASSERT (sc_polynom_is_valid (p));
  return p;
}

sc_polynom_t       *
sc_polynom_new_from_product (const sc_polynom_t * q, const sc_polynom_t * r)
{
  const int           degree = q->degree + r->degree;
  const double       *qca = (const double *) q->c->array;
  const double       *rca = (const double *) r->c->array;
  int                 i, j, k;
  double              sum;
  sc_polynom_t       *p;

  SC_ASSERT (sc_polynom_is_valid (q));
  SC_ASSERT (sc_polynom_is_valid (r));

  p = sc_polynom_new_uninitialized (degree);
  for (i = 0; i <= degree; ++i) {
    sum = 0.;
    k = SC_MIN (i, q->degree);
    for (j = SC_MAX (0, i - r->degree); j <= k; ++j) {
      sum += qca[j] * rca[i - j];
    }
    *((double *) sc_array_index_int (p->c, i)) = sum;
  }

  SC_ASSERT (sc_polynom_is_valid (p));
  return p;
}

void
sc_polynom_set_degree (sc_polynom_t * p, int degree)
{
  int                 i;

  SC_ASSERT (sc_polynom_is_valid (p));
  SC_ASSERT (degree >= 0);

#ifdef SC_ENABLE_DEBUG
  for (i = degree; i < p->degree; ++i) {
    *((double *) sc_array_index_int (p->c, i + 1)) = -1.;
  }
#endif
  sc_array_resize (p->c, (size_t) degree + 1);
  for (i = p->degree; i < degree; ++i) {
    *((double *) sc_array_index_int (p->c, i + 1)) = 0.;
  }
  p->degree = degree;

  SC_ASSERT (sc_polynom_is_valid (p));
}

void
sc_polynom_set_value (sc_polynom_t * p, double value)
{
  sc_polynom_set_degree (p, 0);

  *((double *) sc_array_index (p->c, 0)) = value;

  SC_ASSERT (sc_polynom_is_valid (p));
}

void
sc_polynom_set_polynom (sc_polynom_t * p, const sc_polynom_t * q)
{
  SC_ASSERT (sc_polynom_is_valid (q));

  sc_polynom_set_degree (p, q->degree);
  sc_array_copy (p->c, q->c);

  SC_ASSERT (sc_polynom_is_valid (p));
}

void
sc_polynom_shift (sc_polynom_t * p, int exponent, double factor)
{
  SC_ASSERT (sc_polynom_is_valid (p));
  SC_ASSERT (exponent >= 0);

  if (exponent > p->degree) {
    sc_polynom_set_degree (p, exponent);
  }
  *((double *) sc_array_index_int (p->c, exponent)) += factor;

  SC_ASSERT (sc_polynom_is_valid (p));
}

void
sc_polynom_scale (sc_polynom_t * p, int exponent, double factor)
{
  const int           degree = p->degree;
  int                 i;

  SC_ASSERT (sc_polynom_is_valid (p));
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

  SC_ASSERT (sc_polynom_is_valid (p));
}

void
sc_polynom_add (sc_polynom_t * p, const sc_polynom_t * q)
{
  sc_polynom_AXPY (1., q, p);
}

void
sc_polynom_sub (sc_polynom_t * p, const sc_polynom_t * q)
{
  sc_polynom_AXPY (-1., q, p);
}

void
sc_polynom_AXPY (double A, const sc_polynom_t * X, sc_polynom_t * Y)
{
  int                 i;

  SC_ASSERT (sc_polynom_is_valid (X));

  sc_polynom_set_degree (Y, SC_MAX (Y->degree, X->degree));
  for (i = 0; i <= X->degree; ++i) {
    *((double *) sc_array_index_int (Y->c, i)) +=
      A * *((double *) sc_array_index_int (X->c, i));
  }

  SC_ASSERT (sc_polynom_is_valid (Y));
}

void
sc_polynom_multiply (sc_polynom_t * p, const sc_polynom_t * q)
{
  sc_polynom_t       *prod;

  prod = sc_polynom_new_from_product (p, q);
  sc_polynom_set_polynom (p, prod);
  sc_polynom_destroy (prod);
}
