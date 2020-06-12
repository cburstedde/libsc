/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include <sc_uint128.h>

void
sc_uint128_init (sc_uint128_t * input, uint64_t high, uint64_t low)
{
  input->high_bits = high;
  input->low_bits = low;
}

void
sc_uint128_set_bit (sc_uint128_t * input, int exponent)
{
  SC_ASSERT (exponent >= 0);

  if (exponent < 64) {
    input->low_bits |= ((uint64_t) 1) << exponent;
  }
  else {
    SC_ASSERT (exponent < 128);
    input->high_bits |= ((uint64_t) 1) << (exponent - 64);
  }
}

void
sc_uint128_copy (const sc_uint128_t * input, sc_uint128_t * output)
{
  output->high_bits = input->high_bits;
  output->low_bits = input->low_bits;
}

int
sc_uint128_is_equal (const sc_uint128_t * a, const sc_uint128_t * b)
{
  return a->high_bits == b->high_bits && a->low_bits == b->low_bits;
}

int
sc_uint128_compare (const void *va, const void *vb)
{
  const sc_uint128_t *a = (sc_uint128_t *) va;
  const sc_uint128_t *b = (sc_uint128_t *) vb;

  if (a->high_bits < b->high_bits) {
    return -1;
  }
  else if (a->high_bits > b->high_bits) {
    return 1;
  }
  else if (a->low_bits < b->low_bits) {
    return -1;
  }
  else if (a->low_bits > b->low_bits) {
    return 1;
  }
  return 0;
}

void
sc_uint128_add (const sc_uint128_t * a, const sc_uint128_t * b,
                sc_uint128_t * result)
{
  SC_ASSERT (result != a && result != b);
  result->high_bits = a->high_bits + b->high_bits;
  result->low_bits = a->low_bits + b->low_bits;
  if (result->low_bits < a->low_bits) {
    ++result->high_bits;
  }
}

 /* we assume result >= 0 and calculate a - b */
void
sc_uint128_sub (const sc_uint128_t * a, const sc_uint128_t * b,
                sc_uint128_t * result)
{
  SC_ASSERT (result != a && result != b);
  result->high_bits = a->high_bits - b->high_bits;
  result->low_bits = a->low_bits - b->low_bits;
  if (a->low_bits < result->low_bits) {
    --result->high_bits;
  }
}

void
sc_uint128_bitwise_neg (const sc_uint128_t * a, sc_uint128_t * result)
{
  result->high_bits = ~a->high_bits;
  result->low_bits = ~a->low_bits;
}

void
sc_uint128_bitwise_or (const sc_uint128_t * a, const sc_uint128_t * b,
                       sc_uint128_t * result)
{
  result->high_bits = a->high_bits | b->high_bits;
  result->low_bits = a->low_bits | b->low_bits;
}

void
sc_uint128_bitwise_and (const sc_uint128_t * a, const sc_uint128_t * b,
                        sc_uint128_t * result)
{
  result->high_bits = a->high_bits & b->high_bits;
  result->low_bits = a->low_bits & b->low_bits;
}

void
sc_uint128_shift_right (const sc_uint128_t * input, int shift_count,
                        sc_uint128_t * result)
{
  SC_ASSERT (shift_count >= 0);
  if (shift_count >= 128) {
    result->high_bits = 0;
    result->low_bits = 0;
    return;
  }

  result->high_bits = input->high_bits;
  result->low_bits = input->low_bits;
  if (shift_count == 0)
    return;

  if (shift_count >= 64) {
    result->low_bits = input->high_bits;
    result->high_bits = 0;
    result->low_bits >>= (shift_count - 64);
  }
  else {
    result->low_bits =
      (result->
       high_bits << (64 - shift_count)) | (input->low_bits >> shift_count);
    result->high_bits >>= shift_count;
  }
}

void
sc_uint128_shift_left (const sc_uint128_t * input, int shift_count,
                       sc_uint128_t * result)
{
  SC_ASSERT (shift_count >= 0);
  if (shift_count >= 128) {
    result->high_bits = 0;
    result->low_bits = 0;
    return;
  }  

  result->high_bits = input->high_bits;
  result->low_bits = input->low_bits;
  if (shift_count == 0)
    return;

  if (shift_count >= 64) {
    result->high_bits = input->low_bits;
    result->low_bits = 0;
    result->high_bits <<= (shift_count - 64);
  }
  else {
    result->high_bits =
      (result->
       high_bits << shift_count) | (input->low_bits >> (64 - shift_count));
    result->low_bits <<= shift_count;
  }
}

void
sc_uint128_add_inplace (sc_uint128_t * a, const sc_uint128_t * b)
{
  uint64_t            temp = a->low_bits;
  a->high_bits += b->high_bits;
  a->low_bits += b->low_bits;
  if (a->low_bits < temp) {
    ++a->high_bits;
  }
}

void
sc_uint128_sub_inplace (sc_uint128_t * a, const sc_uint128_t * b)
{
  uint64_t            temp = a->low_bits;
  a->high_bits -= b->high_bits;
  a->low_bits -= b->low_bits;
  if (temp < a->low_bits) {
    --a->high_bits;
  }
}

void
sc_uint128_bitwise_or_inplace (sc_uint128_t * a, const sc_uint128_t * b)
{
  a->low_bits |= b->low_bits;
  a->high_bits |= b->high_bits;
}

void
sc_uint128_bitwise_and_inplace (sc_uint128_t * a, const sc_uint128_t * b)
{
  a->high_bits &= b->high_bits;
  a->low_bits &= b->low_bits;
}
