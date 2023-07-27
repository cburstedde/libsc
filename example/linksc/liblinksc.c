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

#include <liblinksc.h>
#include <sc_io.h>

void
linksc_hello (void)
{
  static const char  *hello = "Hello, world!";
  sc_array_t         *in, *out;

  SC_GLOBAL_PRODUCTIONF ("%s\n", hello);

  in = sc_array_new_data ((void *) hello, 1, strlen (hello) + 1);
  out = sc_array_new (1);

  sc_io_encode (in, out);

  SC_GLOBAL_PRODUCTIONF ("Encoded: %s", (const char *) out->array);

  sc_array_destroy (in);
  sc_array_destroy (out);
}
