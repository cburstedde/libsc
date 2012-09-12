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

#ifndef SC_MPI

/* gettimeofday is in either of these two */
#ifdef SC_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef SC_HAVE_TIME_H
#include <time.h>
#endif

static inline void
sc_mpi_dummy_assert_op (sc_MPI_Op op)
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
            void *q, int nq, sc_MPI_Datatype tq, int rank, sc_MPI_Comm comm)
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
  sc_mpi_dummy_assert_op (op);

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
sc_MPI_Recv (void *buf, int count, sc_MPI_Datatype datatype, int source, int tag,
          sc_MPI_Comm comm, sc_MPI_Status * status)
{
  SC_ABORT ("MPI_Recv is not implemented");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Irecv (void *buf, int count, sc_MPI_Datatype datatype, int source, int tag,
           sc_MPI_Comm comm, sc_MPI_Request * request)
{
  SC_ABORT ("MPI_Irecv is not implemented");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Send (void *buf, int count, sc_MPI_Datatype datatype,
          int dest, int tag, sc_MPI_Comm comm)
{
  SC_ABORT ("MPI_Send is not implemented");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Isend (void *buf, int count, sc_MPI_Datatype datatype, int dest, int tag,
           sc_MPI_Comm comm, sc_MPI_Request * request)
{
  SC_ABORT ("MPI_Isend is not implemented");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Probe (int source, int tag, sc_MPI_Comm comm, sc_MPI_Status * status)
{
  SC_ABORT ("MPI_Probe is not implemented");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Iprobe (int source, int tag, sc_MPI_Comm comm, int *flag,
            sc_MPI_Status * status)
{
  SC_ABORT ("MPI_Iprobe is not implemented");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Get_count (sc_MPI_Status * status, sc_MPI_Datatype datatype, int *count)
{
  SC_ABORT ("MPI_Get_count is not implemented");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Wait (sc_MPI_Request * request, sc_MPI_Status * status)
{
  SC_ABORT ("MPI_Wait is not implemented");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Waitsome (int incount, sc_MPI_Request * array_of_requests,
              int *outcount, int *array_of_indices,
              sc_MPI_Status * array_of_statuses)
{
  SC_CHECK_ABORT (incount == 0, "MPI_Waitsome handles zero requests only");
  return sc_MPI_SUCCESS;
}

int
sc_MPI_Waitall (int count, sc_MPI_Request * array_of_requests,
             sc_MPI_Status * array_of_statuses)
{
  SC_CHECK_ABORT (count == 0, "MPI_Waitall handles zero requests only");
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

#endif /* !SC_MPI */

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
