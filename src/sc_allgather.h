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

#ifndef SC_ALLGATHER_H
#define SC_ALLGATHER_H

#include <sc.h>

#ifndef SC_AG_ALLTOALL_MAX
#define SC_AG_ALLTOALL_MAX      5
#endif

SC_EXTERN_C_BEGIN;

/** Allgather by direct point-to-point communication.
 * Only makes sense for small group sizes.
 */
void                sc_allgather_alltoall (sc_MPI_Comm mpicomm, char *data,
                                           int datasize, int groupsize,
                                           int myoffset, int myrank);

/** Performs recursive bisection allgather.
 * When size becomes small enough, calls sc_ag_alltoall.
 */
void                sc_allgather_recursive (sc_MPI_Comm mpicomm, char *data,
                                            int datasize, int groupsize,
                                            int myoffset, int myrank);

/** Drop-in allgather replacement.
 */
int                 sc_allgather (void *sendbuf, int sendcount,
                                  sc_MPI_Datatype sendtype, void *recvbuf,
                                  int recvcount, sc_MPI_Datatype recvtype,
                                  sc_MPI_Comm mpicomm);


/** Prototype for a function that allgathers into a newly created array
 * __DATA THAT IS NOT TO BE CHANGED__.  This is to allow for implementations
 * where the data is shared between mpi processes.
 */
typedef void (*sc_allgather_final_create_t) (void *sendbuf, int sendcount,
                                             sc_MPI_Datatype sendtype,
                                             void **recvbuf,
                                             int recvcount, sc_MPI_Datatype recvtype,
                                             sc_MPI_Comm mpicomm);

typedef void (*sc_allgather_final_scan_create_t) (void *sendbuf, void **recvbuf,
                                                  int count, sc_MPI_Datatype type,
                                                  sc_MPI_Op op, MPI_Comm comm);

/** Prototype for a function that destroys the receive buffer created
 * by a sc_allgather_final_create_t implementation.
 */
typedef void (*sc_allgather_final_destroy_t) (void *recvbuf, sc_MPI_Comm mpicomm);

/** function pointers, so that the user can at runtime change which method is
 * used from the default */
extern sc_allgather_final_create_t sc_allgather_final_create;
extern sc_allgather_final_scan_create_t sc_allgather_final_scan_create;
extern sc_allgather_final_destroy_t sc_allgather_final_destroy;

/** default implementation using SC_ALLOC and sc_MPI_Allgather() */
void sc_allgather_final_create_default(void *sendbuf, int sendcount,
                                       sc_MPI_Datatype sendtype, void **recvbuf, int recvcount,
                                       sc_MPI_Datatype recvtype, sc_MPI_Comm mpicomm);
/** default implementation: alloc, allgather, sum on array: only works for
 * MPI_SUM and pre-defined integer and floating point types*/
void sc_allgather_final_scan_create_default(void *sendbuf, void **recvbuf, int count,
                                            sc_MPI_Datatype type, sc_MPI_Op op, sc_MPI_Comm mpicomm);
/** prescan implementation: mpi_scan, then alloc, then allgather: works for any type or op */
void sc_allgather_final_scan_create_prescan(void *sendbuf, void **recvbuf, int count,
                                            sc_MPI_Datatype type, sc_MPI_Op op, sc_MPI_Comm mpicomm);
void sc_allgather_final_destroy_default(void *recvbuf, sc_MPI_Comm mpicomm);

/** implementation for shared memory spaces that WON'T work in most settings */
void sc_allgather_final_create_shared(void *sendbuf, int sendcount,
                                       sc_MPI_Datatype sendtype, void **recvbuf, int recvcount,
                                       sc_MPI_Datatype recvtype, sc_MPI_Comm mpicomm);
void sc_allgather_final_scan_create_shared(void *sendbuf, void **recvbuf, int count,
                                           sc_MPI_Datatype type, sc_MPI_Op op, sc_MPI_Comm mpicomm);
void sc_allgather_final_scan_create_shared_prescan(void *sendbuf, void **recvbuf, int count,
                                                   sc_MPI_Datatype type, sc_MPI_Op op, sc_MPI_Comm mpicomm);
void sc_allgather_final_destroy_shared(void *recvbuf, sc_MPI_Comm mpicomm);

/** implementation using MPI_Win_allocate_shared() */
void sc_allgather_final_create_window(void *sendbuf, int sendcount,
                                       sc_MPI_Datatype sendtype, void **recvbuf, int recvcount,
                                       sc_MPI_Datatype recvtype, sc_MPI_Comm mpicomm);
void sc_allgather_final_scan_create_window(void *sendbuf, void **recvbuf, int count,
                                           sc_MPI_Datatype type, sc_MPI_Op op, sc_MPI_Comm mpicomm);
void sc_allgather_final_scan_create_window_prescan(void *sendbuf, void **recvbuf, int count,
                                                   sc_MPI_Datatype type, sc_MPI_Op op, sc_MPI_Comm mpicomm);
void sc_allgather_final_destroy_window(void *recvbuf, sc_MPI_Comm mpicomm);

SC_EXTERN_C_END;

#endif /* !SC_ALLGATHER_H */
