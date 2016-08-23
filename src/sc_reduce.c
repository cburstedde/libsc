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

#include <sc_reduce.h>
#include <sc_search.h>

static void
sc_reduce_alltoall (sc_MPI_Comm mpicomm,
                    void *data, int count, sc_MPI_Datatype datatype,
                    int groupsize, int target,
                    int maxlevel, int level, int branch,
                    sc_reduce_t reduce_fn)
{
  int                 i, l;
  int                 mpiret;
  int                 doall, allcount;
  int                 myrank, peer, peer2;
  int                 shift;
  char               *alldata;
  size_t              datasize;
  sc_MPI_Request     *request, *rrequest, *srequest;

  doall = 0;
  if (target == -1) {
    doall = 1;
    target = 0;
  }

  SC_ASSERT (0 <= target && target < groupsize);

  myrank = sc_search_bias (maxlevel, level, branch, target);

  SC_ASSERT (0 <= myrank && myrank < groupsize);
  SC_ASSERT (reduce_fn != NULL);

  /* *INDENT-OFF* HORRIBLE indent bug */
  datasize = (size_t) count * sc_mpi_sizeof (datatype);
  /* *INDENT-ON* */

  if (doall || target == myrank) {
    allcount = 1 << level;

    alldata = SC_ALLOC (char, allcount * datasize);
    request = SC_ALLOC (sc_MPI_Request, 2 * allcount);
    rrequest = request;
    srequest = request + allcount;

    for (i = 0; i < allcount; ++i) {
      peer = sc_search_bias (maxlevel, level, i, target);

      /* communicate with existing peers */
      if (peer == myrank) {
        memcpy (alldata + i * datasize, data, datasize);
        rrequest[i] = srequest[i] = sc_MPI_REQUEST_NULL;
      }
      else {
        if (peer < groupsize) {
          mpiret =
            sc_MPI_Irecv (alldata + i * datasize, datasize, sc_MPI_BYTE, peer,
                          SC_TAG_REDUCE, mpicomm, rrequest + i);
          SC_CHECK_MPI (mpiret);
          if (doall) {
            mpiret = sc_MPI_Isend (data, datasize, sc_MPI_BYTE,
                                   peer, SC_TAG_REDUCE, mpicomm,
                                   srequest + i);
            SC_CHECK_MPI (mpiret);
          }
          else {
            srequest[i] = sc_MPI_REQUEST_NULL;  /* unused */
          }
        }
        else {
          /* ignore non-existing ranks greater or equal mpisize */
          rrequest[i] = srequest[i] = sc_MPI_REQUEST_NULL;
        }
      }
    }

    /* complete receive operations */
    mpiret = sc_MPI_Waitall (allcount, rrequest, sc_MPI_STATUSES_IGNORE);
    SC_CHECK_MPI (mpiret);

    /* process received data in the same order as sc_reduce_recursive */
    for (shift = 0, l = level - 1; l >= 0; ++shift, --l) {
      for (i = 0; i < 1 << l; ++i) {
#ifdef SC_DEBUG
        peer = sc_search_bias (maxlevel, l + 1, 2 * i, target);
#endif
        peer2 = sc_search_bias (maxlevel, l + 1, 2 * i + 1, target);
        SC_ASSERT (peer < peer2);

        if (peer2 < groupsize) {
          reduce_fn (alldata + ((2 * i + 1) << shift) * datasize,
                     alldata + ((2 * i) << shift) * datasize,
                     count, datatype);
        }
      }
    }
    memcpy (data, alldata, datasize);
    SC_FREE (alldata);          /* alldata is not used in send buffers */

    /* wait for sends only after computation is done */
    if (doall) {
      mpiret = sc_MPI_Waitall (allcount, srequest, sc_MPI_STATUSES_IGNORE);
      SC_CHECK_MPI (mpiret);
    }
    SC_FREE (request);
  }
  else {
    mpiret = sc_MPI_Send (data, datasize, sc_MPI_BYTE,
                          target, SC_TAG_REDUCE, mpicomm);
    SC_CHECK_MPI (mpiret);
  }
}

static void
sc_reduce_recursive (sc_MPI_Comm mpicomm,
                     void *data, int count, sc_MPI_Datatype datatype,
                     int groupsize, int target,
                     int maxlevel, int level, int branch,
                     sc_reduce_t reduce_fn)
{
  int                 mpiret;
  int                 orig_target, doall;
  int                 myrank, peer, higher;
  char               *peerdata;
  size_t              datasize;
  sc_MPI_Status       rstatus;

  orig_target = target;
  doall = 0;
  if (target == -1) {
    doall = 1;
    target = 0;
  }

  SC_ASSERT (0 <= target && target < groupsize);

  myrank = sc_search_bias (maxlevel, level, branch, target);

  SC_ASSERT (0 <= myrank && myrank < groupsize);
  SC_ASSERT (reduce_fn != NULL);

  if (level == 0) {
    /* result is in data */
  }
  else if (level <= SC_REDUCE_ALLTOALL_LEVEL) {
    /* all-to-all communication */
    sc_reduce_alltoall (mpicomm, data, count, datatype,
                        groupsize, orig_target,
                        maxlevel, level, branch, reduce_fn);
  }
  else {
    /* *INDENT-OFF* HORRIBLE indent bug */
    datasize = (size_t) count * sc_mpi_sizeof (datatype);
    /* *INDENT-ON* */
    peer = sc_search_bias (maxlevel, level, branch ^ 0x01, target);
    SC_ASSERT (peer != myrank);

    higher = sc_search_bias (maxlevel, level - 1, branch / 2, target);
    if (myrank == higher) {
      if (peer < groupsize) {
        /* temporary data to compare against peer */
        peerdata = SC_ALLOC (char, datasize);

        mpiret = sc_MPI_Recv (peerdata, datasize, sc_MPI_BYTE,
                              peer, SC_TAG_REDUCE, mpicomm, &rstatus);
        SC_CHECK_MPI (mpiret);

        /* execute reduction operation here */
        reduce_fn (peerdata, data, count, datatype);
        SC_FREE (peerdata);
      }

      /* execute next higher level of recursion */
      sc_reduce_recursive (mpicomm, data, count, datatype,
                           groupsize, orig_target,
                           maxlevel, level - 1, branch / 2, reduce_fn);

      if (doall && peer < groupsize) {
        /* if allreduce send back result of reduction */
        mpiret = sc_MPI_Send (data, datasize, sc_MPI_BYTE,
                              peer, SC_TAG_REDUCE, mpicomm);
        SC_CHECK_MPI (mpiret);
      }
    }
    else {
      if (peer < groupsize) {
        mpiret = sc_MPI_Send (data, datasize, sc_MPI_BYTE,
                              peer, SC_TAG_REDUCE, mpicomm);
        SC_CHECK_MPI (mpiret);
        if (doall) {
          /* if allreduce receive back result of reduction */
          mpiret = sc_MPI_Recv (data, datasize, sc_MPI_BYTE,
                                peer, SC_TAG_REDUCE, mpicomm, &rstatus);
          SC_CHECK_MPI (mpiret);
        }
      }
    }
  }
}

static void
sc_reduce_max (void *sendbuf, void *recvbuf,
               int sendcount, sc_MPI_Datatype sendtype)
{
  int                 i;

  if (sendtype == sc_MPI_CHAR || sendtype == sc_MPI_BYTE) {
    const char         *s = (char *) sendbuf;
    char               *r = (char *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] > r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_SHORT) {
    const short        *s = (short *) sendbuf;
    short              *r = (short *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] > r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_UNSIGNED_SHORT) {
    const unsigned short *s = (unsigned short *) sendbuf;
    unsigned short     *r = (unsigned short *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] > r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_INT) {
    const int          *s = (int *) sendbuf;
    int                *r = (int *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] > r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_UNSIGNED) {
    const unsigned     *s = (unsigned *) sendbuf;
    unsigned           *r = (unsigned *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] > r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_LONG) {
    const long         *s = (long *) sendbuf;
    long               *r = (long *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] > r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_UNSIGNED_LONG) {
    const unsigned long *s = (unsigned long *) sendbuf;
    unsigned long      *r = (unsigned long *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] > r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_LONG_LONG_INT) {
    const long long    *s = (long long *) sendbuf;
    long long          *r = (long long *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] > r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_FLOAT) {
    const float        *s = (float *) sendbuf;
    float              *r = (float *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] > r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_DOUBLE) {
    const double       *s = (double *) sendbuf;
    double             *r = (double *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] > r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_LONG_DOUBLE) {
    const long double  *s = (long double *) sendbuf;
    long double        *r = (long double *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] > r[i])
        r[i] = s[i];
  }
  else {
    SC_ABORT ("Unsupported MPI datatype in sc_reduce_max");
  }
}

static void
sc_reduce_min (void *sendbuf, void *recvbuf,
               int sendcount, sc_MPI_Datatype sendtype)
{
  int                 i;

  if (sendtype == sc_MPI_CHAR || sendtype == sc_MPI_BYTE) {
    const char         *s = (char *) sendbuf;
    char               *r = (char *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] < r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_SHORT) {
    const short        *s = (short *) sendbuf;
    short              *r = (short *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] < r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_UNSIGNED_SHORT) {
    const unsigned short *s = (unsigned short *) sendbuf;
    unsigned short     *r = (unsigned short *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] < r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_INT) {
    const int          *s = (int *) sendbuf;
    int                *r = (int *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] < r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_UNSIGNED) {
    const unsigned     *s = (unsigned *) sendbuf;
    unsigned           *r = (unsigned *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] < r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_LONG) {
    const long         *s = (long *) sendbuf;
    long               *r = (long *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] < r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_UNSIGNED_LONG) {
    const unsigned long *s = (unsigned long *) sendbuf;
    unsigned long      *r = (unsigned long *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] < r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_LONG_LONG_INT) {
    const long long    *s = (long long *) sendbuf;
    long long          *r = (long long *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] < r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_FLOAT) {
    const float        *s = (float *) sendbuf;
    float              *r = (float *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] < r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_DOUBLE) {
    const double       *s = (double *) sendbuf;
    double             *r = (double *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] < r[i])
        r[i] = s[i];
  }
  else if (sendtype == sc_MPI_LONG_DOUBLE) {
    const long double  *s = (long double *) sendbuf;
    long double        *r = (long double *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      if (s[i] < r[i])
        r[i] = s[i];
  }
  else {
    SC_ABORT ("Unsupported MPI datatype in sc_reduce_min");
  }
}

static void
sc_reduce_sum (void *sendbuf, void *recvbuf,
               int sendcount, sc_MPI_Datatype sendtype)
{
  int                 i;

  if (sendtype == sc_MPI_CHAR || sendtype == sc_MPI_BYTE) {
    const char         *s = (char *) sendbuf;
    char               *r = (char *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      r[i] += s[i];
  }
  else if (sendtype == sc_MPI_SHORT) {
    const short        *s = (short *) sendbuf;
    short              *r = (short *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      r[i] += s[i];
  }
  else if (sendtype == sc_MPI_UNSIGNED_SHORT) {
    const unsigned short *s = (unsigned short *) sendbuf;
    unsigned short     *r = (unsigned short *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      r[i] += s[i];
  }
  else if (sendtype == sc_MPI_INT) {
    const int          *s = (int *) sendbuf;
    int                *r = (int *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      r[i] += s[i];
  }
  else if (sendtype == sc_MPI_UNSIGNED) {
    const unsigned     *s = (unsigned *) sendbuf;
    unsigned           *r = (unsigned *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      r[i] += s[i];
  }
  else if (sendtype == sc_MPI_LONG) {
    const long         *s = (long *) sendbuf;
    long               *r = (long *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      r[i] += s[i];
  }
  else if (sendtype == sc_MPI_UNSIGNED_LONG) {
    const unsigned long *s = (unsigned long *) sendbuf;
    unsigned long      *r = (unsigned long *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      r[i] += s[i];
  }
  else if (sendtype == sc_MPI_LONG_LONG_INT) {
    const long long    *s = (long long *) sendbuf;
    long long          *r = (long long *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      r[i] += s[i];
  }
  else if (sendtype == sc_MPI_FLOAT) {
    const float        *s = (float *) sendbuf;
    float              *r = (float *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      r[i] += s[i];
  }
  else if (sendtype == sc_MPI_DOUBLE) {
    const double       *s = (double *) sendbuf;
    double             *r = (double *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      r[i] += s[i];
  }
  else if (sendtype == sc_MPI_LONG_DOUBLE) {
    const long double  *s = (long double *) sendbuf;
    long double        *r = (long double *) recvbuf;
    for (i = 0; i < sendcount; ++i)
      r[i] += s[i];
  }
  else {
    SC_ABORT ("Unsupported MPI datatype in sc_reduce_sum");
  }
}

static int
sc_reduce_custom_dispatch (void *sendbuf, void *recvbuf, int sendcount,
                           sc_MPI_Datatype sendtype, sc_reduce_t reduce_fn,
                           int target, sc_MPI_Comm mpicomm)
{
  int                 mpiret;
  int                 mpisize;
  int                 mpirank;
  int                 maxlevel;
  size_t              datasize;

  SC_ASSERT (sendcount >= 0);

  /* *INDENT-OFF* HORRIBLE indent bug */
  datasize = (size_t) sendcount * sc_mpi_sizeof (sendtype);
  /* *INDENT-ON* */
  memcpy (recvbuf, sendbuf, datasize);

  mpiret = sc_MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  SC_ASSERT (-1 <= target && target < mpisize);

  maxlevel = SC_LOG2_32 (mpisize - 1) + 1;
  sc_reduce_recursive (mpicomm, recvbuf, sendcount, sendtype, mpisize,
                       target, maxlevel, maxlevel, mpirank, reduce_fn);

  return sc_MPI_SUCCESS;
}

int
sc_allreduce_custom (void *sendbuf, void *recvbuf, int sendcount,
                     sc_MPI_Datatype sendtype, sc_reduce_t reduce_fn,
                     sc_MPI_Comm mpicomm)
{
  return sc_reduce_custom_dispatch (sendbuf, recvbuf, sendcount,
                                    sendtype, reduce_fn, -1, mpicomm);
}

int
sc_reduce_custom (void *sendbuf, void *recvbuf, int sendcount,
                  sc_MPI_Datatype sendtype, sc_reduce_t reduce_fn,
                  int target, sc_MPI_Comm mpicomm)
{
  SC_CHECK_ABORT (target >= 0,
                  "sc_reduce_custom requires non-negative target");

  return sc_reduce_custom_dispatch (sendbuf, recvbuf, sendcount,
                                    sendtype, reduce_fn, target, mpicomm);
}

static int
sc_reduce_dispatch (void *sendbuf, void *recvbuf, int sendcount,
                    sc_MPI_Datatype sendtype, sc_MPI_Op operation,
                    int target, sc_MPI_Comm mpicomm)
{
  sc_reduce_t         reduce_fn;

  if (operation == sc_MPI_MAX)
    reduce_fn = sc_reduce_max;
  else if (operation == sc_MPI_MIN)
    reduce_fn = sc_reduce_min;
  else if (operation == sc_MPI_SUM)
    reduce_fn = sc_reduce_sum;
  else
    SC_ABORT ("Unsupported operation in sc_allreduce or sc_reduce");

  return sc_reduce_custom_dispatch (sendbuf, recvbuf, sendcount,
                                    sendtype, reduce_fn, target, mpicomm);
}

int
sc_allreduce (void *sendbuf, void *recvbuf, int sendcount,
              sc_MPI_Datatype sendtype, sc_MPI_Op operation,
              sc_MPI_Comm mpicomm)
{
  return sc_reduce_dispatch (sendbuf, recvbuf, sendcount,
                             sendtype, operation, -1, mpicomm);
}

int
sc_reduce (void *sendbuf, void *recvbuf, int sendcount,
           sc_MPI_Datatype sendtype, sc_MPI_Op operation,
           int target, sc_MPI_Comm mpicomm)
{
  SC_CHECK_ABORT (target >= 0, "sc_reduce requires non-negative target");

  return sc_reduce_dispatch (sendbuf, recvbuf, sendcount,
                             sendtype, operation, target, mpicomm);
}
