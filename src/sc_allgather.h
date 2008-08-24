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

#define SC_AG_ALLTOALL_TAG      1
#define SC_AG_RECURSIVE_TAG_A   2
#define SC_AG_RECURSIVE_TAG_B   3

/** Returns the size of the MPI data type in bytes. */
size_t              sc_mpi_sizeof (MPI_Comm mpicomm, MPI_Datatype t);

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

/** Drop-in allgather replacement.
 */
int                 sc_allgather (void *sendbuf, int sendcount,
                                  MPI_Datatype sendtype, void *recvbuf,
                                  int recvcount, MPI_Datatype recvtype,
                                  MPI_Comm mpicomm);

#endif /* !SC_ALLGATHER_H */
