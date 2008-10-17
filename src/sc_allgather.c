/*
 * Copyright (c) 2008 Carsten Burstedde <carsten@ices.utexas.edu>
 *
 * May only be used with the Mangll, Rhea and P4est codes.
 * Any other use is prohibited.
 */

#include <sc.h>
#include <sc_allgather.h>

#ifdef SC_MPI

enum
{
  SC_AG_ALLTOALL_TAG = 17,
  SC_AG_RECURSIVE_TAG_A,
  SC_AG_RECURSIVE_TAG_B,
  SC_AG_RECURSIVE_TAG_C,
};

void
sc_ag_alltoall (MPI_Comm mpicomm, char *data, int datasize,
                int groupsize, int myoffset, int myrank)
{
  int                 j, peer;
  int                 mpiret;
  MPI_Request        *request;

  SC_ASSERT (myoffset >= 0 && myoffset < groupsize);

  request = SC_ALLOC (MPI_Request, 2 * groupsize);

  for (j = 0; j < groupsize; ++j) {
    if (j == myoffset) {
      request[j] = request[groupsize + j] = MPI_REQUEST_NULL;
      continue;
    }
    peer = myrank - (myoffset - j);

    mpiret = MPI_Irecv (data + j * datasize, datasize, MPI_BYTE,
                        peer, SC_AG_ALLTOALL_TAG, mpicomm, request + j);
    SC_CHECK_MPI (mpiret);

    mpiret = MPI_Isend (data + myoffset * datasize, datasize, MPI_BYTE,
                        peer, SC_AG_ALLTOALL_TAG,
                        mpicomm, request + groupsize + j);
    SC_CHECK_MPI (mpiret);
  }

  mpiret = MPI_Waitall (2 * groupsize, request, MPI_STATUSES_IGNORE);
  SC_CHECK_MPI (mpiret);

  SC_FREE (request);
}

void
sc_ag_recursive (MPI_Comm mpicomm, char *data, int datasize,
                 int groupsize, int myoffset, int myrank)
{
  const int           g2 = groupsize / 2;
  const int           g2B = groupsize - g2;
  int                 mpiret;
  MPI_Request         request[3];

  SC_ASSERT (myoffset >= 0 && myoffset < groupsize);

  if (groupsize > SC_AG_ALLTOALL_MAX) {
    if (myoffset < g2) {
      sc_ag_recursive (mpicomm, data, datasize, g2, myoffset, myrank);

      mpiret = MPI_Irecv (data + g2 * datasize, g2B * datasize, MPI_BYTE,
                          myrank + g2, SC_AG_RECURSIVE_TAG_B,
                          mpicomm, request + 0);
      SC_CHECK_MPI (mpiret);

      mpiret = MPI_Isend (data, g2 * datasize, MPI_BYTE,
                          myrank + g2, SC_AG_RECURSIVE_TAG_A,
                          mpicomm, request + 1);
      SC_CHECK_MPI (mpiret);

      if (myoffset == g2 - 1 && g2 != g2B) {
        mpiret = MPI_Isend (data, g2 * datasize, MPI_BYTE,
                            myrank + g2B, SC_AG_RECURSIVE_TAG_C,
                            mpicomm, request + 2);
        SC_CHECK_MPI (mpiret);
      }
      else {
        request[2] = MPI_REQUEST_NULL;
      }
    }
    else {
      sc_ag_recursive (mpicomm, data + g2 * datasize, datasize, g2B,
                       myoffset - g2, myrank);

      if (myoffset == groupsize - 1 && g2 != g2B) {
        request[0] = MPI_REQUEST_NULL;
        request[1] = MPI_REQUEST_NULL;

        mpiret = MPI_Irecv (data, g2 * datasize, MPI_BYTE,
                            myrank - g2B, SC_AG_RECURSIVE_TAG_C,
                            mpicomm, request + 2);
        SC_CHECK_MPI (mpiret);
      }
      else {
        mpiret = MPI_Irecv (data, g2 * datasize, MPI_BYTE,
                            myrank - g2, SC_AG_RECURSIVE_TAG_A,
                            mpicomm, request + 0);
        SC_CHECK_MPI (mpiret);

        mpiret = MPI_Isend (data + g2 * datasize, g2B * datasize, MPI_BYTE,
                            myrank - g2, SC_AG_RECURSIVE_TAG_B,
                            mpicomm, request + 1);
        SC_CHECK_MPI (mpiret);

        request[2] = MPI_REQUEST_NULL;
      }
    }

    mpiret = MPI_Waitall (3, request, MPI_STATUSES_IGNORE);
    SC_CHECK_MPI (mpiret);
  }
  else {
    sc_ag_alltoall (mpicomm, data, datasize, groupsize, myoffset, myrank);
  }
}

#endif /* SC_MPI */

int
sc_allgather (void *sendbuf, int sendcount, MPI_Datatype sendtype,
              void *recvbuf, int recvcount, MPI_Datatype recvtype,
              MPI_Comm mpicomm)
{
#ifdef SC_MPI
  int                 mpiret;
  int                 mpisize;
  int                 mpirank;
  size_t              datasize, datasize2;

  mpiret = MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  /* *INDENT-OFF* HORRIBLE indent bug */
  datasize = (size_t) sendcount * sc_mpi_sizeof (sendtype);
  datasize2 = (size_t) recvcount * sc_mpi_sizeof (recvtype);
  /* *INDENT-ON* */
  SC_ASSERT (datasize == datasize2);

  memcpy (((char *) recvbuf) + mpirank * datasize, sendbuf, datasize);
  sc_ag_recursive (mpicomm, recvbuf, (int) datasize,
                   mpisize, mpirank, mpirank);

  return MPI_SUCCESS;
#else
  return MPI_Allgather (sendbuf, sendcount, sendtype,
                        recvbuf, recvcount, recvtype, mpicomm);
#endif
}

/* EOF sc_allgather.c */
