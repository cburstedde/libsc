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

/** \file sc_reduce.h
 * Self-contained implementations of MPI_Reduce and MPI_Allreduce.
 *
 * Our motivation for writing these alternatives is reproducible
 * associativity.  With this implementation, the associativity of the
 * reduction operation depends on the size of the communicator only and does
 * not suffer from random or otherwise obscure influences.
 *
 * Both algorithms use a binary communication tree.
 * We provide implementations via a customizable reduction operator
 * as well as drop-in replacements for minimum, maximum, and sum.
 * We do not currently support user-defined MPI datatypes.
 *
 * \ingroup sc_parallelism
 */

#ifndef SC_REDUCE_H
#define SC_REDUCE_H

#include <sc.h>

#ifndef SC_REDUCE_ALLTOALL_LEVEL
/** The highest recursion level that uses direct all-to-all. */
#define SC_REDUCE_ALLTOALL_LEVEL        3
#endif

SC_EXTERN_C_BEGIN;

/** Prototype for a user-defined reduce operation. */
typedef void        (*sc_reduce_t) (void *sendbuf, void *recvbuf,
                                    int sendcount, sc_MPI_Datatype sendtype);

/** Custom allreduce operation with reproducible associativity.
 * \param [in] sendbuf      Send buffer conforming to MPI specification.
 * \param [out] recvbuf     Receive buffer conforming to MPI specification.
 * \param [in] sendcount    Number of data items to reduce.
 * \param [in] sendtype     Valid MPI datatype.
 * \param [in] reduce_fn    Custom, associative reduction operator.
 * \param [in] mpicomm      Valid MPI communicator.
 * \return                  sc_MPI_SUCCESS if not aborting on MPI error.
 */
int                 sc_allreduce_custom (void *sendbuf, void *recvbuf,
                                         int sendcount,
                                         sc_MPI_Datatype sendtype,
                                         sc_reduce_t reduce_fn,
                                         sc_MPI_Comm mpicomm);

/** Custom reduce operation with reproducible associativity.
 * \param [in] sendbuf      Send buffer conforming to MPI specification.
 * \param [out] recvbuf     Receive buffer conforming to MPI specification.
 * \param [in] sendcount    Number of data items to reduce.
 * \param [in] sendtype     Valid MPI datatype.
 * \param [in] reduce_fn    Custom, associative reduction operator.
 * \param [in] target       The MPI rank that obtains the result.
 * \param [in] mpicomm      Valid MPI communicator.
 * \return                  sc_MPI_SUCCESS if not aborting on MPI error.
 */
int                 sc_reduce_custom (void *sendbuf, void *recvbuf,
                                      int sendcount, sc_MPI_Datatype sendtype,
                                      sc_reduce_t reduce_fn,
                                      int target, sc_MPI_Comm mpicomm);

/** Drop-in MPI_Allreduce replacement with reproducible associativity.
 * Currently we support the operations minimum, maximum, and sum.
 * \param [in] sendbuf      Send buffer conforming to MPI specification.
 * \param [out] recvbuf     Receive buffer conforming to MPI specification.
 * \param [in] sendcount    Number of data items to reduce.
 * \param [in] sendtype     Valid MPI datatype.
 * \param [in] operation    \ref sc_MPI_MIN, \ref sc_MPI_MAX, or \ref
 *                          sc_MPI_SUM.  We abort otherwise.
 * \param [in] mpicomm      Valid MPI communicator.
 * \return                  sc_MPI_SUCCESS if not aborting on MPI error.
 */
int                 sc_allreduce (void *sendbuf, void *recvbuf, int sendcount,
                                  sc_MPI_Datatype sendtype,
                                  sc_MPI_Op operation, sc_MPI_Comm mpicomm);

/** Drop-in MPI_Reduce replacement with reproducible associativity.
 * Currently we support the operations minimum, maximum, and sum.
 * \param [in] sendbuf      Send buffer conforming to MPI specification.
 * \param [out] recvbuf     Receive buffer conforming to MPI specification.
 * \param [in] sendcount    Number of data items to reduce.
 * \param [in] sendtype     Valid MPI datatype.
 * \param [in] operation    \ref sc_MPI_MIN, \ref sc_MPI_MAX, or \ref
 *                          sc_MPI_SUM.  We abort otherwise.
 * \param [in] target       The MPI rank that obtains the result.
 * \param [in] mpicomm      Valid MPI communicator.
 * \return                  sc_MPI_SUCCESS if not aborting on MPI error.
 */
int                 sc_reduce (void *sendbuf, void *recvbuf, int sendcount,
                               sc_MPI_Datatype sendtype, sc_MPI_Op operation,
                               int target, sc_MPI_Comm mpicomm);

SC_EXTERN_C_END;

#endif /* !SC_REDUCE_H */
