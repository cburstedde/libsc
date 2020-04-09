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

#ifndef SC_UINT128_H
#define SC_UINT128_H

/** \file sc_uint128.h
 *
 * Routines for managing unsigned 128 bit integer.
 *
 * \ingroup sc
 */

#include <sc.h>

/** An unsigned 128 bit integer represented as two uint64_t. */
typedef struct sc_uint128
{
  uint64_t            high_bits;        /**< The more significant 64 bits. */
  uint64_t            low_bits;         /**< The less significant 64 bits. */
}
sc_uint128_t;

/** Compare the sc_uint128_t \a a and the sc_uint128_t \a b.
 * \param [in]  a A pointer to a sc_uint128_t.
 * \param [in]  b A pointer to a sc_uint128_t.
 * \return        Returns -1 if a < b,
 *                         1 if a > b and
 *                         0 if a == b.
 */
int                 sc_uint128_compare (const void *a, const void *b);

/** Checks if the sc_uint128_t \a a and the sc_uint128_t \a b are equal.
 * \param [in]  a A pointer to a sc_uint128_t.
 * \param [in]  b A pointer to a sc_uint128_t.
 * \return        Returns a true value if \a a and \a b are equal,
 *                false otherwise.
 */
int                 sc_uint128_is_equal (const sc_uint128_t * a,
                                         const sc_uint128_t * b);

/** Initializes an unsigned 128 bit integer to a given value.
 * \param [in,out] a        A pointer to the sc_uint128_t that will be 
 *                          initialized.
 * \param [in]     high     The given high bits to intialize \a a.
 * \param [in]     low      The given low bits to initialize \a a.
 */
void                sc_uint128_init (sc_uint128_t * a,
                                     uint64_t high, uint64_t low);

/** Sets the exponent-th bit of \a a to one.
 * This function modifies an existing, initialized value.
 * \param [in,out] a        A pointer to a sc_uint128_t.
 * \param[in]      exponent The bit (counted from the right hand side)
 *                          that is set to one by logical or.
 *                          0 <= \a exponent < 128.
 */
void                sc_uint128_bitwise_pow2 (sc_uint128_t * a, int exponent);

/** Copies an initialized sc_uint128_t to a sc_uint128_t.
 * \param [in]     input    A pointer to the sc_uint128 that is copied.
 * \param [in,out] output   A pointer to a sc_uint128_t.
 *                          The high and low bits of \a output will
 *                          be set to the high and low bits of
 *                          \a input, respectively.
 */
void                sc_uint128_copy (const sc_uint128_t * input,
                                     sc_uint128_t * output);

/** Adds the uint128_t \a b to the uint128_t \a a.
 * \a result == \a a or \a result == \a b is not allowed.
 * \a a == \a b is allowed.
 * \param [in]  a       A pointer to a sc_uint128_t.
 * \param [in]  b       A pointer to a sc_uint128_t.
 * \param[out]  result  A pointer to a sc_uint128_t.
 *                      The sum \a a + \a b will be saved in \a result.
 */
void                sc_uint128_add (const sc_uint128_t * a,
                                    const sc_uint128_t * b,
                                    sc_uint128_t * result);

/** Substracts the uint128_t \a b from the uint128_t \a a.
 * This function assumes that the result is >= 0.
 * \a result == \a a or \a result == \a b is not allowed.
 * \a a == \a b is allowed.
 * \param [in]  a       A pointer to a sc_uint128_t.
 * \param [in]  b       A pointer to a sc_uint128_t.
 * \param[out]  result  A pointer to a sc_uint128_t.
 *                      The difference \a a - \a b will be saved in \a result.
 */
void                sc_uint128_sub (const sc_uint128_t * a,
                                    const sc_uint128_t * b,
                                    sc_uint128_t * result);

/** Calculates the bitwise negation of the uint128_t \a a.
 * \a a == \a result is allowed.
 * \param[in]  a        A pointer to a sc_uint128_t.
 * \param[out] result   A pointer to a sc_uint128_t.
 *                      The bitwise negation of \a a will be saved in 
 *                      \a result.
 */
void                sc_uint128_bitwise_neg (const sc_uint128_t * a,
                                            sc_uint128_t * result);

/** Calculates the bitwise or of the uint128_t \a a and \a b.
 * \a a == \a result is allowed. Furthermore, \a a == \a result
 * and/or \a b == \a result is allowed.
 * \param[in]  a        A pointer to a sc_uint128_t.
 * \param[in]  b        A pointer to a sc_uint128_t.
 * \param[out] result   A pointer to a sc_uint128_t.
 *                      The bitwise or of \a a and \a b will be
 *                      saved in \a result.
 */
void                sc_uint128_bitwise_or (const sc_uint128_t * a,
                                           const sc_uint128_t * b,
                                           sc_uint128_t * result);

/** Calculates the bitwise and of the uint128_t \a a and the uint128_t \a b.
 * \a a == \a result is allowed. Furthermore, \a a == \a result
 * and/or \a b == \a result is allowed.
 * \param [in]  a       A pointer to a sc_uint128_t.
 * \param [in]  b       A pointer to a sc_uint128_t.
 * \param[out]  result  A pointer to a sc_uint128_t.
 *                      The bitwise and of \a a and \a b will be saved.
 *                      in \a result.
 */
void                sc_uint128_bitwise_and (const sc_uint128_t * a,
                                            const sc_uint128_t * b,
                                            sc_uint128_t * result);

/** Calculates the bit right shift of uint128_t \a input by shift_count bits.
 * We shift in zeros from the left. If \a shift_count >= 128, \a result is 0.
 * All bits right from the zeroth bit (counted from the right hand side) 
 * drop out. \a input == \a result is allowed.
 * \param [in]      input       A pointer to a sc_uint128_t.
 * \param [in]      shift_count Bits to shift. \a shift_count >= 0.
 * \param [in,out]  result      A pointer to a sc_uint128_t.
 *                              The right shifted number will be saved 
 *                              in \a result.
 */
void                sc_uint128_shift_right (const sc_uint128_t * input,
                                            int shift_count,
                                            sc_uint128_t * result);

/** Calculates the bit left shift of uint128_t \a input by shift_count bits.
 * We shift in zeros from the right. If \a shift_count >= 128, \a result is 0.
 * All bits left from the 127th bit (counted zero based from the right
 * hand side) drop out. \a input == \a result is allowed.
 * \param [in]      input       A pointer to a sc_uint128_t.
 * \param [in]      shift_count Bits to shift. \a shift_count >= 0.
 * \param [in,out]  result      A pointer to a sc_uint128_t.
 *                              The left shifted number will be saved 
 *                              in \a result.
 */
void                sc_uint128_shift_left (const sc_uint128_t * input,
                                           int shift_count,
                                           sc_uint128_t * result);

/** Adds the uint128 \a b to the uint128_t \a a.
 * The result is saved in \a a. \a a == \a b is allowed.
 * \param [in, out] a   A pointer to a sc_uint128_t. \a a
 *                      will be overwritten by \a a + \a b.
 * \param [in]      b   A pointer to a sc_uint128_t.
 */
void                sc_uint128_add_inplace (sc_uint128_t * a,
                                            const sc_uint128_t * b);

/** Substracts the uint128_t \a b from the uint128_t \a a.
 * The result is saved in \a a. \a a == \a b is allowed.
 * This function assumes that the result is >= 0.
 * \param [in,out]  a   A pointer to a sc_uint128_t.
 *                      \a a will be overwritten by \a a - \a b.
 * \param [in]      b   A pointer to a sc_uint128_t.
 */
void                sc_uint128_sub_inplace (sc_uint128_t * a,
                                            const sc_uint128_t * b);

/** Calculates the bitwise or of the uint128_t \a a and the uint128_t \a b.
 * \a a == \a b is allowed.
 * \param [in,out]  a   A pointer to a sc_uint128_t.
 *                      The bitwise or will be saved in \a a.
 * \param [in]      b   A pointer to a sc_uint128_t.
 */
void                sc_uint128_bitwise_or_inplace (sc_uint128_t * a,
                                                   const sc_uint128_t * b);

/** Calculates the bitwise and of the uint128_t \a a and the uint128_t \a b.
 * \a a == \a b is allowed.
 * \param [in,out]  a   A pointer to a sc_uint128_t.
 *                      The bitwise and will be saved in \a a.
 * \param [in]      b   A pointer to a sc_uint128_t.
 */
void                sc_uint128_bitwise_and_inplace (sc_uint128_t * a,
                                                    const sc_uint128_t * b);

#endif /* !SC_UINT128_H */
