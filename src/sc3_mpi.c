/*
  This file is part of the SC Library, version 3.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include <sc3_mpi.h>

#ifndef SC_ENABLE_MPI

static sc3_error_t *
sc3_MPI_Datatype_size (sc3_MPI_Datatype_t datatype, size_t * size)
{
  SC3A_CHECK (size != NULL);
  switch (datatype) {
  case SC3_MPI_BYTE:
    *size = 1;
    return NULL;
  case SC3_MPI_INT:
    *size = sizeof (int);
    return NULL;
  case SC3_MPI_LONG:
    *size = sizeof (long);
    return NULL;
  case SC3_MPI_FLOAT:
    *size = sizeof (float);
    return NULL;
  case SC3_MPI_DOUBLE:
    *size = sizeof (double);
    return NULL;
  default:
    SC3E_UNREACH ("Invalid MPI type");
  }
}

static struct sc3_MPI_Comm
{
  int                 comm;
} comm_world       , comm_null;
sc3_MPI_Comm_t      SC3_MPI_COMM_WORLD = &comm_world;
sc3_MPI_Comm_t      SC3_MPI_COMM_SELF = &comm_world;
sc3_MPI_Comm_t      SC3_MPI_COMM_NULL = &comm_null;

static struct sc3_MPI_Info
{
  int                 info;
} info_null;
sc3_MPI_Info_t      SC3_MPI_INFO_NULL = &info_null;

#endif /* !SC_ENABLE_MPI */

#ifndef SC_ENABLE_MPIWINSHARED

static struct sc3_MPI_Win
{
  int                 win;
  int                 size;
  int                 rank;
  int                 disp_unit;
  sc3_MPI_Aint_t      memsize;
  char               *baseptr;
} win_null;
sc3_MPI_Win_t       SC3_MPI_WIN_NULL = &win_null;

#endif /* !SC_ENABLE_MPIWINSHARED */

void
sc3_MPI_Error_class (int errorcode, int *errorclass)
{
  if (errorclass != NULL) {
#ifndef SC_ENABLE_MPI
    *errorclass = errorcode;
#else
    int                 mpiret = MPI_Error_class (errorcode, errorclass);
    if (mpiret != SC3_MPI_SUCCESS) {
      *errorclass = SC3_MPI_ERR_OTHER;
    }
#endif
  }
}

void
sc3_MPI_Error_string (int errorcode, char *errstr, int *errlen)
{
  int                 res;

  if (errstr == NULL) {
    return;
  }
  if (errlen == NULL) {
    *errstr = '\0';
    return;
  }
#ifdef SC_ENABLE_MPI
  errorcode = MPI_Error_string (errorcode, errstr, errlen);
  if (errorcode == SC3_MPI_SUCCESS) {
    int                 i;
    for (i = 0; i < *errlen; ++i) {
      if (errstr[i] == '\n') {
        errstr[i] = ' ';
      }
    }
  }
  else {
#endif
    res = snprintf (errstr, SC3_MPI_MAX_ERROR_STRING, "MPI %s",
                    errorcode == SC3_MPI_SUCCESS ? "Success" : "Error");
    if (res >= SC3_MPI_MAX_ERROR_STRING) {
      res = SC3_MPI_MAX_ERROR_STRING - 1;
    }
    if (res <= 0) {
      *errstr = '\0';
      *errlen = 0;
      return;
    }
    *errlen = res;
#ifdef SC_ENABLE_MPI
  }
#endif
}

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
#ifndef SC_ENABLE_MPI
  SC3A_CHECK (comm != SC3_MPI_COMM_NULL);
#else
  SC3E_MPI (MPI_Comm_set_errhandler (comm, errh));
#endif
  return NULL;
}

sc3_error_t        *
sc3_MPI_Comm_size (sc3_MPI_Comm_t comm, int *size)
{
  SC3A_CHECK (size != NULL);
#ifndef SC_ENABLE_MPI
  SC3A_CHECK (comm != SC3_MPI_COMM_NULL);
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
  SC3A_CHECK (comm != SC3_MPI_COMM_NULL);
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
  SC3A_CHECK (comm != SC3_MPI_COMM_NULL);
  *newcomm = comm;
#else
  SC3E_MPI (MPI_Comm_dup (comm, newcomm));
#endif
  return NULL;
}

sc3_error_t        *
sc3_MPI_Comm_split (sc3_MPI_Comm_t comm, int color, int key,
                    sc3_MPI_Comm_t * newcomm)
{
  SC3A_CHECK (newcomm != NULL);
#ifndef SC_ENABLE_MPI
  SC3A_CHECK (comm != SC3_MPI_COMM_NULL);
  if (color == SC3_MPI_UNDEFINED) {
    *newcomm = SC3_MPI_COMM_NULL;
  }
  else {
    *newcomm = comm;
  }
#else
  SC3E_MPI (MPI_Comm_split (comm, color, key, newcomm));
#endif
  return NULL;
}

sc3_error_t        *
sc3_MPI_Comm_split_type (sc3_MPI_Comm_t comm, int split_type, int key,
                         sc3_MPI_Info_t info, sc3_MPI_Comm_t * newcomm)
{
#ifndef SC3_ENABLE_MPI3
  int                 rank;
#endif

  SC3A_CHECK (newcomm != NULL);
#ifndef SC3_ENABLE_MPI3
  SC3A_CHECK (comm != SC3_MPI_COMM_NULL);
  SC3E (sc3_MPI_Comm_rank (comm, &rank));
  SC3E (sc3_MPI_Comm_split (comm, rank, key, newcomm));
  if (split_type == SC3_MPI_UNDEFINED) {
    SC3E (sc3_MPI_Comm_free (newcomm));
  }
#else
  SC3E_MPI (MPI_Comm_split_type (comm, split_type, key, info, newcomm));
#endif
  return NULL;
}

sc3_error_t        *
sc3_MPI_Comm_free (sc3_MPI_Comm_t * comm)
{
  SC3A_CHECK (comm != NULL);
#ifndef SC_ENABLE_MPI
  SC3A_CHECK (*comm != SC3_MPI_COMM_NULL);
  *comm = SC3_MPI_COMM_NULL;
#else
  SC3E_MPI (MPI_Comm_free (comm));
#endif
  return NULL;
}

sc3_error_t        *
sc3_MPI_Win_allocate_shared (sc3_MPI_Aint_t size, int disp_unit,
                             sc3_MPI_Info_t info, sc3_MPI_Comm_t comm,
                             void *baseptr, sc3_MPI_Win_t * win)
{
#ifndef SC_ENABLE_MPIWINSHARED
  sc3_MPI_Win_t       newin;
#endif

  SC3A_CHECK (win != NULL);
#ifndef SC_ENABLE_MPIWINSHARED
  SC3A_CHECK (comm != SC3_MPI_COMM_NULL);
  newin = SC3_MALLOC (struct sc3_MPI_Win, 1);
  newin->win = 1;
  SC3E (sc3_MPI_Comm_size (comm, &newin->size));
  SC3E (sc3_MPI_Comm_rank (comm, &newin->rank));
  newin->disp_unit = disp_unit;
  newin->memsize = size;
  newin->baseptr = SC3_MALLOC (char, size);
  *(void **) baseptr = newin->baseptr;
  *win = newin;
#else
  SC3E_MPI (MPI_Win_allocate_shared (size, disp_unit, info, comm,
                                     baseptr, win));
#endif
  return NULL;
}

sc3_error_t        *
sc3_MPI_Win_shared_query (sc3_MPI_Win_t win, int rank, sc3_MPI_Aint_t * size,
                          int *disp_unit, void *baseptr)
{
  SC3A_CHECK (size != NULL);
  SC3A_CHECK (disp_unit != NULL);
  SC3A_CHECK (baseptr != NULL);
#ifndef SC_ENABLE_MPIWINSHARED
  SC3A_CHECK (0 <= win->rank && win->rank < win->size);
  SC3A_CHECK (rank == win->rank);
  *disp_unit = win->disp_unit;
  *size = win->memsize;
  *(void **) baseptr = win->baseptr;
#else
  SC3E_MPI (MPI_Win_shared_query (win, rank, size, disp_unit, baseptr));
#endif
  return NULL;
}

sc3_error_t        *
sc3_MPI_Win_free (sc3_MPI_Win_t * win)
{
  SC3A_CHECK (win != NULL);
#ifndef SC_ENABLE_MPIWINSHARED
  SC3A_CHECK ((*win)->win == 1);
  SC3A_CHECK ((*win)->baseptr != NULL || (*win)->memsize == 0);
  SC3_FREE ((*win)->baseptr);
  SC3_FREE (*win);
  *win = SC3_MPI_WIN_NULL;
#else
  SC3E_MPI (MPI_Win_free (win));
#endif
  return NULL;
}

sc3_error_t        *
sc3_MPI_Barrier (sc3_MPI_Comm_t comm)
{
#ifndef SC_ENABLE_MPI
  SC3A_CHECK (comm != SC3_MPI_COMM_NULL);
#else
  SC3E_MPI (MPI_Barrier (comm));
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

  SC3A_CHECK (comm != SC3_MPI_COMM_NULL);
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
sc3_MPI_Allgatherv (void *sendbuf, int sendcount, sc3_MPI_Datatype_t sendtype,
                    void *recvbuf, int *recvcounts, int *displs,
                    sc3_MPI_Datatype_t recvtype, sc3_MPI_Comm_t comm)
{
  SC3A_CHECK (recvcounts != NULL);
  SC3A_CHECK (displs != NULL);
#ifndef SC_ENABLE_MPI
  SC3A_CHECK (displs[0] == 0);
  SC3E (sc3_MPI_Allgather (sendbuf, sendcount, sendtype,
                           recvbuf, recvcounts[0], recvtype, comm));
#else
  SC3E_MPI (MPI_Allgatherv (sendbuf, sendcount, sendtype,
                            recvbuf, recvcounts, displs, recvtype, comm));
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

  SC3A_CHECK (comm != SC3_MPI_COMM_NULL);
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
