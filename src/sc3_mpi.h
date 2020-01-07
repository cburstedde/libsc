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

/** \file sc3_mpi.h
 */

#ifndef SC3_MPI_H
#define SC3_MPI_H

#include <sc3_error.h>

#ifndef SC_ENABLE_MPI

typedef struct sc3_MPI_Errhandler *sc3_MPI_Errhandler_t;
typedef struct sc3_MPI_Comm *sc3_MPI_Comm_t;
typedef enum sc3_MPI_Datatype
{
  SC3_MPI_BYTE,
  SC3_MPI_INT,
  SC3_MPI_LONG,
  SC3_MPI_FLOAT,
  SC3_MPI_DOUBLE
}
sc3_MPI_Datatype_t;
typedef enum sc3_MPI_Op
{
  SC3_MPI_MIN,
  SC3_MPI_MAX,
  SC3_MPI_SUM
}
sc3_MPI_Op_t;
typedef enum sc3_MPI_Errorcode
{
  SC3_MPI_SUCCESS,
  SC3_MPI_ERR_OTHER
}
sc3_MPI_Errorcode_t;

#define SC3_MPI_ERRORS_RETURN NULL
#define SC3_MPI_COMM_WORLD NULL
#define SC3_MPI_COMM_SELF NULL
#define SC3_MPI_COMM_NULL NULL
#define SC3_MPI_MAX_ERROR_STRING SC3_BUFSIZE

#else
#include <mpi.h>

typedef MPI_Errhandler sc3_MPI_Errhandler_t;
typedef MPI_Comm    sc3_MPI_Comm_t;
typedef MPI_Datatype sc3_MPI_Datatype_t;
typedef MPI_Op      sc3_MPI_Op_t;

#define SC3_MPI_BYTE MPI_BYTE
#define SC3_MPI_INT MPI_INT
#define SC3_MPI_LONG MPI_LONG
#define SC3_MPI_FLOAT MPI_FLOAT
#define SC3_MPI_DOUBLE MPI_DOUBLE

#define SC3_MPI_MIN MPI_MIN
#define SC3_MPI_MAX MPI_MAX
#define SC3_MPI_SUM MPI_SUM

#define SC3_MPI_ERRORS_RETURN MPI_ERRORS_RETURN
#define SC3_MPI_COMM_WORLD MPI_COMM_WORLD
#define SC3_MPI_COMM_SELF MPI_COMM_SELF
#define SC3_MPI_COMM_NULL MPI_COMM_NULL

#define SC3_MPI_MAX_ERROR_STRING MPI_MAX_ERROR_STRING
#define SC3_MPI_SUCCESS MPI_SUCCESS
#define SC3_MPI_ERR_OTHER MPI_ERR_OTHER

#endif /* SC_ENABLE_MPI */

#define SC3E_MPI(f) do {                                                \
  int _mpiret = (f);                                                    \
  if (_mpiret != SC3_MPI_SUCCESS) {                                     \
    int _errlen;                                                        \
    char _errstr[SC3_MPI_MAX_ERROR_STRING];                             \
    char _errmsg[SC3_BUFSIZE];                                          \
    sc3_MPI_Error_string (_mpiret, _errstr, &_errlen);                  \
    (void) snprintf (_errmsg, SC3_BUFSIZE, "%s: %s", #f, _errstr);      \
    return sc3_error_new_fatal (__FILE__, __LINE__, _errmsg);           \
  }} while (0)

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

void                sc3_MPI_Error_class (int errorcode, int *errorclass);
void                sc3_MPI_Error_string (int errorcode,
                                          char *errstr, int *errlen);

sc3_error_t        *sc3_MPI_Init (int *argc, char ***argv);
sc3_error_t        *sc3_MPI_Finalize (void);

sc3_error_t        *sc3_MPI_Comm_set_errhandler (sc3_MPI_Comm_t comm,
                                                 sc3_MPI_Errhandler_t errh);
sc3_error_t        *sc3_MPI_Comm_size (sc3_MPI_Comm_t comm, int *size);
sc3_error_t        *sc3_MPI_Comm_rank (sc3_MPI_Comm_t comm, int *rank);
sc3_error_t        *sc3_MPI_Comm_dup (sc3_MPI_Comm_t comm,
                                      sc3_MPI_Comm_t * newcomm);
sc3_error_t        *sc3_MPI_Comm_free (sc3_MPI_Comm_t * comm);

sc3_error_t        *sc3_MPI_Allgather (void *sendbuf, int sendcount,
                                       sc3_MPI_Datatype_t sendtype,
                                       void *recvbuf, int recvcount,
                                       sc3_MPI_Datatype_t recvtype,
                                       sc3_MPI_Comm_t comm);
sc3_error_t        *sc3_MPI_Allreduce (void *sendbuf, void *recvbuf,
                                       int count, sc3_MPI_Datatype_t datatype,
                                       sc3_MPI_Op_t op, sc3_MPI_Comm_t comm);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_MPI_H */
