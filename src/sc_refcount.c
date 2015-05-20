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

#include <sc_refcount.h>

static int          sc_refcount_n_active = 0;

int
sc_refcount_get_n_active (void)
{
  return sc_refcount_n_active;
}

void
sc_refcount_init (int package, sc_refcount_t * rc)
{
  rc->refcount = 1;

#ifdef SC_ENABLE_PTHREAD
  sc_package_lock (package);
#endif
  ++sc_refcount_n_active;
#ifdef SC_ENABLE_PTHREAD
  sc_package_unlock (package);
#endif
}

sc_refcount_t      *
sc_refcount_new (int package)
{
  sc_refcount_t      *rc;

  rc = SC_ALLOC (sc_refcount_t, 1);
  sc_refcount_init (package, rc);

  return rc;
}

void
sc_refcount_destroy (sc_refcount_t * rc)
{
  SC_ASSERT (rc->refcount == 0);
  SC_FREE (rc);
}

void
sc_refcount_ref (int package, sc_refcount_t * rc)
{
  SC_ASSERT (rc->refcount > 0);

#ifdef SC_ENABLE_PTHREAD
  sc_package_lock (package);
#endif
  ++rc->refcount;
#ifdef SC_ENABLE_PTHREAD
  sc_package_unlock (package);
#endif
}

int
sc_refcount_unref (int package, sc_refcount_t * rc)
{
  int                 ret;
  SC_ASSERT (rc->refcount > 0);

#ifdef SC_ENABLE_PTHREAD
  sc_package_lock (package);
#endif
  ret = --rc->refcount == 0 ? --sc_refcount_n_active, 1 : 0;
#ifdef SC_ENABLE_PTHREAD
  sc_package_unlock (package);
#endif
  return ret;
}
