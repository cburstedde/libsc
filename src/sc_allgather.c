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

#include <sc_allgather.h>

void
sc_allgather_alltoall (sc_MPI_Comm mpicomm, char *data, int datasize,
                       int groupsize, int myoffset, int myrank)
{
  int                 j, peer;
  int                 mpiret;
  sc_MPI_Request     *request;

  SC_ASSERT (myoffset >= 0 && myoffset < groupsize);

  request = SC_ALLOC (sc_MPI_Request, 2 * groupsize);

  for (j = 0; j < groupsize; ++j) {
    if (j == myoffset) {
      request[j] = request[groupsize + j] = sc_MPI_REQUEST_NULL;
      continue;
    }
    peer = myrank - (myoffset - j);

    mpiret = sc_MPI_Irecv (data + j * datasize, datasize, sc_MPI_BYTE,
                           peer, SC_TAG_AG_ALLTOALL, mpicomm, request + j);
    SC_CHECK_MPI (mpiret);

    mpiret = sc_MPI_Isend (data + myoffset * datasize, datasize, sc_MPI_BYTE,
                           peer, SC_TAG_AG_ALLTOALL,
                           mpicomm, request + groupsize + j);
    SC_CHECK_MPI (mpiret);
  }

  mpiret = sc_MPI_Waitall (2 * groupsize, request, sc_MPI_STATUSES_IGNORE);
  SC_CHECK_MPI (mpiret);

  SC_FREE (request);
}

void
sc_allgather_recursive (sc_MPI_Comm mpicomm, char *data, int datasize,
                        int groupsize, int myoffset, int myrank)
{
  const int           g2 = groupsize / 2;
  const int           g2B = groupsize - g2;
  int                 mpiret;
  sc_MPI_Request      request[3];

  SC_ASSERT (myoffset >= 0 && myoffset < groupsize);

  if (groupsize > SC_AG_ALLTOALL_MAX) {
    if (myoffset < g2) {
      sc_allgather_recursive (mpicomm, data, datasize, g2, myoffset, myrank);

      mpiret = sc_MPI_Irecv (data + g2 * datasize, g2B * datasize,
                             sc_MPI_BYTE, myrank + g2, SC_TAG_AG_RECURSIVE_B,
                             mpicomm, request + 0);
      SC_CHECK_MPI (mpiret);

      mpiret = sc_MPI_Isend (data, g2 * datasize, sc_MPI_BYTE,
                             myrank + g2, SC_TAG_AG_RECURSIVE_A,
                             mpicomm, request + 1);
      SC_CHECK_MPI (mpiret);

      if (myoffset == g2 - 1 && g2 != g2B) {
        mpiret = sc_MPI_Isend (data, g2 * datasize, sc_MPI_BYTE,
                               myrank + g2B, SC_TAG_AG_RECURSIVE_C,
                               mpicomm, request + 2);
        SC_CHECK_MPI (mpiret);
      }
      else {
        request[2] = sc_MPI_REQUEST_NULL;
      }
    }
    else {
      sc_allgather_recursive (mpicomm, data + g2 * datasize, datasize, g2B,
                              myoffset - g2, myrank);

      if (myoffset == groupsize - 1 && g2 != g2B) {
        request[0] = sc_MPI_REQUEST_NULL;
        request[1] = sc_MPI_REQUEST_NULL;

        mpiret = sc_MPI_Irecv (data, g2 * datasize, sc_MPI_BYTE,
                               myrank - g2B, SC_TAG_AG_RECURSIVE_C,
                               mpicomm, request + 2);
        SC_CHECK_MPI (mpiret);
      }
      else {
        mpiret = sc_MPI_Irecv (data, g2 * datasize, sc_MPI_BYTE,
                               myrank - g2, SC_TAG_AG_RECURSIVE_A,
                               mpicomm, request + 0);
        SC_CHECK_MPI (mpiret);

        mpiret = sc_MPI_Isend (data + g2 * datasize, g2B * datasize,
                               sc_MPI_BYTE, myrank - g2,
                               SC_TAG_AG_RECURSIVE_B, mpicomm, request + 1);
        SC_CHECK_MPI (mpiret);

        request[2] = sc_MPI_REQUEST_NULL;
      }
    }

    mpiret = sc_MPI_Waitall (3, request, sc_MPI_STATUSES_IGNORE);
    SC_CHECK_MPI (mpiret);
  }
  else {
    sc_allgather_alltoall (mpicomm, data, datasize, groupsize, myoffset,
                           myrank);
  }
}

int
sc_allgather (void *sendbuf, int sendcount, sc_MPI_Datatype sendtype,
              void *recvbuf, int recvcount, sc_MPI_Datatype recvtype,
              sc_MPI_Comm mpicomm)
{
  int                 mpiret;
  int                 mpisize;
  int                 mpirank;
  size_t              datasize;
#ifdef SC_DEBUG
  size_t              datasize2;
#endif

  SC_ASSERT (sendcount >= 0 && recvcount >= 0);

  /* *INDENT-OFF* HORRIBLE indent bug */
  datasize = (size_t) sendcount * sc_mpi_sizeof (sendtype);
#ifdef SC_DEBUG
  datasize2 = (size_t) recvcount * sc_mpi_sizeof (recvtype);
#endif
  /* *INDENT-ON* */

  SC_ASSERT (datasize == datasize2);

  mpiret = sc_MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  memcpy (((char *) recvbuf) + mpirank * datasize, sendbuf, datasize);
  sc_allgather_recursive (mpicomm, (char *) recvbuf, (int) datasize,
                          mpisize, mpirank, mpirank);

  return sc_MPI_SUCCESS;
}

void
sc_allgather_final_create_default(void *sendbuf, int sendcount, sc_MPI_Datatype sendtype,
                                  void **recvbuf, int recvcount, sc_MPI_Datatype recvtype,
                                  sc_MPI_Comm mpicomm)
{
  size_t typesize;
  int  mpiret, size;
  char *recvchar;

  mpiret = sc_MPI_Comm_size (mpicomm, &size);
  SC_CHECK_MPI(mpiret);

  typesize = sc_mpi_sizeof (recvtype);
  recvchar = SC_ALLOC(char,size * recvcount * typesize);

  mpiret = sc_MPI_Allgather(sendbuf,sendcount,sendtype,recvchar,recvcount,recvtype,mpicomm);
  SC_CHECK_MPI(mpiret);

  *recvbuf = (void *) recvchar;
}

void
sc_allgather_final_destroy_default(void *recvbuf, sc_MPI_Comm mpicomm)
{
  SC_FREE(recvbuf);
}

sc_allgather_final_create_t sc_allgather_final_create = sc_allgather_final_create_default;
sc_allgather_final_destroy_t sc_allgather_final_destroy = sc_allgather_final_destroy_default;

/** implement sc_allgather_final when nodes have a shared address space, so
 * that raw pointers can be passed and dereferenced in nodes */
void
sc_allgather_final_create_shared(void *sendbuf, int sendcount, sc_MPI_Datatype sendtype,
                                 void **recvbuf, int recvcount, sc_MPI_Datatype recvtype,
                                 sc_MPI_Comm mpicomm)
{
  MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;
  size_t typesize;
  int  mpiret, size, interrank, intrarank, intrasize;
  char *noderecvchar = NULL;

  typesize = sc_mpi_sizeof (recvtype);

  sc_mpi_comm_get_node_comms(mpicomm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    sc_allgather_final_create_default(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,mpicomm);
    return;
  }

  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(intranode,&intrasize);
  SC_CHECK_MPI(mpiret);
  /* node root gathers from node */
  if (!intrarank) {
    noderecvchar = SC_ALLOC (char,intrasize * recvcount * typesize);
  }
  mpiret = sc_MPI_Gather (sendbuf,sendcount,sendtype,noderecvchar,recvcount,recvtype,0,intranode);
  SC_CHECK_MPI(mpiret);

  /* node root allgathers between nodes */
  if (!intrarank) {
    sc_allgather_final_create_default(noderecvchar,sendcount*intrasize,sendtype,recvbuf,recvcount*intrasize,
                                      recvtype, internode);
    SC_FREE(noderecvchar);
  }
  else {
    *recvbuf = NULL;
  }

  /* node root broadcast array start in node */
  mpiret = sc_MPI_Bcast(recvbuf,sizeof(char *),sc_MPI_BYTE,0,intranode);
  SC_CHECK_MPI(mpiret);
}

void
sc_allgather_final_destroy_shared(void *recvbuf, sc_MPI_Comm mpicomm)
{
  MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;
  int  mpiret, size, intersize, interrank, intrarank, intrasize;
  char *noderecvchar = NULL;

  sc_mpi_comm_get_node_comms(mpicomm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    sc_allgather_final_destroy_default(recvbuf,mpicomm);
    return;
  }
  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  if (!intrarank) {
    SC_FREE(recvbuf);
  }
}

void
sc_allgather_final_create_window(void *sendbuf, int sendcount, sc_MPI_Datatype sendtype,
                                 void **recvbuf, int recvcount, sc_MPI_Datatype recvtype,
                                 sc_MPI_Comm mpicomm)
{
#if defined(SC_ENABLE_MPIWINSHARED)
  MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;
  MPI_Win win;
  int typesize;
  int disp_unit;
  int  mpiret, size, intersize, interrank, intrarank, intrasize;
  char *noderecvchar = NULL;
  MPI_Aint winsize = 0;

  typesize = (int) sc_mpi_sizeof (recvtype);

  sc_mpi_comm_get_node_comms(mpicomm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    sc_allgather_final_create_default(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,mpicomm);
    return;
  }

  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(intranode,&intrasize);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(internode,&intersize);
  SC_CHECK_MPI(mpiret);
  /* node root gathers from node */
  if (!intrarank) {
    noderecvchar = SC_ALLOC (char,intrasize * recvcount * typesize);
  }
  mpiret = sc_MPI_Gather (sendbuf,sendcount,sendtype,noderecvchar,recvcount,recvtype,0,intranode);
  SC_CHECK_MPI(mpiret);

  /* create the shared array */
  disp_unit = SC_MAX(typesize,sizeof (MPI_Win));
  if (!intrarank) {
    winsize = intrasize * intersize * recvcount * typesize + intrasize * sizeof (MPI_Win);
    if (winsize % disp_unit) {
      winsize = ((winsize / disp_unit) + 1) * disp_unit;
    }
  }
  mpiret = MPI_Win_allocate_shared(winsize,disp_unit,MPI_INFO_NULL,intranode,recvbuf,&win);
  SC_CHECK_MPI(mpiret);
  mpiret = MPI_Win_shared_query(win,0,&winsize,&disp_unit,recvbuf);
  SC_CHECK_MPI(mpiret);
  /* store the windows at the front of the array */
  mpiret = sc_MPI_Gather (&win,sizeof(MPI_Win),sc_MPI_BYTE,
                          *recvbuf,sizeof(MPI_Win),sc_MPI_BYTE,
                          0,intranode);
  SC_CHECK_MPI(mpiret);
  {
    MPI_Win *shiftedbuf = ((MPI_Win *) *recvbuf) + intrasize;
    *recvbuf = (void *) shiftedbuf;
  }
  /* node root allgathers between nodes */
  if (!intrarank) {
    mpiret = MPI_Win_lock(MPI_LOCK_EXCLUSIVE,0,MPI_MODE_NOCHECK,win);
    SC_CHECK_MPI(mpiret);
    mpiret = sc_MPI_Allgather(noderecvchar,intrasize*recvcount,recvtype,
                              *recvbuf,intrasize*recvcount,recvtype,internode);
    SC_CHECK_MPI(mpiret);
    mpiret = MPI_Win_unlock(0,win);
    SC_CHECK_MPI(mpiret);
    SC_FREE(noderecvchar);
  }
  mpiret = sc_MPI_Barrier(intranode);
  SC_CHECK_MPI(mpiret);

  /* open access */
  mpiret = MPI_Win_lock(MPI_LOCK_SHARED,0,MPI_MODE_NOCHECK,win);
  SC_CHECK_MPI(mpiret);
#else
  sc_allgather_final_create_default(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,mpicomm);
#endif
}

void
sc_allgather_final_destroy_window(void *recvbuf, sc_MPI_Comm mpicomm)
{
#if defined(SC_ENABLE_MPIWINSHARED)
  MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;
  int  mpiret, size, intersize, interrank, intrarank, intrasize;
  char *noderecvchar = NULL;
  MPI_Win win;

  sc_mpi_comm_get_node_comms(mpicomm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    sc_allgather_final_destroy_default(recvbuf,mpicomm);
    return;
  }
  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(intranode,&intrasize);
  SC_CHECK_MPI(mpiret);

  /* get the stashed window */
  win = ((MPI_Win *) recvbuf)[-intrasize + intrarank];

  mpiret = MPI_Win_unlock(0,win);
  SC_CHECK_MPI(mpiret);

  mpiret = MPI_Win_free(&win);
#else
  sc_allgather_final_destroy_default(recvbuf,mpicomm);
#endif
}
