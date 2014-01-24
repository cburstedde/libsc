/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2014 The University of Texas System

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

#include <sc_dset.h>

static int
size_offset_compare (const void *key, const void *array)
{
  const size_t k = *((const size_t *) key);
  const size_t *a = (const size_t *) array;
  const size_t l = a[0], h = a[1];

  if (k < l) {
    return -1;
  }
  if (k < h) {
    return 0;
  }
  return 1;
}

int
sc_dset_find_owner (sc_dset_t *dset, size_t global)
{
  int mpisize = dset->mpisize;
  sc_array_t view;
  ssize_t retval;

  if (global >= dset->offset_by_rank[dset->mpisize]) {
    return -1;
  }

  sc_array_init_data (dset->offset_by_rank, sizeof (size_t),
                      (size_t) mpisize);

  retval = sc_array_bsearch (&view, size_offset_compare);

  SC_ASSERT (retval >= 0);

  return (int) retval;
}

sc_dset_t *
sc_dset_new (MPI_Comm mpicomm, size_t num_local, size_t num_owned)
{
  sc_dset_t *dset = SC_ALLOC (sc_dset_t, 1);
  int i, mpiret, mpisize, mpirank;
  size_t *offset_by_rank;
  size_t offset, count;

  SC_ASSERT (num_local >= num_owned);

  dset->mpicomm = mpicomm;

  mpiret = MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  dset->mpisize = mpisize;

  mpiret = MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);
  dset->mpirank = mpirank;

  dset->num_local = num_local;
  dset->num_owned = num_owned;

  offset_by_rank = SC_ALLOC (size_t, mpisize + 1);

  mpiret = MPI_Allgather (&num_owned, sizeof (size_t), MPI_BYTE, offset_by_rank,
                          offset_by_rank, sizeof (size_t), MPI_BYTE, mpicomm);
  SC_CHECK_MPI (mpiret);

  dset->offset_by_rank = offset_by_rank;


  offset = 0;
  for (i = 0; i < mpirank; i++) {
    count = offset_by_rank[i];
    offset_by_rank[i] = offset;
    offset += count;
  }
  offset_by_rank[mpisize] = offset;
  dset->offset = offset_by_rank[mpirank];
  dset->not_owned = SC_ALLOC (size_t, num_local - num_owned);
  dset->sharers = sc_array_new (sizeof (sc_dset_sharer_t));

  return dset;
}

static void
sc_dset_sharer_reset (sc_dset_sharer_t *sharer)
{
  sc_array_reset (&sharer->shared);
}

void
sc_dset_destroy (sc_dset_t *dset)
{
  sc_array_t *sharers = dset->sharers;
  size_t zz, count = dset->sharers->elem_count;

  SC_FREE (dset->offset_by_rank);
  SC_FREE (dset->not_owned);

  for (zz = 0; zz < count; zz++) {
    sc_dset_sharer_t *sharer;

    sharer = sc_dset_sharers_index (sharers, zz);
    sc_dset_sharer_reset (sharer);
  }
  sc_array_destroy (dset->sharers);

  SC_FREE (dset);
}
