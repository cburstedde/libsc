/*
  This file is part of the SC Library, version 3.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

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

#include <sc3_mpi.h>

#ifndef SC_ENABLE_MPI

static sc3_error_t *
sc3_MPI_Datatype_size (sc3_MPI_Datatype_t datatype, size_t * size)
{
  SC3A_CHECK (size != NULL);
  switch (datatype) {
  case sc3_MPI_BYTE:
    *size = 1;
    return NULL;
  case sc3_MPI_INT:
    *size = sizeof (int);
    return NULL;
  case sc3_MPI_LONG:
    *size = sizeof (long);
    return NULL;
  case sc3_MPI_FLOAT:
    *size = sizeof (float);
    return NULL;
  case sc3_MPI_DOUBLE:
    *size = sizeof (double);
    return NULL;
  default:
    SC3E_UNREACH ("Invalid MPI type");
  }
}

#else /* SC_ENABLE_MPI */

#define SC3E_MPI(f) do {                                                \
  int _mpiret = (f);                                                    \
  if (_mpiret != sc3_MPI_SUCCESS) {                                     \
    char _errorstring[sc3_MPI_MAX_ERROR_STRING];                        \
    int _errlen;                                                        \
    int _mpiret2 = MPI_Error_string (_mpiret, _errorstring, &_errlen);  \
    if (_mpiret2 != sc3_MPI_SUCCESS) {                                  \
      (void) snprintf  (_errorstring, sc3_MPI_MAX_ERROR_STRING,         \
                        "%s", "Unknown MPI error");                     \
    }                                                                   \
    return sc3_error_new_fatal (__FILE__, __LINE__, _errorstring);      \
  }} while (0)

#endif /* SC_ENABLE_MPI */

sc3_error_t        *
sc3_MPI_Init (int *argc, char ***argv)
{
#ifdef SC_ENABLE_MPI
  SC3E_MPI (MPI_Init (argc, argv));
#endif
  return NULL;
}

sc3_error_t        *
sc3_MPI_Finalize (void)
{
#ifdef SC_ENABLE_MPI
  SC3E_MPI (MPI_Finalize ());
#endif
  return NULL;
}

sc3_error_t        *
sc3_MPI_Comm_set_errhandler (sc3_MPI_Comm_t comm, sc3_MPI_Errhandler_t errh)
{
#ifdef SC_ENABLE_MPI
  SC3E_MPI (MPI_Comm_set_errhandler (comm, errh));
#endif
  return NULL;
}

sc3_error_t        *
sc3_MPI_Comm_size (sc3_MPI_Comm_t comm, int *size)
{
  SC3A_CHECK (size != NULL);
#ifndef SC_ENABLE_MPI
  *size = 1;
#else
  SC3E_MPI (MPI_Comm_size (comm, size));
#endif
  return NULL;
}

sc3_error_t        *
sc3_MPI_Comm_rank (sc3_MPI_Comm_t comm, int *rank)
{
  SC3A_CHECK (rank != NULL);
#ifndef SC_ENABLE_MPI
  *rank = 0;
#else
  SC3E_MPI (MPI_Comm_rank (comm, rank));
#endif
  return NULL;
}

sc3_error_t        *
sc3_MPI_Comm_dup (sc3_MPI_Comm_t comm, sc3_MPI_Comm_t * newcomm)
{
  SC3A_CHECK (newcomm != NULL);
#ifndef SC_ENABLE_MPI
  *newcomm = comm;
#else
  SC3E_MPI (MPI_Comm_dup (comm, newcomm));
#endif
  return NULL;
}

sc3_error_t        *
sc3_MPI_Comm_free (sc3_MPI_Comm_t * comm)
{
  SC3A_CHECK (comm != NULL);
#ifndef SC_ENABLE_MPI
  *comm = sc3_MPI_COMM_NULL;
#else
  SC3E_MPI (MPI_Comm_free (comm));
#endif
  return NULL;
}

sc3_error_t        *
sc3_MPI_Allgather (void *sendbuf, int sendcount, sc3_MPI_Datatype_t sendtype,
                   void *recvbuf, int recvcount, sc3_MPI_Datatype_t recvtype,
                   sc3_MPI_Comm_t comm)
{
#ifndef SC_ENABLE_MPI
  size_t              sendsize, recvsize;

  SC3A_CHECK (sendcount >= 0);
  SC3A_CHECK (recvcount >= 0);
  SC3E (sc3_MPI_Datatype_size (sendtype, &sendsize));
  SC3E (sc3_MPI_Datatype_size (recvtype, &recvsize));
  sendsize *= sendcount;
  recvsize *= recvcount;
  SC3A_CHECK (sendsize == recvsize);
  if (sendsize > 0) {
    SC3A_CHECK (sendbuf != NULL);
    SC3A_CHECK (recvbuf != NULL);
    (void) memmove (recvbuf, sendbuf, sendsize);
  }
#else
  SC3E_MPI (MPI_Allgather (sendbuf, sendcount, sendtype,
                           recvbuf, recvcount, recvtype, comm));
#endif
  return NULL;
}

sc3_error_t        *
sc3_MPI_Allreduce (void *sendbuf, void *recvbuf, int count,
                   sc3_MPI_Datatype_t datatype,
                   sc3_MPI_Op_t op, sc3_MPI_Comm_t comm)
{
#ifndef SC_ENABLE_MPI
  size_t              datasize;

  SC3A_CHECK (count >= 0);
  SC3E (sc3_MPI_Datatype_size (datatype, &datasize));
  datasize *= count;
  if (datasize > 0) {
    SC3A_CHECK (sendbuf != NULL);
    SC3A_CHECK (recvbuf != NULL);
    (void) memmove (recvbuf, sendbuf, datasize);
  }
#else
  SC3E_MPI (MPI_Allreduce (sendbuf, recvbuf, count, datatype, op, comm));
#endif
  return NULL;
}
