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

#ifdef SC_ENABLE_DEBUG
static int          sc_refcount_n_active = 0;
#endif

int
sc_refcount_get_n_active (void)
{
#ifdef SC_ENABLE_DEBUG
  int                 retval;

  sc_package_lock (sc_package_id);
  retval = sc_refcount_n_active;
  sc_package_unlock (sc_package_id);
  SC_ASSERT (retval >= 0);

  return retval;
#else
  return 0;
#endif
}

void
sc_refcount_init (sc_refcount_t * rc)
{
#ifdef SC_ENABLE_DEBUG
  int                 valid;
#endif

  SC_ASSERT (rc != NULL);

  rc->refcount = 1;

#ifdef SC_ENABLE_DEBUG
  sc_package_lock (sc_package_id);
  valid = sc_refcount_n_active++ >= 0;
  sc_package_unlock (sc_package_id);
#endif
  SC_ASSERT (valid);
}

sc_refcount_t      *
sc_refcount_new (void)
{
  sc_refcount_t      *rc;

  rc = SC_ALLOC (sc_refcount_t, 1);
  sc_refcount_init (rc);

  return rc;
}

void
sc_refcount_destroy (sc_refcount_t * rc)
{
  SC_ASSERT (rc != NULL);
  SC_ASSERT (rc->refcount == 0);

  SC_FREE (rc);
}

void
sc_refcount_ref (sc_refcount_t * rc)
{
  SC_ASSERT (rc != NULL);
  SC_ASSERT (rc->refcount > 0);

  ++rc->refcount;
}

int
sc_refcount_unref (sc_refcount_t * rc)
{
#ifdef SC_ENABLE_DEBUG
  int                 valid;
#endif

  SC_ASSERT (rc != NULL);
  SC_ASSERT (rc->refcount > 0);

  if (--rc->refcount == 0) {
#ifdef SC_ENABLE_DEBUG
    sc_package_lock (sc_package_id);
    valid = --sc_refcount_n_active >= 0;
    sc_package_unlock (sc_package_id);
#endif
    SC_ASSERT (valid);
    return 1;
  }
  else {
    return 0;
  }
}
