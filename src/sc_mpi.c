/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2008,2009 Carsten Burstedde, Lucas Wilcox.

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
mpi_dummy_assert_op (MPI_Op op)
{
  switch (op) {
  case MPI_MAX:
  case MPI_MIN:
  case MPI_SUM:
  case MPI_PROD:
  case MPI_LAND:
  case MPI_BAND:
  case MPI_LOR:
  case MPI_BOR:
  case MPI_LXOR:
  case MPI_BXOR:
  case MPI_MINLOC:
  case MPI_MAXLOC:
  case MPI_REPLACE:
    break;
  default:
    SC_CHECK_NOT_REACHED ();
  }
}

int
MPI_Init (int *argc, char ***argv)
{
  return MPI_SUCCESS;
}

int
MPI_Finalize (void)
{
  return MPI_SUCCESS;
}

int
MPI_Abort (MPI_Comm comm, int exitcode)
{
  abort ();
}

int
MPI_Comm_size (MPI_Comm comm, int *size)
{
  *size = 1;

  return MPI_SUCCESS;
}

int
MPI_Comm_rank (MPI_Comm comm, int *rank)
{
  *rank = 0;

  return MPI_SUCCESS;
}

int
MPI_Barrier (MPI_Comm comm)
{
  return MPI_SUCCESS;
}

int
MPI_Bcast (void *p, int n, MPI_Datatype t, int rank, MPI_Comm comm)
{
  SC_ASSERT (rank == 0);

  return MPI_SUCCESS;
}

int
MPI_Gather (void *p, int np, MPI_Datatype tp,
            void *q, int nq, MPI_Datatype tq, int rank, MPI_Comm comm)
{
  size_t              lp, lq;

  SC_ASSERT (rank == 0 && np >= 0 && nq >= 0);

/* *INDENT-OFF* horrible indent bug */
  lp = (size_t) np * sc_mpi_sizeof (tp);
  lq = (size_t) nq * sc_mpi_sizeof (tq);
/* *INDENT-ON* */

  SC_ASSERT (lp == lq);
  memcpy (q, p, lp);

  return MPI_SUCCESS;
}

int
MPI_Gatherv (void *p, int np, MPI_Datatype tp,
             void *q, int *recvc, int *displ,
             MPI_Datatype tq, int rank, MPI_Comm comm)
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

  return MPI_SUCCESS;
}

int
MPI_Allgather (void *p, int np, MPI_Datatype tp,
               void *q, int nq, MPI_Datatype tq, MPI_Comm comm)
{
  return MPI_Gather (p, np, tp, q, nq, tq, 0, comm);
}

int
MPI_Allgatherv (void *p, int np, MPI_Datatype tp,
                void *q, int *recvc, int *displ,
                MPI_Datatype tq, MPI_Comm comm)
{
  return MPI_Gatherv (p, np, tp, q, recvc, displ, tq, 0, comm);
}

int
MPI_Reduce (void *p, void *q, int n, MPI_Datatype t,
            MPI_Op op, int rank, MPI_Comm comm)
{
  size_t              l;

  SC_ASSERT (rank == 0 && n >= 0);
  mpi_dummy_assert_op (op);

/* *INDENT-OFF* horrible indent bug */
  l = (size_t) n * sc_mpi_sizeof (t);
/* *INDENT-ON* */

  memcpy (q, p, l);

  return MPI_SUCCESS;
}

int
MPI_Allreduce (void *p, void *q, int n, MPI_Datatype t,
               MPI_Op op, MPI_Comm comm)
{
  return MPI_Reduce (p, q, n, t, op, 0, comm);
}

int
MPI_Recv (void *buf, int count, MPI_Datatype datatype, int source, int tag,
          MPI_Comm comm, MPI_Status * status)
{
  SC_CHECK_ABORT (false, "MPI_Recv is not implemented");
  return MPI_SUCCESS;
}

int
MPI_Irecv (void *buf, int count, MPI_Datatype datatype, int source, int tag,
           MPI_Comm comm, MPI_Request * request)
{
  SC_CHECK_ABORT (false, "MPI_Irecv is not implemented");
  return MPI_SUCCESS;
}

int
MPI_Send (void *buf, int count, MPI_Datatype datatype,
          int dest, int tag, MPI_Comm comm)
{
  SC_CHECK_ABORT (false, "MPI_Send is not implemented");
  return MPI_SUCCESS;
}

int
MPI_Isend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
           MPI_Comm comm, MPI_Request * request)
{
  SC_CHECK_ABORT (false, "MPI_Isend is not implemented");
  return MPI_SUCCESS;
}

int
MPI_Probe (int source, int tag, MPI_Comm comm, MPI_Status * status)
{
  SC_CHECK_ABORT (false, "MPI_Probe is not implemented");
  return MPI_SUCCESS;
}

int
MPI_Iprobe (int source, int tag, MPI_Comm comm, int *flag,
            MPI_Status * status)
{
  SC_CHECK_ABORT (false, "MPI_Iprobe is not implemented");
  return MPI_SUCCESS;
}

int
MPI_Get_count (MPI_Status * status, MPI_Datatype datatype, int *count)
{
  SC_CHECK_ABORT (false, "MPI_Get_count is not implemented");
  return MPI_SUCCESS;
}

int
MPI_Waitsome (int incount, MPI_Request * array_of_requests,
              int *outcount, int *array_of_indices,
              MPI_Status * array_of_statuses)
{
  SC_CHECK_ABORT (incount == 0, "MPI_Waitsome handles zero requests only");
  return MPI_SUCCESS;
}

int
MPI_Waitall (int count, MPI_Request * array_of_requests,
             MPI_Status * array_of_statuses)
{
  SC_CHECK_ABORT (count == 0, "MPI_Waitall handles zero requests only");
  return MPI_SUCCESS;
}

double
MPI_Wtime (void)
{
  int                 retval;
  struct timeval      tv;

  retval = gettimeofday (&tv, NULL);
  SC_CHECK_ABORT (retval == 0, "gettimeofday");

  return (double) tv.tv_sec + 1.e-6 * tv.tv_usec;
}

#endif /* !SC_MPI */

size_t
sc_mpi_sizeof (MPI_Datatype t)
{
  if (t == MPI_CHAR)
    return sizeof (char);
  if (t == MPI_BYTE)
    return 1;
  if (t == MPI_SHORT || t == MPI_UNSIGNED_SHORT)
    return sizeof (short);
  if (t == MPI_INT || t == MPI_UNSIGNED)
    return sizeof (int);
  if (t == MPI_LONG || t == MPI_UNSIGNED_LONG)
    return sizeof (long);
  if (t == MPI_FLOAT)
    return sizeof (float);
  if (t == MPI_DOUBLE)
    return sizeof (double);
  if (t == MPI_LONG_DOUBLE)
    return sizeof (long double);
  if (t == MPI_LONG_LONG_INT)
    return sizeof (long long);

  SC_CHECK_NOT_REACHED ();
}
