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

/** \file sc_allgather.h
 * Self-contained implementation of MPI_Allgather.
 *
 * The algorithm uses a binary communication tree.
 * The recursion terminates at a specified depth by an all-to-all step.
 *
 * \ingroup sc_parallelism
 */

#ifndef SC_ALLGATHER_H
#define SC_ALLGATHER_H

#include <sc.h>

#ifndef SC_ALLGATHER_ALLTOALL_MAX
/** The largest group size that uses direct all-to-all. */
#define SC_ALLGATHER_ALLTOALL_MAX   5
#endif

SC_EXTERN_C_BEGIN;

/** Allgather by direct point-to-point communication.
 * This function is only efficient for small group sizes.
 * \param [in] mpicomm      Valid MPI communicator.
 * \param [in,out] data     Send and receive buffer for a subgroup of the
                            communicator.
 * \param [in] datasize     Number of bytes to send.
 * \param [in] groupsize    Number of processes in the subgroup.
 * \param [in] myoffset     Offset of the subgroup in the communicator.
 * \param [in] myrank       MPI rank in the communicator.
 */
void                sc_allgather_alltoall (sc_MPI_Comm mpicomm, char *data,
                                           int datasize, int groupsize,
                                           int myoffset, int myrank);

/** Perform recursive bisection allgather.
 * When size becomes less equal \ref SC_ALLGATHER_ALLTOALL_MAX, call \ref
 * sc_allgather_alltoall.
 * \param [in] mpicomm      Valid MPI communicator.
 * \param [in,out] data     Send and receive buffer for a subgroup of the
                            communicator.
 * \param [in] datasize     Number of bytes to send.
 * \param [in] groupsize    Number of processes in the subgroup.
 * \param [in] myoffset     Offset of the subgroup in the communicator.
 * \param [in] myrank       MPI rank in the communicator.
 */
void                sc_allgather_recursive (sc_MPI_Comm mpicomm, char *data,
                                            int datasize, int groupsize,
                                            int myoffset, int myrank);

/** Drop-in allgather replacement.
 * \param [in] sendbuf      Send buffer conforming to MPI specification.
 * \param [in] sendcount    Number of data items to send.
 * \param [in] sendtype     Valid MPI Datatype.
 * \param [out] recvbuf     Receive buffer conforming to MPI specification.
 * \param [in] recvcount    Number of data items to receive.
 * \param [in] recvtype     Valid MPI Datatype.
 * \param [in] mpicomm      Valid MPI communicator.
 * \return int              sc_MPI_SUCCESS if not aborting on MPI error.
 */
int                 sc_allgather (void *sendbuf, int sendcount,
                                  sc_MPI_Datatype sendtype, void *recvbuf,
                                  int recvcount, sc_MPI_Datatype recvtype,
                                  sc_MPI_Comm mpicomm);

SC_EXTERN_C_END;

#endif /* !SC_ALLGATHER_H */
