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

#include <sc_reduce.h>
#include <sc_search.h>

#ifdef SC_MPI

static void
sc_reduce_recursive (MPI_Comm mpicomm, char *data, int datasize,
		     int groupsize, int myrank, int targetrank,
		     int maxlevel, int level, int branch)
{
  int                 mpiret;
  int                 peer, higher;
  char               *peerdata;
  MPI_Status          rstatus;

  SC_ASSERT (myrank ==
	     sc_search_bias (maxlevel, level, branch, targetrank));
  SC_ASSERT (myrank < groupsize);
  SC_ASSERT (targetrank < groupsize);

  if (level == 0) {
    /* result is in data */
  }
  else {
    peer = sc_search_bias (maxlevel, level, branch ^ 0x01, targetrank);
    SC_ASSERT (peer != myrank);

    higher = sc_search_bias (maxlevel, level - 1, branch / 2, targetrank);
    if (myrank == higher) {
      if (peer < groupsize) {
	/* temporary data to compare against peer */
	peerdata = SC_ALLOC (char, datasize);

	mpiret = MPI_Recv (peerdata, datasize, MPI_BYTE,
			   peer, SC_TAG_REDUCE, mpicomm, &rstatus);
	SC_CHECK_MPI (mpiret);

	/* run reduce operation here and write result into data */
	SC_FREE (peerdata);

	mpiret = MPI_Send (data, datasize, MPI_BYTE,
			   peer, SC_TAG_REDUCE, mpicomm);
	SC_CHECK_MPI (mpiret);
      }
    }
    else {
      if (peer < groupsize) {
	mpiret = MPI_Send (data, datasize, MPI_BYTE,
			   peer, SC_TAG_REDUCE, mpicomm);
	SC_CHECK_MPI (mpiret);
	mpiret = MPI_Recv (data, datasize, MPI_BYTE,
			   peer, SC_TAG_REDUCE, mpicomm, &rstatus);
	SC_CHECK_MPI (mpiret);
      }
    }
  }
}

#endif /* SC_MPI */

int
sc_reduce (void *sendbuf, void *recvbuf, int sendcount,
	   MPI_Datatype sendtype, MPI_Op operation,
	   int rank, MPI_Comm mpicomm)
{
#ifdef SC_MPI
  int                 mpiret;
  int                 mpisize;
  int                 mpirank;
  int                 maxlevel;
#endif
  size_t              datasize;

  SC_ASSERT (sendcount >= 0);

  datasize = (size_t) sendcount * sc_mpi_sizeof (sendtype);
  memcpy (recvbuf, sendbuf, datasize);

#ifdef SC_MPI
  mpiret = MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  maxlevel = SC_LOG2_32 (mpisize - 1) + 1;

  sc_reduce_recursive (mpicomm, recvbuf, datasize,
		       mpisize, mpirank, rank,
		       maxlevel, maxlevel, mpirank);
#endif

  return MPI_SUCCESS;
}

