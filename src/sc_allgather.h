/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2008 Carsten Burstedde, Lucas Wilcox.

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

#ifndef SC_ALLGATHER_H
#define SC_ALLGATHER_H

#ifndef SC_H
#error "sc.h should be included before this header file"
#endif

#ifndef SC_AG_ALLTOALL_MAX
#define SC_AG_ALLTOALL_MAX      5
#endif

#ifdef SC_MPI

/** Allgather by direct point-to-point communication.
 * Only makes sense for small group sizes.
 */
void                sc_ag_alltoall (MPI_Comm mpicomm, char *data,
                                    int datasize, int groupsize, int myoffset,
                                    int myrank);

/** Performs recursive bisection allgather.
 * When size becomes small enough, calls sc_ag_alltoall.
 */
void                sc_ag_recursive (MPI_Comm mpicomm, char *data,
                                     int datasize, int groupsize,
                                     int myoffset, int myrank);

#endif /* SC_MPI */

/** Drop-in allgather replacement.
 */
int                 sc_allgather (void *sendbuf, int sendcount,
                                  MPI_Datatype sendtype, void *recvbuf,
                                  int recvcount, MPI_Datatype recvtype,
                                  MPI_Comm mpicomm);

#endif /* !SC_ALLGATHER_H */
