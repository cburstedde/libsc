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

#include <sc_unique_counter.h>

sc_unique_counter_t *
sc_unique_counter_new (int start_value)
{
  sc_unique_counter_t *uc;

  uc = SC_ALLOC (sc_unique_counter_t, 1);
  uc->start_value = start_value;
  uc->mempool = sc_mempool_new_zero_and_persist (sizeof (int));

  return uc;
}

void
sc_unique_counter_destroy (sc_unique_counter_t * uc)
{
  SC_ASSERT (uc->mempool->elem_count == 0);

  sc_mempool_destroy (uc->mempool);
  SC_FREE (uc);
}

size_t
sc_unique_counter_memory_used (sc_unique_counter_t * uc)
{
  return sizeof (sc_unique_counter_t) + sc_mempool_memory_used (uc->mempool);
}

int                *
sc_unique_counter_add (sc_unique_counter_t * uc)
{
  int                *counter;

  counter = (int *) sc_mempool_alloc (uc->mempool);
  if (!*counter) {
    *counter = (int) uc->mempool->elem_count;
  }
  *counter += uc->start_value - 1;
  SC_ASSERT (*counter >= uc->start_value);

  return counter;
}

void
sc_unique_counter_release (sc_unique_counter_t * uc, int *counter)
{
  SC_ASSERT (counter != NULL);
  SC_ASSERT (*counter >= uc->start_value);

  *counter -= uc->start_value - 1;

  sc_mempool_free (uc->mempool, counter);
}
