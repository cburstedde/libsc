/*
  This file is part of the SC Library, version 3.
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

/** \file sc3_base.h
 */

#ifndef SC3_BASE_H
#define SC3_BASE_H

#include <sc3_config.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SC3_NOOP do { ; } while (0)

#define SC3_INT_BITS (8 * SC_SIZEOF_INT)
#define SC3_INT_HPOW (1 << (SC3_INT_BITS - 2))

#define SC3_BUFSIZE 512
#define SC3_BUFZERO(b) do { memset (b, 0, SC3_BUFSIZE); } while (0)
#define SC3_BUFCOPY(b,s) \
          do { snprintf (b, SC3_BUFSIZE, "%s", s); } while (0)

#define SC3_MALLOC(typ,nmemb) ((typ *) malloc ((nmemb) * sizeof (typ)))
#define SC3_CALLOC(typ,nmemb) ((typ *) calloc (nmemb, sizeof (typ)))
#define SC3_FREE(p) do { free (p); } while (0)
#define SC3_REALLOC(p,typ,nmemb) ((typ *) realloc (p, (nmemb) * sizeof (typ)))

#define SC3_ISPOWOF2(a) ((a) > 0 && ((a) & ((a) - 1)) == 0)

#define SC3_MIN(in1,in2) ((in1) <= (in2) ? (in1) : (in2))
#define SC3_MAX(in1,in2) ((in1) >= (in2) ? (in1) : (in2))

#define SC3_SIZET_INT(s) ((s) <= INT_MAX ? (int) s : -1)

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** Determine the highest bit position of a positive integer.
 * \param [in] a, bits  The lowest *bits* bits of *a* are examined.
 *                      Higher bits are silently assumed to be zero.
 *                      *a* and *bits* must be positive.
 * \return              We return the zero-based position from the right
 *                      of the highest 1-bit of *a*,
 *                      or -1 if *a* or *bits* non-negative.
 */
int                 sc3_highbit (int a, int bits);
int                 sc3_log2_ceil (int a, int bits);

int                 sc3_intpow (int base, int exp);
long                sc3_longpow (long base, int exp);

/** Compute a cumulative partition cut by floor (Np / P).
 * The product NP must fit into a long integer.
 * For p <= 0 we return 0 and for p >= P we return N.
 * \param [in] N       Non-negative integer to divide between P slots.
 * \param [in] P       The total number of slots, a positive integer.
 * \param [in] p       Slot number is trimmed to satisfy 0 <= p <= P.
 * \return             floor (Np / P) in long integer arithmetic or
 *                     0 if any argument is invalid.
 */
int                 sc3_intcut (int N, int P, int p);

/** Compute a cumulative partition cut by floor (Np / P).
 * The product NP must fit into a long integer.
 * For p <= 0 we return 0 and for p >= P we return N.
 * \param [in] N       Non-negative integer to divide between P slots.
 * \param [in] P       The total number of slots, a positive integer.
 * \param [in] p       Slot number is trimmed to satisfy 0 <= p <= P.
 * \return             floor (Np / P) in long integer arithmetic or
 *                     0 if any argument is invalid.
 */
long                sc3_longcut (long N, int P, int p);

char               *sc3_basename (char *path);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_BASE_H */
