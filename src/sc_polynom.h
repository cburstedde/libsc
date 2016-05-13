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

#ifndef SC_POLYNOM_H
#define SC_POLYNOM_H

#include <sc.h>

SC_EXTERN_C_BEGIN;

/** Data structure is opaque */
typedef struct sc_polynom sc_polynom_t;

/** Access the degree of a polynomial.
 * It is possible that the highest coefficient is zero,
 * depending on prior manipulations of the polynom.
 * \param [in] p        This polynom is accessed.
 * \return              The degree of the polynomial.
 */
int                 sc_polynom_degree (sc_polynom_t * p);

/** Access the coefficient to a given degree.
 * \param [in] p        This polynom is accessed.
 * \param [in] i        This index must be less equal the degree.
 * \return              Pointer to the coefficient of degree \a i.
 */
double             *sc_polynom_coefficient (sc_polynom_t * p, int i);

/** Access the coefficient to a given degree.
 * \param [in] p        This polynom is accessed and cannot be modified.
 * \param [in] i        This index must be less equal the degree.
 * \return              Pointer to the coefficient of degree \a i,
 *                      whose value must not be modified.
 */
const double       *sc_polynom_coefficient_const (const sc_polynom_t * p,
                                                  int i);

/** Destroy all memory used by a polynom */
void                sc_polynom_destroy (sc_polynom_t * p);

/** Create the zero polynom. */
sc_polynom_t       *sc_polynom_new (void);

/* Alternate constructors */

/** Create the constant polynom. */
sc_polynom_t       *sc_polynom_new_constant (double c);

/** Construct a Lagrange interpolation polynomial.
 * The computation is naive, successively multiplying linear factors.
 * \param [in] degree           Must be non-negative.
 * \param [in] which            The index must be in [0, degree].
 * \param [in] points           A set of \a degree + 1 values.
 * \return                      The polynomial
 *      sum_{0 \le i \le degree, i \ne which} (x - p_i) / (p_which - p_i)
 */
sc_polynom_t       *sc_polynom_new_lagrange (int degree, int which,
                                             const double *points);

/** Create a polynom from given monomial coefficients.
 * \param[in] degree            Degree of the polynom, >= 0.
 * \param[in] coefficients      Monomial coefficients [0..degree].
 */
sc_polynom_t       *sc_polynom_new_from_coefficients (int degree,
                                                      const double
                                                      *coefficients);

/* Alternate constructors using other polynoms */

sc_polynom_t       *sc_polynom_new_from_polynom (const sc_polynom_t * q);
sc_polynom_t       *sc_polynom_new_from_scale (const sc_polynom_t * q,
                                               int exponent, double factor);
sc_polynom_t       *sc_polynom_new_from_sum (const sc_polynom_t * q,
                                             const sc_polynom_t * r);
sc_polynom_t       *sc_polynom_new_from_product (const sc_polynom_t * q,
                                                 const sc_polynom_t * r);

/* Manipulating a polynom */

/** Set degree of a polynomial.
 * If the new degree is larger than the old degree, the new coefficients
 * are set to zero.  If the new degree is smaller, the smaller set of
 * coefficients is unchanged and the others are set to zero.
 */
void                sc_polynom_set_degree (sc_polynom_t * p, int degree);

/** Set the polynomial to a constant.
 * \param [in.out] p    This polynomial will be overwritten.
 * \param [in] value    This is the constant.
 */
void                sc_polynom_set_value (sc_polynom_t * p, double value);

/** Set a polynom to another.
 * \param[in,out] p A polynom that is set to q.
 * \param[in] q     The polynom that is used as the new value for p.
 */
void                sc_polynom_set_polynom (sc_polynom_t * p,
                                            const sc_polynom_t * q);

/** Shift a polynom by (i.e., add) a monomial.
 * \param[in] exponent  Exponent of the monomial, >= 0.
 * \param[in] factor    Prefactor of the monomial.
 */
void                sc_polynom_shift (sc_polynom_t * p,
                                      int exponent, double factor);

/** Scale a polynom by a monomial.
 * \param[in] exponent  Exponent of the monomial, >= 0.
 * \param[in] factor    Prefactor of the monomial.
 */
void                sc_polynom_scale (sc_polynom_t * p,
                                      int exponent, double factor);

/** Modify a polynom by adding another.
 * \param[in,out] p     The polynom p will be set to p + q.
 * \param[in] q         The polynom that is added to p; it is not changed.
 */
void                sc_polynom_add (sc_polynom_t * p, const sc_polynom_t * q);

/** Modify a polynom by substracting another.
 * \param[in,out] p     The polynom p will be set to p - q.
 * \param[in] q         The polynom that is substracted from p; not changed.
 */
void                sc_polynom_sub (sc_polynom_t * p, const sc_polynom_t * q);

/** Perform the well-known BLAS-type operation Y := A * X + Y.
 * \param[in] A         The factor for multiplication.
 * \param[in] X         The polynom X is used as input only.
 * \param[in,out] Y     The polynom that A * X is added to in-place.
 */
void                sc_polynom_AXPY (double A, const sc_polynom_t * X,
                                     sc_polynom_t * Y);

/** Modify a polynom by multiplying another.
 * \param[in,out] p     The polynom p will be set to p * q.
 * \param[in] q         The polynom that is multiplied with p; not changed.
 */
void                sc_polynom_multiply (sc_polynom_t * p,
                                         const sc_polynom_t * q);

SC_EXTERN_C_END;

#endif /* !SC_POLYNOM_H */
