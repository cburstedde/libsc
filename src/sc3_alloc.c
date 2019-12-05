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

#include <sc3_alloc.h>

/*** TODO implement reference counting */

struct sc3_allocator_args
{
  int                 align;
};

struct sc3_allocator
{
  int                 align;
};

sc3_error_t        *
sc3_allocator_args_new (sc3_allocator_args_t ** aar)
{
  sc3_allocator_args_t *aa;

  SC3A_CHECK (aar != NULL);

  /* TODO error-ize malloc/free functions */

  aa = SC3_MALLOC (sc3_allocator_args_t, 1);
  aa->align = SC_SIZEOF_VOID_P;

  *aar = aa;
  return NULL;
}

sc3_error_t        *
sc3_allocator_args_destroy (sc3_allocator_args_t * aa)
{
  SC3A_CHECK (aa != NULL);

  SC3_FREE (aa);
  return NULL;
}

sc3_error_t        *
sc3_allocator_args_set_align (sc3_allocator_args_t * aa, int align)
{
  SC3A_CHECK (aa != NULL);
  SC3A_CHECK (SC3_ISPOWOF2 (align));

  aa->align = align;
  return NULL;
}

sc3_allocator_t    *
sc3_allocator_new (sc3_allocator_args_t * aa)
{
  return NULL;
}

void
sc3_allocator_destroy (sc3_allocator_t * a)
{
}
