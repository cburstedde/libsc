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

#include <sc_uint128_t.h>

sc_uint128_t       *
sc_uint128_t_alloc (int package_id)
{
  sc_uint128_t       *new_number;

  new_number = (sc_uint128_t *) sc_malloc (package_id, sizeof (sc_uint128_t));
  return new_number;
}

void
sc_uint128_t_init (sc_uint128_t * input, uint64_t high, uint64_t low)
{
  input->high_bits = high;
  input->low_bits = low;
}

sc_uint128_t       *
sc_uint128_t_copy (const sc_uint128_t * input, int package_id)
{
  sc_uint128_t       *copy;

  copy = sc_uint128_t_alloc (package_id);

  SC_ASSERT (input != NULL);
  copy->high_bits = input->high_bits;
  copy->low_bits = input->low_bits;

  return copy;
}

int
sc_uint128_t_equal (const sc_uint128_t * a, const sc_uint128_t * b)
{
  return ((a->high_bits == b->high_bits) && (a->low_bits == b->low_bits));
}

int
sc_uint128_t_compare (const sc_uint128_t * a, const sc_uint128_t * b)
{
  if (a->high_bits < b->high_bits)
    return -1;
  else if (a->high_bits > b->high_bits)
    return 1;
  else if (a->low_bits == b->low_bits)
    return 0;
  else if (a->low_bits < b->low_bits)
    return -1;
  else                          /* now it is guaranteed that a->low_bits > b->low_bits holds */
    return 1;
}

void
sc_uint128_t_add_to (sc_uint128_t * a, const sc_uint128_t * b)
{
  uint64_t            temp = a->low_bits;
  a->high_bits += b->high_bits;
  a->low_bits += b->low_bits;
  if (a->low_bits < temp)
    ++a->high_bits;
}

 /* we assume result => 0 and calculate a - b */
void
sc_uint128_t_substract (const sc_uint128_t * a, const sc_uint128_t * b,
                        sc_uint128_t * result)
{
  result->high_bits = a->high_bits - b->high_bits;
  result->low_bits = a->low_bits - b->low_bits;
  if (a->low_bits < result->low_bits)
    --result->high_bits;
}

void
sc_uint128_t_bitwise_and (const sc_uint128_t * a, const sc_uint128_t * b,
                          sc_uint128_t * result)
{
  result->high_bits = a->high_bits & b->high_bits;
  result->low_bits = a->low_bits & b->low_bits;
}

void
sc_uint128_t_bitwise_or_direct (sc_uint128_t * a, const sc_uint128_t * b)
{
  a->low_bits |= b->low_bits;
  a->high_bits |= b->high_bits;
}

void
sc_uint128_t_right_shift (const sc_uint128_t * input, unsigned shift_count,
                          sc_uint128_t * result)
{
  result->high_bits = input->high_bits;
  result->low_bits = input->low_bits;
  if (shift_count == 0)
    return;

  SC_ASSERT (shift_count <= 128);
  if (shift_count >= 64) {
    result->low_bits = input->high_bits;
    result->high_bits = 0;
    result->low_bits >>= (shift_count - 64);
  }
  else {
    result->low_bits =
      (result->high_bits << (64 - shift_count)) | (input->
                                                   low_bits >> shift_count);
    result->high_bits >>= shift_count;
  }
}

void
sc_uint128_t_left_shift (const sc_uint128_t * input, unsigned shift_count,
                         sc_uint128_t * result)
{
  result->high_bits = input->high_bits;
  result->low_bits = input->low_bits;
  if (shift_count == 0)
    return;

  SC_ASSERT (shift_count <= 128);
  if (shift_count >= 64) {
    result->high_bits = input->low_bits;
    result->low_bits = 0;
    result->high_bits <<= (shift_count - 64);
  }
  else {
    result->high_bits =
      (result->high_bits << shift_count) | (input->
                                            low_bits >> (64 - shift_count));
    result->low_bits <<= shift_count;
  }
}

void
sc_uint128_t_set_1 (sc_uint128_t * input, unsigned bit_number)
{
  sc_uint128_t        shifted_one, one;

  sc_uint128_t_init (&one, 0, 1);

  sc_uint128_t_left_shift (&one, bit_number, &shifted_one);
  sc_uint128_t_bitwise_or_direct (input, &shifted_one);
}
