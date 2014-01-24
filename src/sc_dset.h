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

#ifndef SC_DSET_H
#define SC_DSET_H

#include <sc_containers.h>
#include <sc_mpi.h>

SC_EXTERN_C_BEGIN;

/** Communication manager for a distributed set, with each object in the set
 * being owned by a particular process, but with processes sharing objects
 * that are owned elsewhere.
 *
 * Each object has a unique global index, and the global numbering is
 * contiguous, starting from 0.
 *
 * The global indices must be consistent with the process ranks: e.g., the
 * global indices of objects owned by rank 0 are less than those owned by rank
 * 1.
 *
 * Each * process assigns a unique local index to the objects that it owns and
 * shares.
 *
 * The local indices must also be contiguous, starting from 0.
 *
 * Each process numbers its own objects first, in ascending global order, then
 * the objects it does not-own, also in ascending global order.  If the map
 * from local index to global index were put into an array, it would look like
 * this:
 *
 * global_indices = [<-(owned, ascending)->|<-(not-owned, ascending)->]
 *                = [<-----(all objects shared by this process)------>]
 *
 * The array global_indices is not stored, because the global index of an
 * owned object can be computed as g = (local_index + \a offset), where \a
 * offset is the global index of the first object owned by this process.  A
 * local-to-global table is only needed for not-owned objects.
 */
typedef struct sc_dset
{
  MPI_Comm            mpicomm;
  int                 mpirank;
  int                 mpisize;
  int                 finalized;
  size_t              num_local;
  size_t              num_owned;
  size_t              offset;
  size_t             *not_owned;
  size_t             *offset_by_rank;
  sc_array_t         *sharers;
  sc_array_t          not_owned_array; /* used during construction */
}
sc_dset_t;

/** Compute a global index from a local index */
/*@unused@*/
static inline       size_t
sc_dset_local_to_global (sc_dset_t * dset, size_t lidx)
{
  size_t              owned = dset->num_owned;
  SC_ASSERT (lidx < dset->num_local);
  SC_ASSERT (dset->finalized || lidx < owned);

  return (lidx < owned) ? dset->offset + lidx : dset->not_owned[lidx - owned];
}

/** Compute a global index from another process's local index */
/*@unused@*/
static inline       size_t
sc_dset_rank_local_to_global (sc_dset_t * dset, int rank, size_t lidx)
{
  SC_ASSERT (rank >= 0 && rank < dset->mpisize);
  SC_ASSERT (lidx >= dset->offset_by_rank[rank]);
  SC_ASSERT (lidx < dset->offset_by_rank[rank + 1]);

  return dset->offset_by_rank[rank] + lidx;
}

/** Compute the owner of a global index: -1 if invalid global index */
int                 sc_dset_find_owner (sc_dset_t *dset, size_t global);

/** The structure stored in the sharers array.
 *
 * \a shared is a sorted size_t array of local indices shared by
 * \a rank.  The \a shared array has a contiguous (or empty) section of
 * objects owned by the current rank.  \a shared_mine_offset and
 * \a shared_mine_count identify this section by indexing the \a shared
 * array.  \a owned_offset and \a owned_count define the section of local
 * indices that is owned by the listed rank (the section may be empty).
 */
typedef struct sc_dset_sharer
{
  int                 rank;
  sc_array_t          shared;
  size_t              shared_mine_offset, shared_mine_count;
  size_t              owned_offset, owned_count;
}
sc_dset_sharer_t;

/* Create a dset. It is the user's responsibility to fill the
 * \a not_owned array and construct the \a sharers appropriately.
 */
sc_dset_t          *sc_dset_new (MPI_Comm mpicomm,
                                 size_t num_local,
                                 size_t num_owned);

/** Add a global index from another process
void                sc_dset_add_not_owned (sc_dset_t *set, size_t global);

/** Create a sharer and initializes the \a shared array for that sharer.  The
 * user can then fill \a shared with the local indices that are shared by \a
 * rank.
 */
sc_dset_sharer_t   *sc_dset_add_sharer (sc_dset_t *dset, rank);

void                sc_dset_finalize (sc_dset_t *dset);

/* test the validity of a dset.  Requires O(P^2) communication */
int                 sc_dset_is_valid (sc_dest_t *dset);

void                sc_dset_destroy (sc_dset_t * dset);

/** sc_dset_comm_t handles the MPI data of communication of objects whose
 * distribution is described by an sc_dset_t.
 *
 * \a send_buffers is an array of arrays: one buffer for each process to which
 * the current process sends.
 *
 * \a recv_buffers is an array of arrays.
 * \a recv_buffers[j] corresponds with dset->sharers[j]: it is the same
 * length as \a dset->sharers[j]->shared.
 */
typedef struct sc_dset_comm
{
  sc_array_t         *requests; /* MPI_Request */
  sc_array_t         *send_buffers;
  sc_array_t         *recv_buffers;
}
sc_dset_comm_t;

void                sc_dset_comm_destroy (sc_dset_comm_t * comm);

/** sc_dset_scatter_begin
 *
 * \a data is a user-defined array of arbitrary type, where each entry
 * is associated with a \a dset local index.
 * The value in \a data for an object is taken from the owning process and
 * written into the \a data array of the other processes that share it.
 * Values of \a data are not guaranteed to be correct until the \a comm
 * created by sc_dset_scatter_begin is passed to
 * sc_dset_scatter_end.
 *
 * To be memory neutral, the \a comm created by sc_dset_scatter_begin must be
 * destroyed with sc_dset_comm_destroy (it is not destroyed by
 * sc_dset_scatter_end).
 */
sc_dset_comm_t     *sc_dset_scatter_begin (sc_array_t * data,
                                           sc_dset_t * dset);

void                sc_dset_scatter_end (sc_dset_comm_t * comm);

/** Equivalent to calling sc_dset_scatter_begin directly after
 * sc_dset_scatter_end.  Use if there is no local work that can be done to
 * mask the communication cost.
 */
void                sc_dset_scatter (sc_array_t * data, sc_dset_t * dset);

/** sc_dset_gather
 *
 * \a sc_dset_gather uses the reverse of the communication pattern in \a
 * sc_set_scatter.  \a sc_set_gather assumes that the user wishes to manually
 * combine the values from sharing processors into one value in the owner's \a
 * data array.  After \a sc_dset_gather_end, the values from other processes
 * will be in \a comm->recv_buffers.
 *
 * To be memory neutral, the \a comm created by sc_dset_scatter_begin must be
 * destroyed with sc_dset_comm_destroy (it is not destroyed by
 * sc_dset_scatter_end).
 */
sc_dset_comm_t     *sc_dset_gather_begin (sc_dset_comm_t * comm);
void                sc_dset_gather_end (sc_dset_comm_t * comm);
sc_dset_comm_t     *sc_dset_gather (sc_array_t * data, sc_dset_t * dset);

/* \a sc_dset_reduce is like gather, but combines the different values for an
 * object using one the standard mpi routines */
sc_dset_comm_t     *sc_dset_reduce_begin (sc_dset_comm_t * comm);
void                sc_dset_reduce_end (sc_array_t * data,
                                        sc_dset_comm_t * comm,
                                        sc_dset_t * dset,
                                        MPI_Datatype type, MPI_Op op);
void                sc_dset_reduce (sc_array_t * data,
                                    sc_dset_comm_t * comm,
                                    MPI_Datatype type, MPI_Op op);

/** sc_dset_share_begin
 *
 * \a data is a user-defined array of arbitrary type, where each entry
 * is associated with the set local objects entry of matching index.
 * For every process that shares an entry with the current one, the value in
 * the \a data array of that process is written into a
 * \a comm->recv_buffers entry as described above.  The user can then perform
 * some arbitrary work that requires the data from all processes that share a
 * object (such as reduce, max, min, etc.).  When the work concludes, the
 * \a comm should be destroyed with sc_dset_comm_destroy.
 *
 * Values of \a data are not guaranteed to be sent, and
 * \a comm->recv_buffers entries are not guaranteed to be received until
 * the \a comm created by sc_dset_share_begin is passed to
 * sc_dset_share_end.
 */
sc_dset_comm_t     *sc_dset_share_begin (sc_array_t * data, sc_dset_t * set);
void                sc_dset_share_end (sc_dset_comm_t * comm);

/** Equivalent to calling sc_dset_share_end directly after
 * sc_dset_share_begin.  Use if there is no local work that can be
 * done to mask the communication cost.
 * \return          A fully initialized buffer that contains the received data.
 *                  After processing this data, the buffer must be freed with
 *                  sc_dset_comm_destroy.
 */
sc_dset_comm_t     *sc_dset_share (sc_array_t * data, sc_dset_comm_t * set);

/* \a sc_dset_merge is like \a sc_dset_share, but automatically combines the
 * values back into the \a data array using one of the standard mpi routines
 * */
sc_dset_comm_t     *sc_dset_merge_begin (sc_array_t * data, sc_dset_t * set);
void                sc_dset_merge_end (sc_array_t * data,
                                       sc_dset_comm_t * comm,
                                       sc_dset_t * set,
                                       MPI_Datatype type, MPI_Op op);
void                sc_dset_merge (sc_array_t * data,
                                   sc_dset_t * set,
                                   MPI_Datatype type, MPI_Op op);

/** Return a pointer to a dset_sharer array element indexed by a int.
 */
/*@unused@*/
static inline sc_dset_sharer_t *
sc_dset_sharers_index_int (sc_array_t * array, int it)
{
  SC_ASSERT (array->elem_size == sizeof (sc_dset_sharer_t));
  SC_ASSERT (it >= 0 && (size_t) it < array->elem_count);

  return (sc_dset_sharer_t *)
    (array->array + sizeof (sc_dset_sharer_t) * it);
}

/** Return a pointer to a dset_sharer array element indexed by a size_t.
 */
/*@unused@*/
static inline sc_dset_sharer_t *
sc_dset_sharers_index (sc_array_t * array, size_t it)
{
  SC_ASSERT (array->elem_size == sizeof (sc_dset_sharer_t));
  SC_ASSERT (it < array->elem_count);

  return (sc_dset_sharer_t *)
    (array->array + sizeof (sc_dset_sharer_t) * it);
}

SC_EXTERN_C_END;

#endif /* !SC_DSET_H */
