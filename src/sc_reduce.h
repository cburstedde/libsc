/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2010 Carsten Burstedde, Lucas Wilcox.

  The SC Library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with the SC Library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SC_REDUCE_H
#define SC_REDUCE_H

#include <sc.h>

/* highest level that uses all-to-all instead of recursion */
#ifndef SC_REDUCE_ALLTOALL_LEVEL
#define SC_REDUCE_ALLTOALL_LEVEL        3
#endif

typedef void        (*sc_reduce_t) (void *sendbuf, void *recvbuf,
                                    int sendcount, MPI_Datatype sendtype);

SC_EXTERN_C_BEGIN;

/** Custom allreduce operation.
 */
int                 sc_allreduce_custom (void *sendbuf, void *recvbuf,
                                         int sendcount, MPI_Datatype sendtype,
                                         sc_reduce_t reduce_fn,
                                         MPI_Comm mpicomm);

/** Custom reduce operation.
 * \param [in] target   The MPI rank that obtains the result.
 */
int                 sc_reduce_custom (void *sendbuf, void *recvbuf,
                                      int sendcount, MPI_Datatype sendtype,
                                      sc_reduce_t reduce_fn,
                                      int target, MPI_Comm mpicomm);

/** Drop-in MPI_Allreduce replacement.
 */
int                 sc_allreduce (void *sendbuf, void *recvbuf, int sendcount,
                                  MPI_Datatype sendtype, MPI_Op operation,
                                  MPI_Comm mpicomm);

/** Drop-in MPI_Reduce replacement.
 * \param [in] target   The MPI rank that obtains the result.
 */
int                 sc_reduce (void *sendbuf, void *recvbuf, int sendcount,
                               MPI_Datatype sendtype, MPI_Op operation,
                               int target, MPI_Comm mpicomm);

SC_EXTERN_C_END;

#endif /* !SC_REDUCE_H */
