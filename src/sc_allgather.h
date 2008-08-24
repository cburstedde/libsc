/*
 * Copyright (c) 2008 Carsten Burstedde <carsten@ices.utexas.edu>
 *
 * May only be used with the Mangll, Rhea and P4est codes.
 * Any other use is prohibited.
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
