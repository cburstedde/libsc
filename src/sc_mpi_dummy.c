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

/* sc.h comes first in every compilation unit */
#include <sc.h>
#include <sc_mpi_dummy.h>

/* gettimeofday is in either of these two */
#ifdef SC_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef SC_HAVE_TIME_H
#include <time.h>
#endif

static inline       size_t
mpi_dummy_sizeof (MPI_Datatype t)
{
  switch (t) {
  case MPI_CHAR:
  case MPI_SIGNED_CHAR:
  case MPI_UNSIGNED_CHAR:
    return sizeof (char);
  case MPI_BYTE:
    return 1;
  case MPI_SHORT:
  case MPI_UNSIGNED_SHORT:
    return sizeof (short);
  case MPI_INT:
  case MPI_UNSIGNED:
    return sizeof (int);
  case MPI_LONG:
  case MPI_UNSIGNED_LONG:
    return sizeof (long);
  case MPI_FLOAT:
    return sizeof (float);
  case MPI_DOUBLE:
    return sizeof (double);
  case MPI_LONG_DOUBLE:
    return sizeof (long double);
  case MPI_LONG_LONG_INT:
  case MPI_UNSIGNED_LONG_LONG:
    return sizeof (long long);
  default:
    SC_ASSERT_NOT_REACHED ();
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

int
MPI_Abort (MPI_Comm comm, int exitcode)
{
  abort ();
}

/* EOF sc_mpi_dummy.c */
