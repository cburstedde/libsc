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
child_function (int a, int *result)
{
  SC3A_RETVAL (result, 0);
  SC3A_CHECK (a < 50);

  *result = a + 1;
  return NULL;
}

static sc3_error_t *
parent_function (int a, int *result)
{
  SC3A_RETVAL (result, 0);
  SC3A_CHECK (a < 100);
  SC3E (child_function (a, result));

  *result *= 3;
  return NULL;
}

static sc3_error_t *
run (int input, int *result)
{
#if 0
  sc3_allocator_t    *a;

  SC3E (sc3_allocator_new (NULL, &a));
#endif

  SC3E (parent_function (input, result));

  /* TODO: If we return before here, we will never destroy the allocator.
     This is ok if we only expect fatal errors to occur. */

#if 0
  SC3E (sc3_allocator_destroy (&a));
#endif
  return NULL;
}

int
main (int argc, char **argv)
{
  const int           inputs[3] = { 167, 84, 23 };
  int                 input, i;
  int                 result;
  int                 num_fatal;
  sc3_error_t        *e;

  num_fatal = 0;
  for (i = 0; i < 3; ++i) {
    input = inputs[i];
    e = run (input, &result);
    if (e) {
      printf ("Error return with input %d\n", input);

      if (sc3_error_is_fatal (e))
        ++num_fatal;

      /* TODO: unravel error stack and print messages */

      if (sc3_error_destroy (&e))
        ++num_fatal;
    }
    else {
      printf ("Clean execution with input %d result %d\n", input, result);
    }
  }
  printf ("Fatal errors total %d\n", num_fatal);

  return EXIT_SUCCESS;
}
