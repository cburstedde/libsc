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

#include <sc3_refcount.h>

sc3_error_t        *
sc3_refcount_init_invalid (sc3_refcount_t * r)
{
  SC3A_CHECK (r != NULL);

  r->magic = 0;
  r->rc = 0;
  return NULL;
}

sc3_error_t        *
sc3_refcount_init (sc3_refcount_t * r)
{
  SC3A_CHECK (r != NULL);

  r->magic = SC3_REFCOUNT_MAGIC;
  r->rc = 1;
  return NULL;
}

sc3_error_t        *
sc3_refcount_ref (sc3_refcount_t * r)
{
  SC3A_CHECK (r != NULL && r->magic == SC3_REFCOUNT_MAGIC);
  SC3A_CHECK (r->rc >= 1);

  ++r->rc;
  return NULL;
}

sc3_error_t        *
sc3_refcount_unref (sc3_refcount_t * r, int *waslast)
{
  SC3A_CHECK (r != NULL && r->magic == SC3_REFCOUNT_MAGIC);
  SC3A_CHECK (r->rc >= 1);
  SC3A_RETVAL (waslast, 0);

  if (--r->rc == 0) {
    r->magic = 0;
    *waslast = 1;
  }
  return NULL;
}
