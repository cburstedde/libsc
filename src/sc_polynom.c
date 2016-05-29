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

#include <sc_containers.h>
#include <sc_polynom.h>

struct sc_polynom
{
  int                 degree;   /* Degree of polynom sum_i=0^degree c_i x^i */
  sc_array_t         *c;        /* Array of type double stores coefficients.
                                   It holds degree < c->elem_count. */
};

static const void  *
sc_array_index_int_const (const sc_array_t * array, int i)
{
  SC_ASSERT (i >= 0 && (size_t) i < array->elem_count);

  return (const void *) (array->array + (array->elem_size * (size_t) i));
}

#ifdef SC_ENABLE_DEBUG

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
    if (*sc_polynom_coefficient_const (p, i) != -1.) {
      return 0;
    }
  }
#endif

  return 1;
}

#endif

int
sc_polynom_degree (const sc_polynom_t * p)
{
  SC_ASSERT (sc_polynom_is_valid (p));

  return p->degree;
}

double             *
sc_polynom_coefficient (sc_polynom_t * p, int i)
{
  SC_ASSERT (sc_polynom_is_valid (p));
  SC_ASSERT (0 <= i && i <= p->degree);

  return (double *) sc_array_index_int (p->c, i);
}

const double       *
sc_polynom_coefficient_const (const sc_polynom_t * p, int i)
{
  SC_ASSERT (sc_polynom_is_valid (p));
  SC_ASSERT (0 <= i && i <= p->degree);

  return (const double *) sc_array_index_int_const (p->c, i);
}

void
sc_polynom_destroy (sc_polynom_t * p)
{
  SC_ASSERT (sc_polynom_is_valid (p));

  sc_array_destroy (p->c);
  SC_FREE (p);
}

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
  return sc_polynom_new_constant (0.);
}

sc_polynom_t       *
sc_polynom_new_constant (double c)
{
  return sc_polynom_new_from_coefficients (0, &c);
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

/*
 * prod_{0 \le i \le degree, i \ne which} (x - p_i) / (p_which - p_i)
 */
sc_polynom_t       *
sc_polynom_new_lagrange (int degree, int which, const double *points)
{
  int                 i;
  double              denom, mp, mw;
  sc_polynom_t       *p, *l;

  SC_ASSERT (0 <= degree);
  SC_ASSERT (0 <= which && which <= degree);

  /* we will need these numbers */
  denom = 1.;
  mw = points[which];

  /* begin with the unit polynom */
  p = sc_polynom_new_constant (1.);

  /* allocate memory for the linear factors */
  l = sc_polynom_new_uninitialized (1);
  *sc_polynom_coefficient (l, 1) = 1.;

  /* multiply the linear factors and update denominator */
  for (i = 0; i <= degree; ++i) {
    if (i == which) {
      continue;
    }
    *sc_polynom_coefficient (l, 0) = (mp = -points[i]);
    sc_polynom_multiply (p, l);
    denom *= mw + mp;
  }
  sc_polynom_destroy (l);

  /* divide by denominator */
  sc_polynom_scale (p, 0, 1. / denom);

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
  int                 degree;
  int                 i, j, k;
  double              sum;
  sc_polynom_t       *p;

  SC_ASSERT (sc_polynom_is_valid (q));
  SC_ASSERT (sc_polynom_is_valid (r));

  p = sc_polynom_new_uninitialized (degree = q->degree + r->degree);

  for (i = 0; i <= degree; ++i) {
    sum = 0.;
    k = SC_MIN (i, q->degree);
    for (j = SC_MAX (0, i - r->degree); j <= k; ++j) {
      sum +=
        *sc_polynom_coefficient_const (q, j) *
        *sc_polynom_coefficient_const (r, i - j);
    }
    *sc_polynom_coefficient (p, i) = sum;
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
    *sc_polynom_coefficient (p, i + 1) = -1.;
  }
#endif
  sc_array_resize (p->c, (size_t) degree + 1);
  for (i = p->degree; i < degree; ++i) {
    *sc_polynom_coefficient (p, i + 1) = 0.;
  }
  p->degree = degree;

  SC_ASSERT (sc_polynom_is_valid (p));
}

void
sc_polynom_set_constant (sc_polynom_t * p, double value)
{
  sc_polynom_set_degree (p, 0);
  *sc_polynom_coefficient (p, 0) = value;

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
  *sc_polynom_coefficient (p, exponent) += factor;

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
      *sc_polynom_coefficient (p, i) *= factor;
    }
  }
  else {
    sc_polynom_set_degree (p, degree + exponent);

    for (i = degree; i >= 0; --i) {
      *sc_polynom_coefficient (p, i + exponent) =
        factor * *sc_polynom_coefficient (p, i);
    }
    for (i = 0; i < exponent; ++i) {
      *sc_polynom_coefficient (p, i) = 0.;
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
    *sc_polynom_coefficient (Y, i) +=
      A * *sc_polynom_coefficient_const (X, i);
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

double
sc_polynom_eval (const sc_polynom_t * p, double x)
{
  int                 i;
  int                 deg;
  double              v;

  deg = sc_polynom_degree (p);
  SC_ASSERT (deg >= 0);

  v = *sc_polynom_coefficient_const (p, deg);
  for (i = deg - 1; i >= 0; --i) {
    v = x * v + *sc_polynom_coefficient_const (p, i);
  }
  return v;
}

int
sc_polynom_roots (const sc_polynom_t * p, double *roots)
{
  int                 deg;
  double              a, b, c;

  deg = sc_polynom_degree (p);
  SC_ASSERT (0 <= deg && deg <= 2);

  if (deg < 2 ||
      fabs (a = *sc_polynom_coefficient_const (p, 2)) < SC_1000_EPS) {
    /* this polynomial is at most of linear degree (up to a tolerance) */

    if (deg < 1 ||
        fabs (b = *sc_polynom_coefficient_const (p, 1)) < SC_1000_EPS) {
      /* this polynomial is a constant (up to a tolerance) */
      return 0;
    }

    /* we know now that the leading coefficient is in b and nonzero */
    c = *sc_polynom_coefficient_const (p, 0);
    roots[0] = -c / b;
    return 1;
  }
  else {
    /* solve quadratic equation */
    /* we have set variable a above and normalize the polynomial */
    b = *sc_polynom_coefficient_const (p, 1) / a;
    c = *sc_polynom_coefficient_const (p, 0) / a;

    /* reuse a variable as discriminant */
    a = .25 * b * b - c;

    /* isolate cases of less than two zeros */
    if (a < SC_1000_EPS) {
      if (a <= -SC_1000_EPS) {
        return 0;
      }
      roots[0] = -.5 * b;
      return 1;
    }

    /* return two distinct roots */
    roots[0] = -.5 * b - sqrt (a);
    roots[1] = -.5 * b + sqrt (a);
    return 2;
  }
}
