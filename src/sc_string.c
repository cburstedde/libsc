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

#include <sc_string.h>

void
sc_string_init (sc_string_t * scs)
{
  SC_ASSERT (scs != NULL);

  scs->printed = 0;
  scs->buffer[0] = '\0';
}

int
sc_string_appendf (sc_string_t * scs, const char *fmt, ...)
{
  int                 result;
  va_list             val;

  SC_ASSERT (scs != NULL);
  SC_ASSERT (0 <= scs->printed && scs->printed < SC_STRING_SIZE);

  va_start (val, fmt);
  result = sc_string_vappendf (scs, fmt, val);
  va_end (val);

  return result;
}

int
sc_string_vappendf (sc_string_t * scs, const char *fmt, va_list ap)
{
  int                 remain, result;

  SC_ASSERT (scs != NULL);
  SC_ASSERT (0 <= scs->printed && scs->printed < SC_STRING_SIZE);

  remain = SC_STRING_SIZE - scs->printed;
  if (remain == 1) {
    /* the string is full and we cannot append any more */
    return -1;
  }

  /* we print and see how many characters fit */
  result = vsnprintf (scs->buffer + scs->printed, remain, fmt, ap);
  if (result < 0 || result >= remain) {
    /* the string is full now and we cannot append any more */
    scs->printed = SC_STRING_SIZE - 1;
    return -1;
  }
  else {
    /* everything we printed has been fitted into the string */
    scs->printed += result;
    SC_ASSERT (0 <= scs->printed && scs->printed < SC_STRING_SIZE);
    return 0;
  }
}

const char         *
sc_string_get_content (sc_string_t * scs, int *length)
{
  SC_ASSERT (scs != NULL);
  SC_ASSERT (0 <= scs->printed && scs->printed < SC_STRING_SIZE);

  if (length != NULL) {
    *length = scs->printed;
  }
  return scs->buffer;
}
