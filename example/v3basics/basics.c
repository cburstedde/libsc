/*
  This file is part of the SC Library, version 3.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

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

#include <sc3_error.h>

static sc3_error_t *
child_function (int a, int *retval)
{
  SC3A_RETVAL (retval, 0);
  SC3A_CHECK (a < 50);

  *retval = a + 1;
  return NULL;
}

static sc3_error_t *
parent_function (int a, int *retval)
{
  SC3A_RETVAL (retval, 0);
  SC3A_CHECK (a < 100);
  SC3E (child_function (a, retval));

  *retval *= 3;
  return NULL;
}

int
main (int argc, char **argv)
{
  int                 retval;
  sc3_error_t        *e;

  e = parent_function (167, &retval);
  if (e) {
    sc3_error_destroy (e);
  }

  return 0;
}
