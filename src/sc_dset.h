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

/** Typedef for process-local indices. */
typedef int32_t     sc_locidx_t;
#define p4est_locidx_compare sc_int32_compare
#define SC_MPI_LOCIDX MPI_INT
#define SC_VTK_LOCIDX "Int32"
#define SC_LOCIDX_MIN INT32_MIN
#define SC_LOCIDX_MAX INT32_MAX
#define SC_LOCIDX_1   ((sc_locidx_t) 1)

/** Typedef for globally unique indices. */
typedef int64_t     sc_gloidx_t;
#define sc_gloidx_compare sc_int64_compare
#define SC_MPI_GLOIDX MPI_LONG_LONG_INT
#define SC_VTK_GLOIDX "Int64"
#define SC_GLOIDX_MIN INT64_MIN
#define SC_GLOIDX_MAX INT64_MAX
#define SC_GLOIDX_1   ((sc_gloidx_t) 1)

/** A dset is a description of a distributed index set, with each index in the
 * set being owned by a particular process, but with processes' local sets
 * overlapping indices that are owned elsewhere.
 *
 * Global indices are contiguous, starting from 0, and should be less than
 * SC_GLOIDX_MAX.
 *
 * The partitioning of global indices is consistent with the process ranks:
 * e.g., the global indices owned by rank 0 are less than those owned by rank
 * 1.
 *
 * Each process assigns a unique local index to each global index that it owns
 * or borrows.
 *
 * Local indices must also be contiguous, starting from 0, and should be less
 * than SC_LOCIDX_MAX.
 *
 * In local numbering, each process numbers its own global indices first, in
 * ascending global order, then the objects it does not own, also in ascending
 * global order.  If the map from local indices to global indices were put
 * into an array, it would look like this:
 *
 * global_indices = [<-(owned, ascending)->|<-(borrowed, ascending)->]
 *                = [<-----(all indices overlapped by this set)----->]
 *
 * The array global_indices is not stored, because the global index of an
 * owned object can be computed as global_index = (local_index + \a first_owned),
 * where \a offset is the global index of the first object owned by this
 * process.  A local-to-global table is only needed for not-owned objects.
 */
typedef struct sc_dset sc_dset_t;

/** Create a new dset from a known number of indices owned by this process.
 * This routines initializes the dset with no overlaps between processes.
 */
sc_dset_t          *sc_dset_new (MPI_Comm comm, sc_locidx_t num_local);

/** Create a new dset from a known number of global indices.
 * This routines initializes the dset with no overlaps between processes.
 */
sc_dset_t          *sc_dset_new_from_num_global (MPI_Comm comm,
                                                 sc_gloidx_t num_global);

/** Create a new dset from a known partition of global indices.
 * first_owned_by_rank should be an array of size mpisize + 1, with
 * the last entry being the number of global indices.
 */
sc_dset_t          *sc_dset_new_from_partition (MPI_Comm comm,
                                                const sc_gloidx_t
                                                * first_owned_by_rank);

/** Destroy a dset */
void                sc_dset_destroy (sc_dset_t * dset);

/** Because the partition of global indices is constant over the life of a
 * dset, information about this partition can be obtained at any time */

/** Get the number of global indices */
sc_gloidx_t         sc_dset_num_global (sc_dset_t * dset);

/** Get the number of owned indices */
sc_locidx_t         sc_dset_num_owned (sc_dset_t * dset);

/** Get the first global index owned by this rank */
sc_gloidx_t         sc_dset_first_owned (sc_dset_t * dset);

/** Get the number of indices owned by another process */
sc_locidx_t         sc_dset_num_owned_by_rank (sc_dset_t * dset, int rank);

/** Get the first global index owned by another process */
sc_locidx_t         sc_dset_first_owned_by_rank (sc_dset_t * dset, int rank);

/** Compute a global index from any process's local index.  The returned value
 * is only valid if the resulting global index is owned by \a rank, otherwise
 * returns -1. O(1) */
sc_gloidx_t         sc_dset_rank_local_to_global (sc_dset_t * dset, int rank,
                                                  sc_locidx_t lidx);

/** Compute the owner of a global index. Returns -1 if gloidx is not a valid
 * global index O(log(P)) */
int                 sc_dset_find_owner (sc_dset_t * dset, sc_gloidx_t gloidx);

/** Return true if a global index is owned by the current process. O(1) */
int                 sc_dset_is_owned (sc_dset_t * dset, sc_gloidx_t gloidx);

/** Return true if \a rank owns the global index. O(1) */
int                 sc_dset_is_owned_by_rank (sc_dset_t * dset,
                                              sc_gloidx_t gloidx, int rank);

/** Returns true if the dset is valid for scatter/gather/reduce communication,
 * where the owner of a global index can send/receive data associated with it
 * to/from all other overlapping processes.  Must be called on all processes.
 */
int                 sc_dset_is_valid_gather (sc_dset_t * dset);

/** Returns true if the dset is valid for allgather/allreduce communication,
 * where all processes overlapping an index can share with each other data
 * associated with it.  Must be called on all processes.
 */
int                 sc_dset_is_valid_allgather (sc_dset_t * dset);

/** For a dset to be valid for a mode of communication, the description of the
 * overlaps must be consistent between all processes, i.e. if process i
 * believes that it borrows node j from process k, then process k must believe
 * that is lends node j to process i.  Enforcing this type of synchronization
 * can be expensive, so it is not enforced unless the user asks for it.
 *
 * In any case, dset construction should be begin as follows:
 *
 *     dset = sc_dset_new (comm, num_local);
 *     sc_dset_setup_begin (dset);
 *
 *     // multiple calls to sc_dset_add_overlap (dset, gloidx, rank);
 *     //                or sc_dset_add_overlaps_one_rank ()
 *     //                or sc_dset_add_overlaps_one_index ()
 *     // ...
 *
 * then, if the user needs to synchronize the information,
 *
 *     sc_dset_sync_gather (dset); // or sc_dset_sync_allgather (dset);
 *                                 // these routines use sc_notify to
 *                                 // synchronize the overlaps described
 *                                 // on all processes
 *
 * finally, exit setup
 *
 *     sc_dset_setup_end (dset);
 *   #ifdef DEBUG
 *     sc_dset_setup_end (dset);
 *     assert (sc_dset_is_valid_gather (dset));
 *     //   or sc_dset_is_valid_allgather (dset)
 *   #endif
 */

/** Begin modification of the dsets overlap information */
void                sc_dset_setup_begin (sc_dset_t * dset);

/** Add an overlap: a global index that is owned by the current process and
 * borrowed by \a rank, one that is owned by \a rank and borrowed by the
 * current process, or one that is owned by neither and borrowed by both
 */
void                sc_dset_add_overlap (sc_dset_t * dset,
                                         sc_gloidx_t gloidx, int rank);

/** Add multiple overlapping indices with one rank */
void                sc_dset_add_overlaps_one_rank (sc_dset_t * dset,
                                                   sc_locidx_t num_indices,
                                                   sc_gloidx_t * gloidx,
                                                   int rank);

/** Add multiple overlapping ranks with one index */
void                sc_dset_add_overlaps_one_index (sc_dset_t * dset,
                                                    sc_locidx_t num_ranks,
                                                    sc_gloidx_t gloidx,
                                                    int *ranks);

/** This routine synchronizes the overlaps on all processes so that
 * scatter/gather/reduce communication is possible.  If libsc is configured
 * --with-debug, sc_dset_is_valid_gather (dset) is confirmed. */
void                sc_dset_sync_gather (sc_dset_t * dset);

/** This routine synchronizes the overlaps on all processes so that
 * allgather/allreduce communication is possible.  If libsc is configured
 * --with-debug, sc_dset_is_valid_allgather (dset) is confirmed. */
void                sc_dset_sync_allgather (sc_dset_t * dset);

/** End modification of the dset overlap information. */
void                sc_dset_setup_end (sc_dset_t * dset);

/** After setup, information about the nature of overlapping indices is
 * available */

/** The total number of indices overlapped by this process: the number owned
 * plus the number borrowed */
sc_locidx_t         sc_dset_num_local (sc_dset_t * dset);

/** The number of borrowed indices: only valid if not in set up. */
sc_locidx_t         sc_dset_num_borrowed (sc_dset_t * dset);

/** The number of indices owned by this process that are lent with other
 * processes */
sc_locidx_t         sc_dset_num_lent (sc_dset_t * dset);

/** Returen the global indices of a local index.  If lidx is less than the
 * number owned by this process, then the return value is valid even during
 * setup.  Returns -1 if lidx is greater than the number of local nodes. If
 * lidx is greater than the number owned by this process, then it returns the
 * (lidx - num_owned)th global node borrowed by this process. O(1)
 */
sc_gloidx_t         sc_dset_local_to_global (sc_dset_t * dset,
                                             sc_locidx_t lidx);

/** Compute a local index from a global index.  Returns -1 if the global
 * index does not have a local index. O(log(num_borrowed)). */
sc_locidx_t         sc_dset_global_to_local (sc_dset_t * dset,
                                             sc_gloidx_t gidx);

/** Get a list of all borrowed global indices */
void                sc_dset_get_borrowed (sc_dset_t * dset,
                                          const sc_gloidx_t ** borrowed);
/** Restore a list of all borrowed global indices */
void                sc_dset_restore_borrowed (sc_dset_t * dset,
                                              const sc_gloidx_t ** borrowed);

/** Get a list of all lent local indices */
void                sc_dset_get_lent (sc_dset_t * dset,
                                      const sc_locidx_t ** lent);

/** Restore a list of all lent local indices */
void                sc_dset_restore_lent (sc_dset_t * dset,
                                          const sc_locidx_t ** lent);

/** The number of other processes whose local indices overlap this processes
 * local indices */
sc_locidx_t         sc_dset_num_comm_ranks (sc_dset_t * dset);

/** Get a list of all overlapping processes */
void                sc_dset_get_comm_ranks (sc_dset_t * dset,
                                            const int **ranks);

/** Restore a list of all overlapping processes */
void                sc_dset_restore_comm_ranks (sc_dset_t * dset,
                                                const int **ranks);

/** Get the range of local indices that are borrowed from \a rank.  From this,
 * the global indices can be determined with sc_dset_local_to_global */
void                sc_dset_rank_borrowed_range (sc_dset_t * dset,
                                                 sc_locidx_t * first,
                                                 sc_locidx_t * lastplusone,
                                                 int rank);

/** The number of indices lent to \a rank */
sc_locidx_t         sc_dset_rank_num_lent (sc_dset_t * dset, int rank);

/** Get the set of local indices that are lent to \a rank */
void                sc_dset_rank_get_lent (sc_dset_t * dset, int rank,
                                           const sc_locidx_t ** lent);

/** Retore the set of local indices that are lent ot \a rank */
void                sc_dset_rank_restore_lent (sc_dset_t * dset, int rank,
                                               const sc_locidx_t ** lent);

/** The number of indices overlapped with \a rank: includes borrowed from,
 * lent to, and borrowed by both */
sc_locidx_t         sc_dset_rank_num_mutual (sc_dset_t * dset, int rank);

/** Get the local indices overlapped with \a rank */
void                sc_dset_rank_get_mutual (sc_dset_t * dset, int rank,
                                             const sc_locidx_t ** mutual);

/** Restore the local indices overlapped with \a rank */
void                sc_dset_rank_restore_mutual (sc_dset_t * dset, int rank,
                                                 const sc_locidx_t ** mutual);

/** The communication modes enabled by a dset */
typedef enum
{
  SC_DSET_COMM_SCATTER = 0,     /* send data of all owned indices to all
                                   overlapping processes */
  SC_DSET_COMM_GATHER,          /* gather data of all owned indices from all
                                   overlapping processes */
  SC_DSET_COMM_REDUCE,          /* gather data of all owned indices from all
                                   overlapping processes, and reduce one value
                                   for every owned index */
  SC_DSET_COMM_ALLGATHER,       /* gather data of all local indices from all
                                   overlapping processes */
  SC_DSET_COMM_ALLREDUCE,       /* gather data of all local indices from all
                                   overlapping processes, and reduce one value
                                   for every local index */
  SC_DSET_COMM_INVALID
}
sc_dset_comm_type_t;

/** sc_dset_comm_t handles individual communication instances */
typedef struct sc_dset_comm sc_dset_comm_t;

/** returns true and set id of a receive buffer if one remains, otherwise
 * returns false */
int                 sc_dset_comm_recv_waitany (sc_dset_comm_t * comm,
                                               int *id);
/** Get a receive buffer, and the local indices it references,
 * from an id.  returns the rank that the data was received from */
int                 sc_dset_comm_get_recv (sc_dset_comm_t * comm,
                                           int id, int *rank,
                                           sc_array_t ** recv,
                                           const sc_locidx_t ** indices);
/** Restore a receive buffer and indices */
void                sc_dset_comm_restore_recv (sc_dset_comm_t * comm,
                                               int id,
                                               sc_array_t ** recv,
                                               const sc_locidx_t ** indices);
/** Wait for all scheduled sends and receives to finis */
void                sc_dset_comm_waitall (sc_dset_comm_t * comm);
void                sc_dset_comm_destroy (sc_dset_comm_t * comm);

/** scatter: here are the intended patterns for scatter type communication.
 *
 * one time, blocking send and receive:
 *
 *   sc_locidx_t nowned, nborrowed;
 *   sc_dset_t *dset;
 *   sc_array_t *owned_data;
 *   sc_array_t *borrowed_data;
 *
 *   // ... setup dset
 *
 *   nowned = sc_dset_num_owned (dset);
 *   nborrowed = sc_dset_num_borrowed (dset);
 *
 *   owned_data = sc_array_new_size (sizeof (data_type_t), nowned);
 *   borrowed_data = sc_array_new_size (sizeof (data_type_t), nborrowed);
 *
 *   // changes to owned_data that needed to be duplicated on other processes
 *
 *   sc_dset_scatter_sendrecv (dset, owned_data, borrowed_data, NULL);
 *
 *   // ... values in owned_data and borrowed_data match each other, can be
 *   // safely used
 *
 *   sc_array_destroy (owned_data);
 *   sc_array_destroy (borrowed_data);
 *
 * repeated send and receive: a handle to a comm is provided in this case
 * because, depending on the implementation of sc_dset_comm_t, the
 * communication pattern may be saved using mpi's persistent requests
 *
 *   sc_array_t *owned_data, *owned_data2;
 *   sc_array_t *borrowed_data, *borrowed_data2;
 *   sc_dset_comm_t *comm;
 *
 *   // ... setup
 *
 *   sc_dset_scatter_sendrecv (dset, owned_data, borrowed_data, &comm);
 *
 *   // ...
 *
 *   sc_dset_scatter_sendrecv (dset, owned_data, borrowed_data, &comm);
 *
 *   // ...
 *
 *   sc_dset_comm_destroy (comm);
 *
 *   // ... tear down
 *
 * nonblocking send and recv
 *
 *   sc_dset_t *dset;
 *   sc_array_t *owned_data;
 *   sc_array_t *borrowed_data;
 *   sc_dset_comm_t *comm;
 *
 *   // ... setup
 *
 *   // the order of _irecv and _isend can be changed
 *
 *   sc_dset_scatter_irecv (dset, borrowed_data, &comm);
 *
 *   // ... maybe some work in between, borrowed_data can't be used yet
 *
 *   sc_dset_scatter_isend (dset, owned_data, &comm);
 *
 *   // ... overlapping computation, owned_data can be read but should not be
 *   // changed, that would cause undefined behavior
 *
 *   // now we need to read borrowed_data or change owned_data, so we have to
 *   // complete the messages
 *   sc_dset_comm_waitall (comm);
 *
 *   // ... repeat multiple times as needed
 *
 *   // we're done
 *   sc_dset_comm_destroy (comm);
 *
 *   // ... tear down
 *
 */

void                sc_dset_scatter_sendrecv (sc_dset_t * dset,
                                              sc_array_t * owned_data,
                                              sc_array_t * borrowed_data,
                                              sc_array_t ** comm);
void                sc_dset_scatter_irecv (sc_dset_t * dset,
                                           sc_array_t * borrowed_data,
                                           sc_dset_comm_t ** comm);
void                sc_dset_scatter_isend (sc_dset_t * dset,
                                           sc_array_t * owned_data,
                                           sc_dset_comm_t ** comm);

/** gather: the intended communication patterns are the same as the scatter
 * patterns, but with owned_data and borrowed_data swapped.  Here is how we
 * intend the gathered data to be processes after completion.
 *
 *   sc_dset_t *dset;
 *   sc_array_t *borrowed_data;
 *   sc_array_t *owned_data;
 *   sc_dset_comm_t *comm;
 *   int ncomm;
 *
 *   // ... some local changes to borrowed_data that need to be sent back to
 *   // the owners
 *
 *   sc_dset_gather_sendrecv (dset, owned_data, borrowed_data, &comm);
 *
 *   ncomm = sc_dset_comm_num_recv (comm);
 *   for (i = 0; i < ncomm; i++) {
 *     const sc_locidx_t *indices;
 *     sc_array_t *recv;
 *     sc_locidx_t nrecv, jl;
 *     int rank;
 *
 *     rank = sc_dset_comm_get_recv (comm, i, &recv, &indices);
 *
 *     nrecv = recv->elem_count;
 *     assert (nrecv == sc_dset_rank_num_lent (dset, rank));
 *
 *     for (jl = 0; jl < nrecv; jl++) {
 *       sc_locidx_t lidx;
 *       data_type_t *value; // data_type in borrowed_data and owned_data
 *
 *       lidx = indices[jl];
 *       value = (data_type_t *) sc_array_index (recv, (size_t) jl);
 *
 *       // process value ...
 *     }
 *     sc_dset_comm_restore_recv (comm, i, &recv, &indices);
 *   }
 *
 *   sc_dset_comm_destroy (comm);
 *
 * now here's how that looks using nonblocking communication
 *
 *   sc_dset_t *dset;
 *   sc_array_t *borrowed_data;
 *   sc_array_t *owned_data;
 *   sc_dset_comm_t *comm;
 *   sc_array_t *recv;
 *   int i;
 *
 *   // ... post the _irecv for owned_data.  It's still safe to change values
 *   // in owned_data, becaues the received values are buffered elsewhere
 *
 *   sc_dset_gather_irecv (dset, owned_data, &comm);
 *
 *   // ... some local changes to borrowed_data that need to be sent back to
 *   // the owners
 *
 *   sc_dset_gather_isend (dset, borrowed_data, &comm);
 *
 *   // ... some overlapping computation
 *
 *   // now I'm ready to process what I've received
 *   while (sc_dset_comm_recv_waitany (comm, &i)) {
 *     // same contents as the for loop in the previous example
 *   }
 *
 *   // wait for all messages to complete
 *   sc_dset_comm_waitall (comm);
 *   sc_dset_comm_destroy (comm);
 */

void                sc_dset_gather_sendrecv (sc_dset_t * dset,
                                             sc_array_t * owned_data,
                                             sc_array_t * borrowed_data,
                                             sc_dset_comm_t ** comm);
void                sc_dset_gather_irecv (sc_dset_t * dset,
                                          sc_array_t * owned_data,
                                          sc_dset_t ** comm);
void                sc_dset_gather_isend (sc_dset_t * dset,
                                          sc_array_t * borrowed_data,
                                          sc_dset_t ** comm);

/** reduce examples
 *
 * nonblocking:
 *
 *   sc_locidx_t nlocal, nowned, nborrowed;
 *   double *vec1, *vec2;
 *   int blocksize = 3;
 *   sc_array_t * owned1, *owned2;
 *   sc_array_t * borrowed1, *borrowed2;
 *   sc_dset_t *dset;
 *   sc_dset_comm_t *comm_scatter, *comm_reduce;
 *
 *   // ... setup dset
 *
 *   nlocal = sc_dset_num_local (dset);
 *   nowned = sc_dset_num_owned (dset);
 *   nborrowed = sc_dset_num_borrowed (dset);
 *
 *   vec1 = SC_ALLOC (double, blocksize * nlocal);
 *   vec2 = SC_ALLOC (double, blocksize * nlocal);
 *
 *   owned1 = sc_array_new_data (vec1, blocksize * sizeof (double), nowned);
 *   borrowed1 = sc_array_new_data (vec1 + blocksize * nlocal, sizeof (double),
 *                                 nborrowed);
 *   owned2 = sc_array_new_data (vec2, blocksize * sizeof (double), nowned);
 *   borrowed2 = sc_array_new_data (vec2 + blocksize * nlocal, sizeof (double),
 *                                 nborrowed);
 *
 *   sc_dset_scatter_irecv (dset, borrowed1, &comm_scatter);
 *
 *   // ... initialize owned portion of vec1
 *
 *   sc_dset_scatter_isend (dset, owned1, &comm_scatter);
 *
 *   // ... start computing vec2, but we need borrowed values to complete this
 *
 *   sc_dset_comm_waitall (comm_scatter);
 *   sc_dset_destroy (comm_scatter);
 *
 *   // ... now we can finish our contribution to vec2, but the borrowed
 *   // values need to summed on the ranks that own them
 *
 *   sc_dset_reduce_irecv (dset, owned2, 3, MPI_DOUBLE, MPI_SUM,
 *                         &comm_gather);
 *
 *   // ... computation to complete the local contribution to the borrowed
 *   // values in vec2
 *
 *   sc_dset_reduce_isend (dest, borrowed2, 3, MPI_DOUBLE, MPI_SUM,
 *                         &comm_gather);
 *
 *   // ... even more overlapping computation
 *
 *   // now we need the reduced values in vec2
 *   sc_dset_comm_waitall (comm_gather);
 *   sc_dset_comm_destroy (comm_gather);
 *
 *   // ... tear down
 *
 * here is the blocking version: notice that no comm handles are needed
 *
 *   sc_locidx_t nlocal, nowned, nborrowed;
 *   double *vec1, *vec2;
 *   int blocksize = 3;
 *   sc_array_t * owned1, *owned2;
 *   sc_array_t * borrowed1, *borrowed2;
 *   sc_dset_t *dset;
 *   sc_dset_comm_t *comm_scatter, *comm_reduce;
 *
 *   // ... setup dset
 *
 *   nlocal = sc_dset_num_local (dset);
 *   nowned = sc_dset_num_owned (dset);
 *   nborrowed = sc_dset_num_borrowed (dset);
 *
 *   vec1 = SC_ALLOC (double, blocksize * nlocal);
 *   vec2 = SC_ALLOC (double, blocksize * nlocal);
 *
 *   owned1 = sc_array_new_data (vec1, blocksize * sizeof (double), nowned);
 *   borrowed1 = sc_array_new_data (vec1 + blocksize * nlocal, sizeof (double),
 *                                 nborrowed);
 *   owned2 = sc_array_new_data (vec2, blocksize * sizeof (double), nowned);
 *   borrowed2 = sc_array_new_data (vec2 + blocksize * nlocal, sizeof (double),
 *                                 nborrowed);
 *
 *   // ... initialize owned portion of vec1
 *
 *   sc_dset_scatter_sendrecv (dset, owned1, borrowed1, NULL);
 *
 *   // ... compute local contribution to vec2
 *
 *   sc_dset_reduce_sendrecv (dset, owned1, borrowed1, 3, MPI_DOUBLE, MPI_SUM,
 *                            NULL);
 *
 *   // ... tear down
 */
void                sc_dset_reduce_sendrecv (sc_dset_t * dset,
                                             sc_array_t * owned_data,
                                             sc_array_t * borrowed_data,
                                             int datacount,
                                             MPI_Datatype datatype,
                                             MPI_Op op, sc_dset_t ** comm);
void                sc_dset_reduce_irecv (sc_dset_t * dset,
                                          sc_array_t * owned_data,
                                          int datacount,
                                          MPI_Datatype datatype,
                                          MPI_Op op, sc_dset_t ** comm);
void                sc_dset_reduce_isend (sc_dset_t * dset,
                                          sc_array_t * borrowed_data,
                                          int datacount,
                                          MPI_Datatype datatype,
                                          MPI_Op op, sc_dset_t ** comm);

/** the allgather examples look just like the gather examples */
void                sc_dset_allgather_sendrecv (sc_dset_t * dset,
                                                sc_array_t * owned_data,
                                                sc_array_t * borrowed_data,
                                                sc_dset_comm_t ** comm);
void                sc_dset_allgather_irecv (sc_dset_t * dset,
                                             sc_array_t * owned_data,
                                             sc_array_t * borrowed_data,
                                             sc_dset_comm_t ** comm);
void                sc_dset_allgather_isend (sc_dset_t * dset,
                                             sc_array_t * owned_data,
                                             sc_array_t * borrowed_data,
                                             sc_dset_comm_t ** comm);

/** Here is the same vec1, vec2 update shown in reduce, but using one round of
 * communication using allreduce.  It assumes that all borrowed values are
 * correct on entry, and ensures that all borrowed values are correct on exit.
 * while there is only one round of communication, there are potentially many
 * more messages being passed, depending on how much overlap there is between
 * local indices.
 *
 *   sc_locidx_t nlocal, nowned, nborrowed, il;
 *   double *vec1, *vec2;
 *   int blocksize = 3;
 *   sc_array_t * owned1, *owned2;
 *   sc_array_t * borrowed1, *borrowed2;
 *   sc_dset_t *dset;
 *   sc_dset_comm_t *comm;
 *
 *   // ... setup vectors and arrays as before: we assume borrowed values in
 *   // vec1 are correct on all processes
 *
 *   // ... set vec2 to zero
 *   for (il = 0; il < blocksize * nlocal; il++) {
 *     vec2[il] = 0.;
 *   }
 *
 *   // post the reduce receives
 *   sc_dset_allreduce_irecv (dset, owned2, borrowed2, 3, MPI_DOUBLE, MPI_SUM,
 *                            &comm);
 *
 *   // ... compute all local contributions the the borrowed section of vec2
 *
 *   // post the reduce sends
 *   sc_dset_allreduce_isend (dset, owned2, borrowed2, 3, MPI_DOUBLE, MPI_SUM,
 *                            &comm);
 *
 *   // ... compute all remaining local contributions to vec2
 *
 *   // now we need the reduced values in vec2
 *   sc_dset_comm_waitall (comm);
 *   sc_dset_comm_destroy (comm);
 *
 *   // ... tear down
 */
void                sc_dset_allreduce_sendrecv (sc_dset_t * dset,
                                                sc_array_t * owned_data,
                                                sc_array_t * borrowed_data,
                                                int datacount,
                                                MPI_Datatype datatype,
                                                MPI_Op op,
                                                sc_dset_comm_t ** comm);
void                sc_dset_allreduce_irecv (sc_dset_t * dset,
                                             sc_array_t * owned_data,
                                             sc_array_t * borrowed_data,
                                             int datacount,
                                             MPI_Datatype datatype,
                                             MPI_Op op,
                                             sc_dset_comm_t ** comm);
void                sc_dset_allreduce_isend (sc_dset_t * dset,
                                             sc_array_t * owned_data,
                                             sc_array_t * borrowed_data,
                                             int datacount,
                                             MPI_Datatype datatype,
                                             MPI_Op op,
                                             sc_dset_comm_t ** comm);

/** struct implementation follows */

typedef struct sc_dset_rank
{
  int                 other_rank;       /* the rank of the other
                                           process */

  sc_locidx_t         num_borrowed;     /* the number the indices in
                                           the dset->borrowed array
                                           that are owned by the
                                           other rank */
  sc_locidx_t         first_borrowed;   /* the first such index */

  sc_locidx_t         num_lent; /* the number of indices
                                   owned by this process
                                   that are shared with the
                                   the other process */
  sc_locidx_t        *lent;     /* the owned indices shared
                                   with the other process */

  sc_locidx_t         num_shared;       /* the number of indices
                                           that are borrowed by both
                                           this process and the
                                           other process from other
                                           processes shared by
                                           neither */
  sc_locidx_t        *shared;   /* the overlapped indices */

  sc_array_t         *work;     /* used during setup */
}
sc_dset_rank_t;

struct sc_dset
{
  MPI_Comm            mpicomm;
  int                 mpirank;
  int                 mpisize;
  int                 in_setup;

  sc_locidx_t         num_owned;
  sc_gloidx_t         first_owned;
  sc_gloidx_t        *first_owned_by_rank;      /* [mpisize + 1] */

  sc_locidx_t         num_borrowed;
  sc_gloidx_t        *borrowed;

  sc_locidx_t         num_lent;
  sc_locidx_t        *lent;

  int                 num_comm_ranks;
  sc_dset_rank_t     *comm_ranks;

  sc_array_t         *work_borrowed;    /* used during setup */
  sc_array_t         *work_lent;
  sc_array_t         *work_comm_ranks;
};

struct sc_dset_comm
{
  sc_array_t         *owned_data;
  sc_array_t         *borrowed_data;
  sc_dset_t          *dset;
  sc_dset_comm_type_t type;
  int                 datacount;
  MPI_Datatype        datatype;
  MPI_Op              op;
  sc_array_t         *send_buffers;
  sc_array_t         *send_requests;
  sc_array_t         *recv_buffers;
  sc_array_t         *recv_requests;
};

SC_EXTERN_C_END;

#endif /* !SC_DSET_H */
