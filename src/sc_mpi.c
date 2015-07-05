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

/* including sc_mpi.h does not work here since sc_mpi.h is included by sc.h */
#include <sc.h>

#ifndef SC_ENABLE_MPI

/* gettimeofday is in either of these two */
#ifdef SC_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef SC_HAVE_TIME_H
#include <time.h>
#endif

static inline void
mpi_dummy_assert_op (sc_MPI_Op op)
{
  switch (op) {
  case sc_MPI_MAX:
  case sc_MPI_MIN:
  case sc_MPI_SUM:
  case sc_MPI_PROD:
  case sc_MPI_LAND:
  case sc_MPI_BAND:
  case sc_MPI_LOR:
  case sc_MPI_BOR:
  case sc_MPI_LXOR:
  case sc_MPI_BXOR:
  case sc_MPI_MINLOC:
  case sc_MPI_MAXLOC:
  case sc_MPI_REPLACE:
    break;
  default:
    SC_ABORT_NOT_REACHED ();
  }
}

int
sc_MPI_Init (int *argc, char ***argv)
{
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Init_thread (int *argc, char ***argv, int required, int *provided)
{
  if (provided != NULL) {
    *provided = sc_MPI_THREAD_SINGLE;
  }
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Finalize (void)
{
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Abort (sc_MPI_Comm comm, int exitcode)
{
  abort ();
}

int
sc_MPI_Comm_dup (sc_MPI_Comm comm, sc_MPI_Comm * newcomm)
{
  *newcomm = comm;

  return sc_MPI_SUCCESS;
}

int
sc_MPI_Comm_free (sc_MPI_Comm * comm)
{
  *comm = sc_MPI_COMM_NULL;

  return sc_MPI_SUCCESS;
}

int
sc_MPI_Comm_size (sc_MPI_Comm comm, int *size)
{
  *size = 1;

  return sc_MPI_SUCCESS;
}

int
sc_MPI_Comm_rank (sc_MPI_Comm comm, int *rank)
{
  *rank = 0;

  return sc_MPI_SUCCESS;
}

int
sc_MPI_Barrier (sc_MPI_Comm comm)
{
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Bcast (void *p, int n, sc_MPI_Datatype t, int rank, sc_MPI_Comm comm)
{
  SC_ASSERT (rank == 0);

  return sc_MPI_SUCCESS;
}

int
sc_MPI_Gather (void *p, int np, sc_MPI_Datatype tp,
               void *q, int nq, sc_MPI_Datatype tq, int rank,
               sc_MPI_Comm comm)
{
  size_t              lp, lq;

  SC_ASSERT (rank == 0 && np >= 0 && nq >= 0);

/* *INDENT-OFF* horrible indent bug */
  lp = (size_t) np * sc_mpi_sizeof (tp);
  lq = (size_t) nq * sc_mpi_sizeof (tq);
/* *INDENT-ON* */

  SC_ASSERT (lp == lq);
  memcpy (q, p, lp);

  return sc_MPI_SUCCESS;
}

int
sc_MPI_Gatherv (void *p, int np, sc_MPI_Datatype tp,
                void *q, int *recvc, int *displ,
                sc_MPI_Datatype tq, int rank, sc_MPI_Comm comm)
{
  int                 nq;
  size_t              lp, lq;

  nq = recvc[0];
  SC_ASSERT (rank == 0 && np >= 0 && nq >= 0);

/* *INDENT-OFF* horrible indent bug */
  lp = (size_t) np * sc_mpi_sizeof (tp);
  lq = (size_t) nq * sc_mpi_sizeof (tq);
/* *INDENT-ON* */

  SC_ASSERT (lp == lq);
  memcpy ((char *) q + displ[0] * sc_mpi_sizeof (tq), p, lp);

  return sc_MPI_SUCCESS;
}

int
sc_MPI_Allgather (void *p, int np, sc_MPI_Datatype tp,
                  void *q, int nq, sc_MPI_Datatype tq, sc_MPI_Comm comm)
{
  return sc_MPI_Gather (p, np, tp, q, nq, tq, 0, comm);
}

int
sc_MPI_Allgatherv (void *p, int np, sc_MPI_Datatype tp,
                   void *q, int *recvc, int *displ,
                   sc_MPI_Datatype tq, sc_MPI_Comm comm)
{
  return sc_MPI_Gatherv (p, np, tp, q, recvc, displ, tq, 0, comm);
}

int
sc_MPI_Reduce (void *p, void *q, int n, sc_MPI_Datatype t,
               sc_MPI_Op op, int rank, sc_MPI_Comm comm)
{
  size_t              l;

  SC_ASSERT (rank == 0 && n >= 0);
  mpi_dummy_assert_op (op);

/* *INDENT-OFF* horrible indent bug */
  l = (size_t) n * sc_mpi_sizeof (t);
/* *INDENT-ON* */

  memcpy (q, p, l);

  return sc_MPI_SUCCESS;
}

int
sc_MPI_Allreduce (void *p, void *q, int n, sc_MPI_Datatype t,
                  sc_MPI_Op op, sc_MPI_Comm comm)
{
  return sc_MPI_Reduce (p, q, n, t, op, 0, comm);
}

int
sc_MPI_Scan (void *sendbuf, void *recvbuf, int count,
             sc_MPI_Datatype datatype, sc_MPI_Op op, sc_MPI_Comm comm)
{
  return sc_MPI_Reduce (sendbuf, recvbuf, count, datatype, op, 0, comm);
}

/* Exscan recvbuf undefined on proc 0 */
int
sc_MPI_Exscan (void *sendbuf, void *recvbf, int count,
               sc_MPI_Datatype datatype, sc_MPI_Op op, sc_MPI_Comm comm)
{
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Recv (void *buf, int count, sc_MPI_Datatype datatype, int source,
             int tag, sc_MPI_Comm comm, sc_MPI_Status * status)
{
  SC_ABORT ("non-MPI MPI_Recv is not implemented");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Irecv (void *buf, int count, sc_MPI_Datatype datatype, int source,
              int tag, sc_MPI_Comm comm, sc_MPI_Request * request)
{
  SC_ABORT ("non-MPI MPI_Irecv is not implemented");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Send (void *buf, int count, sc_MPI_Datatype datatype,
             int dest, int tag, sc_MPI_Comm comm)
{
  SC_ABORT ("non-MPI MPI_Send is not implemented");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Isend (void *buf, int count, sc_MPI_Datatype datatype, int dest,
              int tag, sc_MPI_Comm comm, sc_MPI_Request * request)
{
  SC_ABORT ("non-MPI MPI_Isend is not implemented");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Probe (int source, int tag, sc_MPI_Comm comm, sc_MPI_Status * status)
{
  SC_ABORT ("non-MPI MPI_Probe is not implemented");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Iprobe (int source, int tag, sc_MPI_Comm comm, int *flag,
               sc_MPI_Status * status)
{
  SC_ABORT ("non-MPI MPI_Iprobe is not implemented");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Get_count (sc_MPI_Status * status, sc_MPI_Datatype datatype,
                  int *count)
{
  SC_ABORT ("non-MPI MPI_Get_count is not implemented");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Wait (sc_MPI_Request * request, sc_MPI_Status * status)
{
  SC_CHECK_ABORT (*request == sc_MPI_REQUEST_NULL,
                  "non-MPI MPI_Wait handles NULL request only");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Waitsome (int incount, sc_MPI_Request * array_of_requests,
                 int *outcount, int *array_of_indices,
                 sc_MPI_Status * array_of_statuses)
{
  int                 i;

  for (i = 0; i < incount; ++i) {
    SC_CHECK_ABORT (array_of_requests[i] == sc_MPI_REQUEST_NULL,
                    "non-MPI MPI_Waitsome handles NULL requests only");
  }
  *outcount = 0;

  return sc_MPI_SUCCESS;
}

int
sc_MPI_Waitall (int count, sc_MPI_Request * array_of_requests,
                sc_MPI_Status * array_of_statuses)
{
  int                 i;

  for (i = 0; i < count; ++i) {
    SC_CHECK_ABORT (array_of_requests[i] == sc_MPI_REQUEST_NULL,
                    "non-MPI MPI_Waitall handles NULL requests only");
  }
  return sc_MPI_SUCCESS;
}

double
sc_MPI_Wtime (void)
{
  int                 retval;
  struct timeval      tv;

  retval = gettimeofday (&tv, NULL);
  SC_CHECK_ABORT (retval == 0, "gettimeofday");

  return (double) tv.tv_sec + 1.e-6 * tv.tv_usec;
}

#else /* SC_ENABLE_MPI */
#ifndef SC_ENABLE_MPITHREAD

/* default to non-threaded operation */

int
sc_MPI_Init_thread (int *argc, char ***argv, int required, int *provided)
{
  if (provided != NULL) {
    *provided = sc_MPI_THREAD_SINGLE;
  }
  return sc_MPI_Init (argc, argv);
}

#endif /* !SC_ENABLE_MPITHREAD */
#endif /* SC_ENABLE_MPI */

size_t
sc_mpi_sizeof (sc_MPI_Datatype t)
{
  if (t == sc_MPI_CHAR)
    return sizeof (char);
  if (t == sc_MPI_BYTE)
    return 1;
  if (t == sc_MPI_SHORT || t == sc_MPI_UNSIGNED_SHORT)
    return sizeof (short);
  if (t == sc_MPI_INT || t == sc_MPI_UNSIGNED)
    return sizeof (int);
  if (t == sc_MPI_LONG || t == sc_MPI_UNSIGNED_LONG)
    return sizeof (long);
  if (t == sc_MPI_LONG_LONG_INT)
    return sizeof (long long);
  if (t == sc_MPI_FLOAT)
    return sizeof (float);
  if (t == sc_MPI_DOUBLE)
    return sizeof (double);
  if (t == sc_MPI_LONG_DOUBLE)
    return sizeof (long double);

  SC_ABORT_NOT_REACHED ();
}

#if defined(SC_ENABLE_MPI)
/* these should be initialized in sc_init() */
static int          sc_mpi_node_comm_keyval = MPI_KEYVAL_INVALID;

static int
sc_mpi_node_comms_destroy (MPI_Comm comm, int comm_keyval,
                           void *attribute_val, void *extra_state)
{
  int                 mpiret;
  MPI_Comm           *node_comms = (MPI_Comm *) attribute_val;

  mpiret = MPI_Comm_free (&node_comms[0]);
  if (mpiret != MPI_SUCCESS) {
    return mpiret;
  }
  mpiret = MPI_Comm_free (&node_comms[1]);
  if (mpiret != MPI_SUCCESS) {
    return mpiret;
  }
  mpiret = MPI_Free_mem (node_comms);

  return MPI_SUCCESS;
}

static int
sc_mpi_node_comms_copy (MPI_Comm oldcomm, int comm_keyval,
                        void *extra_state,
                        void *attribute_val_in,
                        void *attribute_val_out, int *flag)
{
  MPI_Comm           *node_comms_in = (MPI_Comm *) attribute_val_in;
  MPI_Comm           *node_comms_out;
  int                 mpiret;

  /* We can't used SC_ALLOC because these might be destroyed after
   * sc finalizes */
  mpiret =
    MPI_Alloc_mem (2 * sizeof (MPI_Comm), MPI_INFO_NULL, &node_comms_out);
  if (mpiret != MPI_SUCCESS) {
    return mpiret;
  }

  mpiret = MPI_Comm_dup (node_comms_in[0], &node_comms_out[0]);
  if (mpiret != MPI_SUCCESS) {
    return mpiret;
  }
  mpiret = MPI_Comm_dup (node_comms_in[1], &node_comms_out[1]);
  if (mpiret != MPI_SUCCESS) {
    return mpiret;
  }

  *((MPI_Comm **) attribute_val_out) = node_comms_out;
  *flag = 1;

  return MPI_SUCCESS;
}
#endif

void
sc_mpi_comm_attach_node_comms (sc_MPI_Comm comm, int processes_per_node)
{
#if defined(SC_ENABLE_MPI)
  int                 keyval, mpiret, rank, size;
  MPI_Comm           *node_comms, internode, intranode;

  if (sc_mpi_node_comm_keyval == MPI_KEYVAL_INVALID) {
    /* register the node comm attachment with MPI */
    mpiret =
      MPI_Comm_create_keyval (MPI_COMM_NULL_COPY_FN,
                              sc_mpi_node_comms_destroy,
                              &sc_mpi_node_comm_keyval, NULL);
    SC_CHECK_MPI (mpiret);
  }
  SC_ASSERT (sc_mpi_node_comm_keyval != MPI_KEYVAL_INVALID);

  mpiret = MPI_Comm_size (comm, &size);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Comm_rank (comm, &rank);
  SC_CHECK_MPI (mpiret);

  if (processes_per_node < 1) {
#if !defined(SC_ENABLE_MPICOMMSHARED)
    SC_ABORT
      ("Require MPI-3 or greater to automatically determine node communicators");
#else
    int                 intrasize, intrarank, maxintrasize, minintrasize;

    mpiret =
      MPI_Comm_split_type (comm, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL,
                           &intranode);
    SC_CHECK_MPI (mpiret);

    /* We only accept node comms if they are all the same size */
    mpiret = MPI_Comm_size (intranode, &intrasize);
    SC_CHECK_MPI (mpiret);
    mpiret = MPI_Comm_rank (intranode, &intrarank);
    SC_CHECK_MPI (mpiret);

    mpiret =
      MPI_Allreduce (&intrasize, &maxintrasize, 1, MPI_INT, MPI_MAX, comm);
    SC_CHECK_MPI (mpiret);
    mpiret =
      MPI_Allreduce (&intrasize, &minintrasize, 1, MPI_INT, MPI_MIN, comm);
    SC_CHECK_MPI (mpiret);

    if (maxintrasize != minintrasize) {
      SC_GLOBAL_LDEBUG
        ("node communicators are not the same size: not attaching\n");

      mpiret = MPI_Comm_free (&intrasize);
      SC_CHECK_MPI (mpiret);

      return;
    }

    mpiret = MPI_Comm_split (comm, intrarank, rank, &internode);
    SC_CHECK_MPI (mpiret);
#endif
  }
  else {
    int                 node, offset;

    SC_ASSERT (!(size % processes_per_node));

    node = rank / processes_per_node;
    offset = rank % processes_per_node;

    mpiret = MPI_Comm_split (comm, node, offset, &intranode);
    SC_CHECK_MPI (mpiret);

    mpiret = MPI_Comm_split (comm, offset, node, &internode);
    SC_CHECK_MPI (mpiret);
  }

  /* We can't used SC_ALLOC because these might be destroyed after
   * sc finalizes */
  mpiret = MPI_Alloc_mem (2 * sizeof (MPI_Comm), MPI_INFO_NULL, &node_comms);
  SC_CHECK_MPI (mpiret);
  node_comms[0] = intranode;
  node_comms[1] = internode;

  mpiret = MPI_Comm_set_attr (comm, sc_mpi_node_comm_keyval, node_comms);
  SC_CHECK_MPI (mpiret);
#endif
}

void
sc_mpi_comm_get_node_comms (sc_MPI_Comm comm,
                            sc_MPI_Comm * intranode, sc_MPI_Comm * internode)
{
  int                 mpiret, flag;
  MPI_Comm           *node_comms;
  void               *intraval, *interval;

  *intranode = sc_MPI_COMM_NULL;
  *internode = sc_MPI_COMM_NULL;
#if defined(SC_ENABLE_MPI)
  if (sc_mpi_node_comm_keyval == MPI_KEYVAL_INVALID) {
    SC_GLOBAL_LDEBUG
      ("Asking for node comms before sc_mpi_comm_attach_node_comms is called\n");
    return;
  }
  mpiret =
    MPI_Comm_get_attr (comm, sc_mpi_node_comm_keyval, &node_comms, &flag);
  SC_CHECK_MPI (mpiret);
  if (flag && node_comms) {
    *intranode = (MPI_Comm) node_comms[0];
    *internode = (MPI_Comm) node_comms[1];
  }
#endif
}
