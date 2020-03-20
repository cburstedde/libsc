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

#include <sc3_base.h>
#include <stdarg.h>
#ifdef SC_HAVE_LIBGEN_H
#include <libgen.h>
#endif

void
sc3_strcopy (char *dest, size_t size, const char *src)
{
  sc3_snprintf (dest, size, "%s", src);
}

void
sc3_snprintf (char *str, size_t size, const char *fmt, ...)
{
  int                 retval;
  va_list             ap;

  /* If there is no space, we do not access the buffer at all.
     Further down we expect it to be at least 1 byte wide */
  if (str == NULL || size == 0) {
    return;
  }

  /* Writing this function just to catch the return value.
     Avoiding -Wnoformat-truncation gcc option this way */
  va_start (ap, fmt);
  retval = vsnprintf (str, size, fmt, ap);
  if (retval < 0) {
    str[0] = '\0';
  }
  /* We do not handle truncation, since it is expected in our design. */
  va_end (ap);
}

int
sc3_highbit (int a, int bits)
{
  if (a <= 0 || bits <= 0) {
    return -1;
  }
  else if (bits == 1) {
    return 0;
  }
  else {
    const int           b2 = bits / 2;
    const int           a2 = a >> b2;

    if (a2 > 0) {
      return sc3_highbit (a2, bits - b2) + b2;
    }
    else {
      return sc3_highbit (a, b2);
    }
  }
}

int
sc3_log2_ceil (int a, int bits)
{
  if (a < 0 || bits <= 0) {
    return -1;
  }
  return sc3_highbit (a - 1, bits) + 1;
}

int
sc3_intpow (int base, int exp)
{
  int                 result = 1;

  if (exp < 0) {
    return 0;
  }
  while (exp) {
    if (exp & 1) {
      result *= base;
    }
    exp >>= 1;
    base *= base;
  }
  return result;
}

long
sc3_longpow (long base, int exp)
{
  long                result = 1;

  if (exp < 0) {
    return 0;
  }
  while (exp) {
    if (exp & 1) {
      result *= base;
    }
    exp >>= 1;
    base *= base;
  }
  return result;
}

int
sc3_intcut (int N, int P, int p)
{
  if (N <= 0 || P <= 0 || p <= 0) {
    return 0;
  }
  return p < P ? (int) (((long) N * (long) p) / P) : N;
}

long
sc3_longcut (long N, int P, int p)
{
  if (N <= 0 || P <= 0 || p <= 0) {
    return 0;
  }
  return p < P ? (N * p) / P : N;
}

char               *
sc3_basename (char *path)
{
  static char         dotstring[] = ".";
#ifndef SC_HAVE_BASENAME
  return path == NULL ? dotstring : path;
#else
  return basename (path);
#endif
}
