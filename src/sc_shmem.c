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

#include <sc_shmem.h>

#if defined(__bgq__)
/** for sc_allgather_final_*_bgq routines to work on BG/Q, you must
 * run with --env BG_MAPCOMMONHEAP=1 */
#include <hwi/include/bqc/A2_inlines.h>
#endif

#if defined(SC_ENABLE_MPI)
static int          sc_shmem_keyval = MPI_KEYVAL_INVALID;
#endif

const char         *sc_shmem_type_to_string[SC_SHMEM_NUM_TYPES] = {
  "basic", "basic_prescan",
#if defined(SC_ENABLE_MPIWINSHARED)
  "window", "window_prescan",
#endif
#if defined(__bgq__)
  "bgq", "bgq_prescan",
#endif
};

static void
sc_scan_on_array (void *recvchar, int size, int count, int typesize,
                  sc_MPI_Datatype type, sc_MPI_Op op)
{
  int                 p, c;

  if (op == sc_MPI_SUM) {
    if (type == sc_MPI_CHAR) {
      char               *array = (char *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_SHORT) {
      short              *array = (short *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_UNSIGNED_SHORT) {
      unsigned short     *array = (unsigned short *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_INT) {
      int                *array = (int *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_UNSIGNED) {
      unsigned           *array = (unsigned *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_LONG) {
      long               *array = (long *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_UNSIGNED_LONG) {
      unsigned long      *array = (unsigned long *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_LONG_LONG_INT) {
      long long          *array = (long long *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_FLOAT) {
      float              *array = (float *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_DOUBLE) {
      double             *array = (double *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_LONG_DOUBLE) {
      long double        *array = (long double *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else {
      SC_ABORT ("MPI_Datatype not supported\n");
    }
  }
  else {
    SC_ABORT ("MPI_Op not supported\n");
  }
}

#if !defined(SC_SHMEM_DEFAULT)
#define SC_SHMEM_DEFAULT SC_SHMEM_BASIC
#endif
sc_shmem_type_t     sc_shmem_default_type = SC_SHMEM_DEFAULT;

#ifdef SC_ENABLE_MPI

static sc_shmem_type_t sc_shmem_types[SC_SHMEM_NUM_TYPES] = {
  SC_SHMEM_BASIC,
  SC_SHMEM_PRESCAN,
#if defined(SC_ENABLE_MPIWINSHARED)
  SC_SHMEM_WINDOW,
  SC_SHMEM_WINDOW_PRESCAN
#endif
#if defined(__bgq__)
    SC_SHMEM_BGQ,
  SC_SHMEM_BGQ_PRESCAN,
#endif
};

#endif /* SC_ENABLE_MPI */

sc_shmem_type_t
sc_shmem_get_type (sc_MPI_Comm comm)
{
#if defined(SC_ENABLE_MPI)
  int                 mpiret, flg;
  sc_shmem_type_t    *type;

  if (sc_shmem_keyval == MPI_KEYVAL_INVALID) {
    mpiret =
      MPI_Comm_create_keyval (MPI_COMM_DUP_FN, MPI_COMM_NULL_DELETE_FN,
                              &sc_shmem_keyval, NULL);
    SC_CHECK_MPI (mpiret);
  }
  SC_ASSERT (sc_shmem_keyval != MPI_KEYVAL_INVALID);

  mpiret = MPI_Comm_get_attr (comm, sc_shmem_keyval, &type, &flg);
  SC_CHECK_MPI (mpiret);

  if (flg) {
    return *type;
  }
  else {
    return SC_SHMEM_NOT_SET;
  }
#else
  return SC_SHMEM_BASIC;
#endif
}

void
sc_shmem_set_type (sc_MPI_Comm comm, sc_shmem_type_t type)
{
#if defined(SC_ENABLE_MPI)
  int                 mpiret;

  if (sc_shmem_keyval == MPI_KEYVAL_INVALID) {
    mpiret =
      MPI_Comm_create_keyval (MPI_COMM_DUP_FN, MPI_COMM_NULL_DELETE_FN,
                              &sc_shmem_keyval, NULL);
    SC_CHECK_MPI (mpiret);
  }
  SC_ASSERT (sc_shmem_keyval != MPI_KEYVAL_INVALID);

  mpiret = MPI_Comm_set_attr (comm, sc_shmem_keyval, &sc_shmem_types[type]);
  SC_CHECK_MPI (mpiret);
#endif
}

static              sc_shmem_type_t
sc_shmem_get_type_default (sc_MPI_Comm comm)
{
  sc_shmem_type_t     type = sc_shmem_get_type (comm);
  if (type == SC_SHMEM_NOT_SET) {
    type = sc_shmem_default_type;
    sc_shmem_set_type (comm, type);
  }
  return type;
}

/* BASIC implementation */
static void        *
sc_shmem_malloc_basic (int package, size_t elem_count, size_t elem_size,
                       sc_MPI_Comm comm, sc_MPI_Comm intranode,
                       sc_MPI_Comm internode)
{
  return sc_malloc (package, elem_count * elem_size);
}

static void
sc_shmem_free_basic (int package, void *array, sc_MPI_Comm comm,
                     sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  sc_free (package, array);
}

static int
sc_shmem_write_start_basic (void *array, sc_MPI_Comm comm,
                            sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  return 1;
}

static void
sc_shmem_write_end_basic (void *array, sc_MPI_Comm comm,
                          sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
}

static void
sc_shmem_memcpy_basic (void *destarray, void *srcarray, size_t bytes,
                       sc_MPI_Comm comm, sc_MPI_Comm intranode,
                       sc_MPI_Comm internode)
{
  memcpy (destarray, srcarray, bytes);
}

static void
sc_shmem_allgather_basic (void *sendbuf, int sendcount,
                          sc_MPI_Datatype sendtype, void *recvbuf,
                          int recvcount, sc_MPI_Datatype recvtype,
                          sc_MPI_Comm comm, sc_MPI_Comm intranode,
                          sc_MPI_Comm internode)
{
  int                 mpiret = sc_MPI_Allgather (sendbuf, sendcount, sendtype,
                                                 recvbuf, recvcount, recvtype,
                                                 comm);
  SC_CHECK_MPI (mpiret);
}

static void
sc_shmem_prefix_basic (void *sendbuf, void *recvbuf, int count,
                       sc_MPI_Datatype type, sc_MPI_Op op,
                       sc_MPI_Comm comm,
                       sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  int                 mpiret, size;
  size_t              typesize = sc_mpi_sizeof (type);

  memset (recvbuf, 0, typesize * count);
  mpiret = sc_MPI_Allgather (sendbuf, count, type, (char *) recvbuf +
                             typesize * count, count, type, comm);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_size (comm, &size);
  SC_CHECK_MPI (mpiret);
  sc_scan_on_array (recvbuf, size, count, typesize, type, op);
}

/* PRESCAN implementation */

static void
sc_shmem_prefix_prescan (void *sendbuf, void *recvbuf, int count,
                         sc_MPI_Datatype type, sc_MPI_Op op,
                         sc_MPI_Comm comm, sc_MPI_Comm intranode,
                         sc_MPI_Comm internode)
{
  int                 mpiret;
  size_t              typesize = sc_mpi_sizeof (type);
  char               *sendscan;

  sendscan = SC_ALLOC (char, typesize * count);
  mpiret = sc_MPI_Scan (sendbuf, sendscan, count, type, op, comm);
  SC_CHECK_MPI (mpiret);

  memset (recvbuf, 0, typesize * count);
  mpiret = sc_MPI_Allgather (sendscan, count, type,
                             (char *) recvbuf + typesize * count,
                             count, type, comm);
  SC_CHECK_MPI (mpiret);
  SC_FREE (sendscan);
}

/* common to SHARED and WINDOW */

#if defined(__bgq__) || defined(SC_ENABLE_MPIWINSHARED)

static void
sc_shmem_memcpy_common (void *destarray, void *srcarray, size_t bytes,
                        sc_MPI_Comm comm, sc_MPI_Comm intranode,
                        sc_MPI_Comm internode)
{
  if (sc_shmem_write_start (destarray, comm)) {
    memcpy (destarray, srcarray, bytes);
  }
  sc_shmem_write_end (destarray, comm);
}

static void
sc_shmem_allgather_common (void *sendbuf, int sendcount,
                           sc_MPI_Datatype sendtype, void *recvbuf,
                           int recvcount, sc_MPI_Datatype recvtype,
                           sc_MPI_Comm comm, sc_MPI_Comm intranode,
                           sc_MPI_Comm internode)
{
  size_t              typesize;
  int                 mpiret, intrarank, intrasize;
  char               *noderecvchar = NULL;

  typesize = sc_mpi_sizeof (recvtype);

  mpiret = sc_MPI_Comm_rank (intranode, &intrarank);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_size (intranode, &intrasize);
  SC_CHECK_MPI (mpiret);

  /* node root gathers from node */
  if (!intrarank) {
    noderecvchar = SC_ALLOC (char, intrasize * recvcount * typesize);
  }
  mpiret =
    sc_MPI_Gather (sendbuf, sendcount, sendtype, noderecvchar, recvcount,
                   recvtype, 0, intranode);
  SC_CHECK_MPI (mpiret);

  /* node root allgathers between nodes */
  if (sc_shmem_write_start (recvbuf, comm)) {
    mpiret =
      sc_MPI_Allgather (noderecvchar, sendcount * intrasize, sendtype,
                        recvbuf, recvcount * intrasize, recvtype, internode);
    SC_CHECK_MPI (mpiret);
    SC_FREE (noderecvchar);
  }
  sc_shmem_write_end (recvbuf, comm);
}

static void
sc_shmem_prefix_common (void *sendbuf, void *recvbuf, int count,
                        sc_MPI_Datatype type, sc_MPI_Op op,
                        sc_MPI_Comm comm, sc_MPI_Comm intranode,
                        sc_MPI_Comm internode)
{
  size_t              typesize;
  int                 mpiret, intrarank, intrasize, size;
  char               *noderecvchar = NULL;

  typesize = sc_mpi_sizeof (type);

  mpiret = sc_MPI_Comm_size (comm, &size);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (intranode, &intrarank);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_size (intranode, &intrasize);
  SC_CHECK_MPI (mpiret);

  /* node root gathers from node */
  if (!intrarank) {
    noderecvchar = SC_ALLOC (char, intrasize * count * typesize);
  }
  mpiret =
    sc_MPI_Gather (sendbuf, count, type, noderecvchar, count, type, 0,
                   intranode);
  SC_CHECK_MPI (mpiret);

  /* node root allgathers between nodes */
  if (sc_shmem_write_start (recvbuf, comm)) {
    memset (recvbuf, 0, count * typesize);
    mpiret =
      sc_MPI_Allgather (noderecvchar, count * intrasize, type,
                        (char *) recvbuf + count * typesize,
                        count * intrasize, type, internode);
    SC_CHECK_MPI (mpiret);
    SC_FREE (noderecvchar);
    sc_scan_on_array (recvbuf, size, count, typesize, type, op);
  }
  sc_shmem_write_end (recvbuf, comm);
}

static void
sc_shmem_prefix_common_prescan (void *sendbuf, void *recvbuf, int count,
                                sc_MPI_Datatype type, sc_MPI_Op op,
                                sc_MPI_Comm comm, sc_MPI_Comm intranode,
                                sc_MPI_Comm internode)
{
  size_t              typesize;
  int                 mpiret, intrarank, intrasize, size;
  char               *sendscan = NULL;
  char               *noderecvchar = NULL;

  typesize = sc_mpi_sizeof (type);

  sendscan = SC_ALLOC (char, typesize * count);
  mpiret = sc_MPI_Scan (sendbuf, sendscan, count, type, op, comm);

  mpiret = sc_MPI_Comm_size (comm, &size);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (intranode, &intrarank);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_size (intranode, &intrasize);
  SC_CHECK_MPI (mpiret);

  /* node root gathers from node */
  if (!intrarank) {
    noderecvchar = SC_ALLOC (char, intrasize * count * typesize);
  }
  mpiret =
    sc_MPI_Gather (sendscan, count, type, noderecvchar, count, type, 0,
                   intranode);
  SC_CHECK_MPI (mpiret);
  SC_FREE (sendscan);

  /* node root allgathers between nodes */
  if (sc_shmem_write_start (recvbuf, comm)) {
    memset (recvbuf, 0, count * typesize);
    mpiret =
      sc_MPI_Allgather (noderecvchar, count * intrasize, type,
                        (char *) recvbuf + count * typesize,
                        count * intrasize, type, internode);
    SC_CHECK_MPI (mpiret);
    SC_FREE (noderecvchar);
  }
  sc_shmem_write_end (recvbuf, comm);
}

#endif /* defined(__bgq__) || defined(SC_ENABLE_MPIWINSHARED) */

#if defined(__bgq__)
/* SHARED implementation */

static int
sc_shmem_write_start_bgq (void *array, sc_MPI_Comm comm,
                          sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  int                 intrarank, mpiret;

  mpiret = sc_MPI_Comm_rank (intranode, &intrarank);
  SC_CHECK_MPI (mpiret);

  return !intrarank;
}

static void
sc_shmem_write_end_bgq (void *array, sc_MPI_Comm comm,
                        sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  int                 mpiret;

  /* these memory sync's are included in Jeff's example */
  /* https://wiki.alcf.anl.gov/parts/index.php/Blue_Gene/Q#Abusing_the_common_heap */
  ppc_msync ();
  mpiret = sc_MPI_Barrier (intranode);
  SC_CHECK_MPI (mpiret);
}

static void        *
sc_shmem_malloc_bgq (int package, size_t elem_size, size_t elem_count,
                     sc_MPI_Comm comm, sc_MPI_Comm intranode,
                     sc_MPI_Comm internode)
{
  char               *array = NULL;
  int                 mpiret;

  if (sc_shmem_write_start_bgq (NULL, comm, intranode, internode)) {
    array = sc_malloc (package, elem_size * elem_count);

  }
  sc_shmem_write_end_bgq (NULL, comm, intranode, internode);

  /* node root broadcast array start in node */
  mpiret = sc_MPI_Bcast (&array, sizeof (char *), sc_MPI_BYTE, 0, intranode);
  SC_CHECK_MPI (mpiret);

  /* these memory sync's are included in Jeff's example */
  /* https://wiki.alcf.anl.gov/parts/index.php/Blue_Gene/Q#Abusing_the_common_heap */
  ppc_msync ();

  return (void *) array;
}

static void
sc_shmem_free_bgq (int package, void *array, sc_MPI_Comm comm,
                   sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  if (sc_shmem_write_start_bgq (NULL, comm, intranode, internode)) {
    sc_free (package, array);
  }
  sc_shmem_write_end_bgq (NULL, comm, intranode, internode);
}
#endif /* __bgq__ */

#if defined(SC_ENABLE_MPIWINSHARED)
/* MPI_Win implementation */

static              MPI_Win
sc_shmem_get_win (void *array, sc_MPI_Comm comm, sc_MPI_Comm intranode,
                  sc_MPI_Comm internode)
{
  int                 mpiret, intrarank, intrasize;

  mpiret = sc_MPI_Comm_rank (intranode, &intrarank);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_size (intranode, &intrasize);
  SC_CHECK_MPI (mpiret);
  return ((MPI_Win *) array)[-intrasize + intrarank];
}

static void        *
sc_shmem_malloc_window (int package, size_t elem_size, size_t elem_count,
                        sc_MPI_Comm comm, sc_MPI_Comm intranode,
                        sc_MPI_Comm internode)
{
  char               *array = NULL;
  int                 mpiret, disp_unit, intrarank, intrasize;
  MPI_Win             win;
  MPI_Aint            winsize = 0;

  disp_unit = SC_MAX (elem_size, sizeof (MPI_Win));
  mpiret = sc_MPI_Comm_rank (intranode, &intrarank);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_size (intranode, &intrasize);
  SC_CHECK_MPI (mpiret);
  if (!intrarank) {
    winsize = elem_size * elem_count + intrasize * sizeof (MPI_Win);
    if (winsize % disp_unit) {
      winsize = ((winsize / disp_unit) + 1) * disp_unit;
    }
  }
  mpiret =
    MPI_Win_allocate_bgq (winsize, disp_unit, MPI_INFO_NULL, intranode,
                          &array, &win);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Win_bgq_query (win, 0, &winsize, &disp_unit, &array);
  SC_CHECK_MPI (mpiret);
  /* store the windows at the front of the array */
  mpiret = sc_MPI_Gather (&win, sizeof (MPI_Win), sc_MPI_BYTE,
                          array, sizeof (MPI_Win), sc_MPI_BYTE, 0, intranode);
  SC_CHECK_MPI (mpiret);

  mpiret = MPI_Win_lock (MPI_LOCK_SHARED, 0, MPI_MODE_NOCHECK, win);
  SC_CHECK_MPI (mpiret);

  return ((MPI_Win *) array) + intrasize;
}

static void
sc_shmem_free_window (int package, void *array, sc_MPI_Comm comm,
                      sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  int                 mpiret;
  MPI_Win             win;

  win = sc_shmem_get_win (array, comm, intranode, internode);

  mpiret = MPI_Win_unlock (0, win);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Win_free (&win);
  SC_CHECK_MPI (mpiret);
}

static int
sc_shmem_write_start_window (void *array, sc_MPI_Comm comm,
                             sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  int                 mpiret, intrarank;
  MPI_Win             win;

  win = sc_shmem_get_win (array, comm, intranode, internode);

  mpiret = MPI_Win_unlock (0, win);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (intranode, &intrarank);
  SC_CHECK_MPI (mpiret);
  if (!intrarank) {
    mpiret = MPI_Win_lock (MPI_LOCK_EXCLUSIVE, 0, MPI_MODE_NOCHECK, win);
    SC_CHECK_MPI (mpiret);

    return 1;
  }
  return 0;
}

static void
sc_shmem_write_end_window (void *array, sc_MPI_Comm comm,
                           sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  int                 mpiret, intrarank;
  MPI_Win             win;

  win = sc_shmem_get_win (array, comm, intranode, internode);

  mpiret = sc_MPI_Comm_rank (intranode, &intrarank);
  SC_CHECK_MPI (mpiret);
  if (!intrarank) {
    mpiret = MPI_Win_unlock (0, win);
    SC_CHECK_MPI (mpiret);
  }
  mpiret = sc_MPI_Barrier (intranode);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Win_lock (MPI_LOCK_SHARED, 0, MPI_MODE_NOCHECK, win);
  SC_CHECK_MPI (mpiret);
}
#endif /* SC_ENABLE_MPIWINSHARED */

void               *
sc_shmem_malloc (int package, size_t elem_size, size_t elem_count,
                 sc_MPI_Comm comm)
{
  sc_shmem_type_t     type;
  sc_MPI_Comm         intranode = sc_MPI_COMM_NULL, internode =
    sc_MPI_COMM_NULL;

  type = sc_shmem_get_type_default (comm);
  sc_mpi_comm_get_node_comms (comm, &intranode, &internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    type = SC_SHMEM_BASIC;
  }
  switch (type) {
  case SC_SHMEM_BASIC:
  case SC_SHMEM_PRESCAN:
    return sc_shmem_malloc_basic (package, elem_size, elem_count, comm,
                                  intranode, internode);
#if defined(__bgq__)
  case SC_SHMEM_BGQ:
  case SC_SHMEM_BGQ_PRESCAN:
    return sc_shmem_malloc_bgq (package, elem_size, elem_count, comm,
                                intranode, internode);
#endif
#if defined(SC_ENABLE_MPIWINSHARED)
  case SC_SHMEM_WINDOW:
  case SC_SHMEM_WINDOW_PRESCAN:
    return sc_shmem_malloc_window (package, elem_size, elem_count, comm,
                                   intranode, internode);
#endif
  default:
    SC_ABORT_NOT_REACHED ();
  }
  return NULL;
}

void
sc_shmem_free (int package, void *array, sc_MPI_Comm comm)
{
  sc_shmem_type_t     type;
  sc_MPI_Comm         intranode = sc_MPI_COMM_NULL, internode =
    sc_MPI_COMM_NULL;

  type = sc_shmem_get_type_default (comm);
  sc_mpi_comm_get_node_comms (comm, &intranode, &internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    type = SC_SHMEM_BASIC;
  }
  switch (type) {
  case SC_SHMEM_BASIC:
  case SC_SHMEM_PRESCAN:
    sc_shmem_free_basic (package, array, comm, intranode, internode);
    break;
#if defined(__bgq__)
  case SC_SHMEM_BGQ:
  case SC_SHMEM_BGQ_PRESCAN:
    sc_shmem_free_bgq (package, array, comm, intranode, internode);
    break;
#endif
#if defined(SC_ENABLE_MPIWINSHARED)
  case SC_SHMEM_WINDOW:
  case SC_SHMEM_WINDOW_PRESCAN:
    sc_shmem_free_window (package, array, comm, intranode, internode);
    break;
#endif
  default:
    SC_ABORT_NOT_REACHED ();
  }
}

int
sc_shmem_write_start (void *array, sc_MPI_Comm comm)
{
  sc_shmem_type_t     type;
  sc_MPI_Comm         intranode = sc_MPI_COMM_NULL, internode =
    sc_MPI_COMM_NULL;

  type = sc_shmem_get_type_default (comm);
  sc_mpi_comm_get_node_comms (comm, &intranode, &internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    type = SC_SHMEM_BASIC;
  }
  switch (type) {
  case SC_SHMEM_BASIC:
  case SC_SHMEM_PRESCAN:
    return sc_shmem_write_start_basic (array, comm, intranode, internode);
#if defined(__bgq__)
  case SC_SHMEM_BGQ:
  case SC_SHMEM_BGQ_PRESCAN:
    return sc_shmem_write_start_bgq (array, comm, intranode, internode);
#endif
#if defined(SC_ENABLE_MPIWINSHARED)
  case SC_SHMEM_WINDOW:
  case SC_SHMEM_WINDOW_PRESCAN:
    return sc_shmem_write_start_window (array, comm, intranode, internode);
#endif
  default:
    SC_ABORT_NOT_REACHED ();
  }
  return 0;
}

void
sc_shmem_write_end (void *array, sc_MPI_Comm comm)
{
  sc_shmem_type_t     type;
  sc_MPI_Comm         intranode = sc_MPI_COMM_NULL, internode =
    sc_MPI_COMM_NULL;

  type = sc_shmem_get_type_default (comm);
  sc_mpi_comm_get_node_comms (comm, &intranode, &internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    type = SC_SHMEM_BASIC;
  }
  switch (type) {
  case SC_SHMEM_BASIC:
  case SC_SHMEM_PRESCAN:
    sc_shmem_write_end_basic (array, comm, intranode, internode);
    break;
#if defined(__bgq__)
  case SC_SHMEM_BGQ:
  case SC_SHMEM_BGQ_PRESCAN:
    sc_shmem_write_end_bgq (array, comm, intranode, internode);
    break;
#endif
#if defined(SC_ENABLE_MPIWINSHARED)
  case SC_SHMEM_WINDOW:
  case SC_SHMEM_WINDOW_PRESCAN:
    sc_shmem_write_end_window (array, comm, intranode, internode);
    break;
#endif
  default:
    SC_ABORT_NOT_REACHED ();
  }
}

void
sc_shmem_memcpy (void *destarray, void *srcarray, size_t bytes,
                 sc_MPI_Comm comm)
{
  sc_shmem_type_t     type;
  sc_MPI_Comm         intranode = sc_MPI_COMM_NULL, internode =
    sc_MPI_COMM_NULL;

  type = sc_shmem_get_type_default (comm);
  sc_mpi_comm_get_node_comms (comm, &intranode, &internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    type = SC_SHMEM_BASIC;
  }
  switch (type) {
  case SC_SHMEM_BASIC:
  case SC_SHMEM_PRESCAN:
    sc_shmem_memcpy_basic (destarray, srcarray, bytes, comm, intranode,
                           internode);
    break;
#if defined(__bgq__) || defined(SC_ENABLE_MPIWINSHARED)
#if defined(__bgq__)
  case SC_SHMEM_BGQ:
  case SC_SHMEM_BGQ_PRESCAN:
#endif
#if defined(SC_ENABLE_MPIWINSHARED)
  case SC_SHMEM_WINDOW:
  case SC_SHMEM_WINDOW_PRESCAN:
#endif
    sc_shmem_memcpy_common (destarray, srcarray, bytes, comm, intranode,
                            internode);
    break;
#endif
  default:
    SC_ABORT_NOT_REACHED ();
  }
}

void
sc_shmem_allgather (void *sendbuf, int sendcount,
                    sc_MPI_Datatype sendtype, void *recvbuf,
                    int recvcount, sc_MPI_Datatype recvtype, sc_MPI_Comm comm)
{
  sc_shmem_type_t     type;
  sc_MPI_Comm         intranode = sc_MPI_COMM_NULL, internode =
    sc_MPI_COMM_NULL;

  type = sc_shmem_get_type_default (comm);
  sc_mpi_comm_get_node_comms (comm, &intranode, &internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    type = SC_SHMEM_BASIC;
  }
  switch (type) {
  case SC_SHMEM_BASIC:
  case SC_SHMEM_PRESCAN:
    sc_shmem_allgather_basic (sendbuf, sendcount, sendtype, recvbuf,
                              recvcount, recvtype, comm, intranode,
                              internode);
    break;
#if defined(__bgq__) || defined(SC_ENABLE_MPIWINSHARED)
#if defined(__bgq__)
  case SC_SHMEM_BGQ:
  case SC_SHMEM_BGQ_PRESCAN:
#endif
#if defined(SC_ENABLE_MPIWINSHARED)
  case SC_SHMEM_WINDOW:
  case SC_SHMEM_WINDOW_PRESCAN:
#endif
    sc_shmem_allgather_common (sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, comm, intranode,
                               internode);
    break;
#endif
  default:
    SC_ABORT_NOT_REACHED ();
  }
}

void
sc_shmem_prefix (void *sendbuf, void *recvbuf, int count,
                 sc_MPI_Datatype dtype, sc_MPI_Op op, sc_MPI_Comm comm)
{
  sc_shmem_type_t     type;
  sc_MPI_Comm         intranode = sc_MPI_COMM_NULL, internode =
    sc_MPI_COMM_NULL;

  type = sc_shmem_get_type_default (comm);
  sc_mpi_comm_get_node_comms (comm, &intranode, &internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    type = SC_SHMEM_BASIC;
  }
  switch (type) {
  case SC_SHMEM_BASIC:
    sc_shmem_prefix_basic (sendbuf, recvbuf, count, dtype, op, comm,
                           intranode, internode);
    break;
  case SC_SHMEM_PRESCAN:
    sc_shmem_prefix_prescan (sendbuf, recvbuf, count, dtype, op, comm,
                             intranode, internode);
    break;
#if defined(__bgq__) || defined(SC_ENABLE_MPIWINSHARED)
#if defined(__bgq__)
  case SC_SHMEM_BGQ:
#endif
#if defined(SC_ENABLE_MPIWINSHARED)
  case SC_SHMEM_WINDOW:
#endif
    sc_shmem_prefix_common (sendbuf, recvbuf, count, dtype, op, comm,
                            intranode, internode);
    break;
#endif
#if defined(__bgq__) || defined(SC_ENABLE_MPIWINSHARED)
#if defined(__bgq__)
  case SC_SHMEM_BGQ_PRESCAN:
#endif
#if defined(SC_ENABLE_MPIWINSHARED)
  case SC_SHMEM_WINDOW_PRESCAN:
#endif
    sc_shmem_prefix_common_prescan (sendbuf, recvbuf, count, dtype, op,
                                    comm, intranode, internode);
    break;
#endif
  default:
    SC_ABORT_NOT_REACHED ();
  }
}
