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

#ifndef sc_MPI_H
#define sc_MPI_H

#include <sc.h>

SC_EXTERN_C_BEGIN;

typedef enum
{
  SC_TAG_AG_ALLTOALL = 's' + 'c',       /* anything really */
  SC_TAG_AG_RECURSIVE_A,
  SC_TAG_AG_RECURSIVE_B,
  SC_TAG_AG_RECURSIVE_C,
  SC_TAG_NOTIFY_RECURSIVE,
  SC_TAG_REDUCE,
  SC_TAG_PSORT_LO,
  SC_TAG_PSORT_HI
}
sc_tag_t;


/*
 * If sc is compiled without mpi, provide simple wrappers for
 * the mpi types and functions, that work for a single process.
 */
#ifndef SC_MPI


#define sc_MPI_SUCCESS 0
#define sc_MPI_COMM_NULL           ((sc_MPI_Comm) 0x04000000)
#define sc_MPI_COMM_WORLD          ((sc_MPI_Comm) 0x44000000)
#define sc_MPI_COMM_SELF           ((sc_MPI_Comm) 0x44000001)

#define sc_MPI_ANY_SOURCE          (-2)
#define sc_MPI_ANY_TAG             (-1)
#define sc_MPI_STATUS_IGNORE       (sc_MPI_Status *) 1
#define sc_MPI_STATUSES_IGNORE     (sc_MPI_Status *) 1

#define sc_MPI_REQUEST_NULL        ((sc_MPI_Request) 0x2c000000)

#define sc_MPI_CHAR                ((sc_MPI_Datatype) 0x4c000101)
#define sc_MPI_SIGNED_CHAR         ((sc_MPI_Datatype) 0x4c000118)
#define sc_MPI_UNSIGNED_CHAR       ((sc_MPI_Datatype) 0x4c000102)
#define sc_MPI_BYTE                ((sc_MPI_Datatype) 0x4c00010d)
#define sc_MPI_SHORT               ((sc_MPI_Datatype) 0x4c000203)
#define sc_MPI_UNSIGNED_SHORT      ((sc_MPI_Datatype) 0x4c000204)
#define sc_MPI_INT                 ((sc_MPI_Datatype) 0x4c000405)
#define sc_MPI_UNSIGNED            ((sc_MPI_Datatype) 0x4c000406)
#define sc_MPI_LONG                ((sc_MPI_Datatype) 0x4c000407)
#define sc_MPI_UNSIGNED_LONG       ((sc_MPI_Datatype) 0x4c000408)
#define sc_MPI_LONG_LONG_INT       ((sc_MPI_Datatype) 0x4c000809)
#define sc_MPI_FLOAT               ((sc_MPI_Datatype) 0x4c00040a)
#define sc_MPI_DOUBLE              ((sc_MPI_Datatype) 0x4c00080b)
#define sc_MPI_LONG_DOUBLE         ((sc_MPI_Datatype) 0x4c000c0c)

#define sc_MPI_MAX                 ((sc_MPI_Op) 0x58000001)
#define sc_MPI_MIN                 ((sc_MPI_Op) 0x58000002)
#define sc_MPI_SUM                 ((sc_MPI_Op) 0x58000003)
#define sc_MPI_PROD                ((sc_MPI_Op) 0x58000004)
#define sc_MPI_LAND                ((sc_MPI_Op) 0x58000005)
#define sc_MPI_BAND                ((sc_MPI_Op) 0x58000006)
#define sc_MPI_LOR                 ((sc_MPI_Op) 0x58000007)
#define sc_MPI_BOR                 ((sc_MPI_Op) 0x58000008)
#define sc_MPI_LXOR                ((sc_MPI_Op) 0x58000009)
#define sc_MPI_BXOR                ((sc_MPI_Op) 0x5800000a)
#define sc_MPI_MINLOC              ((sc_MPI_Op) 0x5800000b)
#define sc_MPI_MAXLOC              ((sc_MPI_Op) 0x5800000c)
#define sc_MPI_REPLACE             ((sc_MPI_Op) 0x5800000d)

#define sc_MPI_UNDEFINED           (-32766)

typedef int         sc_MPI_Comm;
typedef int         sc_MPI_Datatype;
typedef int         sc_MPI_Op;
typedef int         sc_MPI_Request;
typedef struct sc_MPI_Status
{
  int                 count;
  int                 cancelled;
  int                 sc_MPI_SOURCE;
  int                 sc_MPI_TAG;
  int                 sc_MPI_ERROR;
}
sc_MPI_Status;

/* These functions are valid and functional for a single process. */

int                 sc_MPI_Init (int *, char ***);
int                 sc_MPI_Finalize (void);
int                 sc_MPI_Abort (sc_MPI_Comm, int)
  __attribute__ ((noreturn));

int                 sc_MPI_Comm_dup (sc_MPI_Comm, sc_MPI_Comm *);
int                 sc_MPI_Comm_free (sc_MPI_Comm *);
int                 sc_MPI_Comm_size (sc_MPI_Comm, int *);
int                 sc_MPI_Comm_rank (sc_MPI_Comm, int *);

int                 sc_MPI_Barrier (sc_MPI_Comm);
int                 sc_MPI_Bcast (void *, int, sc_MPI_Datatype, int, sc_MPI_Comm);
int                 sc_MPI_Gather (void *, int, sc_MPI_Datatype,
                                void *, int, sc_MPI_Datatype, int, sc_MPI_Comm);
int                 sc_MPI_Gatherv (void *, int, sc_MPI_Datatype, void *,
                                 int *, int *, sc_MPI_Datatype, int, sc_MPI_Comm);
int                 sc_MPI_Allgather (void *, int, sc_MPI_Datatype,
                                   void *, int, sc_MPI_Datatype, sc_MPI_Comm);
int                 sc_MPI_Allgatherv (void *, int, sc_MPI_Datatype, void *,
                                    int *, int *, sc_MPI_Datatype, sc_MPI_Comm);
int                 sc_MPI_Reduce (void *, void *, int, sc_MPI_Datatype,
                                sc_MPI_Op, int, sc_MPI_Comm);
int                 sc_MPI_Allreduce (void *, void *, int, sc_MPI_Datatype,
                                   sc_MPI_Op, sc_MPI_Comm);

double              sc_MPI_Wtime (void);

/* These functions will abort. */
int                 sc_MPI_Recv (void *, int, sc_MPI_Datatype, int, int, sc_MPI_Comm,
                              sc_MPI_Status *);
int                 sc_MPI_Irecv (void *, int, sc_MPI_Datatype, int, int, sc_MPI_Comm,
                               sc_MPI_Request *);
int                 sc_MPI_Send (void *, int, sc_MPI_Datatype, int, int, sc_MPI_Comm);
int                 sc_MPI_Isend (void *, int, sc_MPI_Datatype, int, int, sc_MPI_Comm,
                               sc_MPI_Request *);
int                 sc_MPI_Probe (int, int, sc_MPI_Comm, sc_MPI_Status *);
int                 sc_MPI_Iprobe (int, int, sc_MPI_Comm, int *, sc_MPI_Status *);
int                 sc_MPI_Get_count (sc_MPI_Status *, sc_MPI_Datatype, int *);

/* These functions are only allowed to be called with zero size arrays. */
int                 sc_MPI_Wait (sc_MPI_Request *, sc_MPI_Status *);
int                 sc_MPI_Waitsome (int, sc_MPI_Request *,
                                  int *, int *, sc_MPI_Status *);
int                 sc_MPI_Waitall (int, sc_MPI_Request *, sc_MPI_Status *);





/* If sc is compiled with mpi, define the sc_MPI_* as the
 * corresponding MPI type
 *
 * {{{
 *     s = "..."
 *     mpis = re.findall(r"sc_(MPI_\w*)", s)
 *     for mpi in sorted(set(mpis))
 *         print("#define sc_{0:30}{0}".format(mpi))
 * }}}
 */
#else /* SC_MPI */


#define sc_MPI_ANY_SOURCE                MPI_ANY_SOURCE
#define sc_MPI_ANY_TAG                   MPI_ANY_TAG
#define sc_MPI_Abort                     MPI_Abort
#define sc_MPI_Allgather                 MPI_Allgather
#define sc_MPI_Allgatherv                MPI_Allgatherv
#define sc_MPI_Allreduce                 MPI_Allreduce
#define sc_MPI_BAND                      MPI_BAND
#define sc_MPI_BOR                       MPI_BOR
#define sc_MPI_BXOR                      MPI_BXOR
#define sc_MPI_BYTE                      MPI_BYTE
#define sc_MPI_Barrier                   MPI_Barrier
#define sc_MPI_Bcast                     MPI_Bcast
#define sc_MPI_CHAR                      MPI_CHAR
#define sc_MPI_COMM_NULL                 MPI_COMM_NULL
#define sc_MPI_COMM_SELF                 MPI_COMM_SELF
#define sc_MPI_COMM_WORLD                MPI_COMM_WORLD
#define sc_MPI_Comm                      MPI_Comm
#define sc_MPI_Comm_dup                  MPI_Comm_dup
#define sc_MPI_Comm_free                 MPI_Comm_free
#define sc_MPI_Comm_rank                 MPI_Comm_rank
#define sc_MPI_Comm_size                 MPI_Comm_size
#define sc_MPI_DOUBLE                    MPI_DOUBLE
#define sc_MPI_Datatype                  MPI_Datatype
#define sc_MPI_ERROR                     MPI_ERROR
#define sc_MPI_FLOAT                     MPI_FLOAT
#define sc_MPI_Finalize                  MPI_Finalize
#define sc_MPI_Gather                    MPI_Gather
#define sc_MPI_Gatherv                   MPI_Gatherv
#define sc_MPI_Get_count                 MPI_Get_count
#define sc_MPI_INT                       MPI_INT
#define sc_MPI_Init                      MPI_Init
#define sc_MPI_Iprobe                    MPI_Iprobe
#define sc_MPI_Irecv                     MPI_Irecv
#define sc_MPI_Isend                     MPI_Isend
#define sc_MPI_LAND                      MPI_LAND
#define sc_MPI_LONG                      MPI_LONG
#define sc_MPI_LONG_DOUBLE               MPI_LONG_DOUBLE
#define sc_MPI_LONG_LONG_INT             MPI_LONG_LONG_INT
#define sc_MPI_LOR                       MPI_LOR
#define sc_MPI_LXOR                      MPI_LXOR
#define sc_MPI_MAX                       MPI_MAX
#define sc_MPI_MAXLOC                    MPI_MAXLOC
#define sc_MPI_MIN                       MPI_MIN
#define sc_MPI_MINLOC                    MPI_MINLOC
#define sc_MPI_Op                        MPI_Op
#define sc_MPI_PROD                      MPI_PROD
#define sc_MPI_Probe                     MPI_Probe
#define sc_MPI_REPLACE                   MPI_REPLACE
#define sc_MPI_REQUEST_NULL              MPI_REQUEST_NULL
#define sc_MPI_Recv                      MPI_Recv
#define sc_MPI_Reduce                    MPI_Reduce
#define sc_MPI_Request                   MPI_Request
#define sc_MPI_SHORT                     MPI_SHORT
#define sc_MPI_SIGNED_CHAR               MPI_SIGNED_CHAR
#define sc_MPI_SOURCE                    MPI_SOURCE
#define sc_MPI_STATUSES_IGNORE           MPI_STATUSES_IGNORE
#define sc_MPI_STATUS_IGNORE             MPI_STATUS_IGNORE
#define sc_MPI_SUCCESS                   MPI_SUCCESS
#define sc_MPI_SUM                       MPI_SUM
#define sc_MPI_Send                      MPI_Send
#define sc_MPI_Status                    MPI_Status
#define sc_MPI_TAG                       MPI_TAG
#define sc_MPI_UNDEFINED                 MPI_UNDEFINED
#define sc_MPI_UNSIGNED                  MPI_UNSIGNED
#define sc_MPI_UNSIGNED_CHAR             MPI_UNSIGNED_CHAR
#define sc_MPI_UNSIGNED_LONG             MPI_UNSIGNED_LONG
#define sc_MPI_UNSIGNED_SHORT            MPI_UNSIGNED_SHORT
#define sc_MPI_Wait                      MPI_Wait
#define sc_MPI_Waitall                   MPI_Waitall
#define sc_MPI_Waitsome                  MPI_Waitsome
#define sc_MPI_Wtime                     MPI_Wtime

#endif /* sc_MPI */



/** Return the size of MPI data types.
 * \param [in] t    MPI data type.
 * \return          Returns the size in bytes.
 */
size_t              sc_mpi_sizeof (sc_MPI_Datatype t);


SC_EXTERN_C_END;

#endif /* sc_MPI_H */
