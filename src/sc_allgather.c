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

#include <sc_allgather.h>
#if defined(__bgq__)
/** for sc_allgather_final_*_shared routines to work on BG/Q, you must
 * run with --env BG_MAPCOMMONHEAP=1 */
#include <hwi/include/bqc/A2_inlines.h>
#endif

void
sc_allgather_alltoall (sc_MPI_Comm mpicomm, char *data, int datasize,
                       int groupsize, int myoffset, int myrank)
{
  int                 j, peer;
  int                 mpiret;
  sc_MPI_Request     *request;

  SC_ASSERT (myoffset >= 0 && myoffset < groupsize);

  request = SC_ALLOC (sc_MPI_Request, 2 * groupsize);

  for (j = 0; j < groupsize; ++j) {
    if (j == myoffset) {
      request[j] = request[groupsize + j] = sc_MPI_REQUEST_NULL;
      continue;
    }
    peer = myrank - (myoffset - j);

    mpiret = sc_MPI_Irecv (data + j * datasize, datasize, sc_MPI_BYTE,
                           peer, SC_TAG_AG_ALLTOALL, mpicomm, request + j);
    SC_CHECK_MPI (mpiret);

    mpiret = sc_MPI_Isend (data + myoffset * datasize, datasize, sc_MPI_BYTE,
                           peer, SC_TAG_AG_ALLTOALL,
                           mpicomm, request + groupsize + j);
    SC_CHECK_MPI (mpiret);
  }

  mpiret = sc_MPI_Waitall (2 * groupsize, request, sc_MPI_STATUSES_IGNORE);
  SC_CHECK_MPI (mpiret);

  SC_FREE (request);
}

void
sc_allgather_recursive (sc_MPI_Comm mpicomm, char *data, int datasize,
                        int groupsize, int myoffset, int myrank)
{
  const int           g2 = groupsize / 2;
  const int           g2B = groupsize - g2;
  int                 mpiret;
  sc_MPI_Request      request[3];

  SC_ASSERT (myoffset >= 0 && myoffset < groupsize);

  if (groupsize > SC_AG_ALLTOALL_MAX) {
    if (myoffset < g2) {
      sc_allgather_recursive (mpicomm, data, datasize, g2, myoffset, myrank);

      mpiret = sc_MPI_Irecv (data + g2 * datasize, g2B * datasize,
                             sc_MPI_BYTE, myrank + g2, SC_TAG_AG_RECURSIVE_B,
                             mpicomm, request + 0);
      SC_CHECK_MPI (mpiret);

      mpiret = sc_MPI_Isend (data, g2 * datasize, sc_MPI_BYTE,
                             myrank + g2, SC_TAG_AG_RECURSIVE_A,
                             mpicomm, request + 1);
      SC_CHECK_MPI (mpiret);

      if (myoffset == g2 - 1 && g2 != g2B) {
        mpiret = sc_MPI_Isend (data, g2 * datasize, sc_MPI_BYTE,
                               myrank + g2B, SC_TAG_AG_RECURSIVE_C,
                               mpicomm, request + 2);
        SC_CHECK_MPI (mpiret);
      }
      else {
        request[2] = sc_MPI_REQUEST_NULL;
      }
    }
    else {
      sc_allgather_recursive (mpicomm, data + g2 * datasize, datasize, g2B,
                              myoffset - g2, myrank);

      if (myoffset == groupsize - 1 && g2 != g2B) {
        request[0] = sc_MPI_REQUEST_NULL;
        request[1] = sc_MPI_REQUEST_NULL;

        mpiret = sc_MPI_Irecv (data, g2 * datasize, sc_MPI_BYTE,
                               myrank - g2B, SC_TAG_AG_RECURSIVE_C,
                               mpicomm, request + 2);
        SC_CHECK_MPI (mpiret);
      }
      else {
        mpiret = sc_MPI_Irecv (data, g2 * datasize, sc_MPI_BYTE,
                               myrank - g2, SC_TAG_AG_RECURSIVE_A,
                               mpicomm, request + 0);
        SC_CHECK_MPI (mpiret);

        mpiret = sc_MPI_Isend (data + g2 * datasize, g2B * datasize,
                               sc_MPI_BYTE, myrank - g2,
                               SC_TAG_AG_RECURSIVE_B, mpicomm, request + 1);
        SC_CHECK_MPI (mpiret);

        request[2] = sc_MPI_REQUEST_NULL;
      }
    }

    mpiret = sc_MPI_Waitall (3, request, sc_MPI_STATUSES_IGNORE);
    SC_CHECK_MPI (mpiret);
  }
  else {
    sc_allgather_alltoall (mpicomm, data, datasize, groupsize, myoffset,
                           myrank);
  }
}

int
sc_allgather (void *sendbuf, int sendcount, sc_MPI_Datatype sendtype,
              void *recvbuf, int recvcount, sc_MPI_Datatype recvtype,
              sc_MPI_Comm mpicomm)
{
  int                 mpiret;
  int                 mpisize;
  int                 mpirank;
  size_t              datasize;
#ifdef SC_DEBUG
  size_t              datasize2;
#endif

  SC_ASSERT (sendcount >= 0 && recvcount >= 0);

  /* *INDENT-OFF* HORRIBLE indent bug */
  datasize = (size_t) sendcount * sc_mpi_sizeof (sendtype);
#ifdef SC_DEBUG
  datasize2 = (size_t) recvcount * sc_mpi_sizeof (recvtype);
#endif
  /* *INDENT-ON* */

  SC_ASSERT (datasize == datasize2);

  mpiret = sc_MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  memcpy (((char *) recvbuf) + mpirank * datasize, sendbuf, datasize);
  sc_allgather_recursive (mpicomm, (char *) recvbuf, (int) datasize,
                          mpisize, mpirank, mpirank);

  return sc_MPI_SUCCESS;
}

static void
sc_scan_on_array(void *recvchar, int size, int count, int typesize, sc_MPI_Datatype type,
                 sc_MPI_Op op)
{
  int p, c;

  if (op == sc_MPI_SUM) {
    if (type == sc_MPI_CHAR) {
      char *array = recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_SHORT) {
      short *array = (short *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_UNSIGNED_SHORT) {
      unsigned short *array = (unsigned short *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_INT) {
      int *array = (int *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_UNSIGNED) {
      unsigned *array = (unsigned *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_LONG) {
      long *array = (long *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_UNSIGNED_LONG) {
      unsigned long *array = (unsigned long *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_LONG_LONG_INT) {
      long long *array = (long long *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_FLOAT) {
      float *array = (float *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_DOUBLE) {
      double *array = (double *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else if (type == sc_MPI_LONG_DOUBLE) {
      long double *array = (long double *) recvchar;

      SC_ASSERT (sizeof (*array) == typesize);
      for (p = 1; p <= size; p++) {
        for (c = 0; c < count; c++) {
          array[count * p + c] += array[count * (p - 1) + c];
        }
      }
    }
    else {
      SC_ABORT("MPI_Datatype not supported\n");
    }
  }
  else {
    SC_ABORT("MPI_Op not supported\n");
  }
}

#if !defined(SC_SHMEM_ARRAY_DEFAULT)
#define SC_SHMEM_ARRAY_DEFAULT SC_SHMEM_ARRAY_BASIC
#endif
sc_shmem_array_type_t sc_shmem_array_default_type = SC_SHMEM_ARRAY_DEFAULT;

#if defined(SC_ENABLE_MPI)
int sc_shmem_array_keyval = MPI_KEYVAL_INVALID;
#endif

static sc_shmem_array_type_t sc_shmem_array_types[SC_SHMEM_ARRAY_NUM_TYPES] =
{
  SC_SHMEM_ARRAY_BASIC,
  SC_SHMEM_ARRAY_PRESCAN,
#if defined(__bgq__)
  SC_SHMEM_ARRAY_SHARED,
  SC_SHMEM_ARRAY_SHARED_PRESCAN,
#endif
#if defined(SC_ENABLE_WINSHARED)
  SC_SHMEM_ARRAY_WINDOW,
  SC_SHMEM_ARRAY_WINDOW_PRESCAN
#endif
};

sc_shmem_array_type_t
sc_shmem_array_get_type(sc_MPI_Comm comm)
{
  int mpiret, flg;
  sc_shmem_array_type_t *type;

  mpiret = MPI_Comm_get_attr(comm,sc_shmem_array_keyval,&type,&flg);
  SC_CHECK_MPI(mpiret);

  if (flg) {
    return *type;
  }
  else {
    return SC_SHMEM_ARRAY_NOT_SET;
  }
}

void
sc_shmem_array_set_type(sc_MPI_Comm comm, sc_shmem_array_type_t type)
{
  int mpiret;

  mpiret = MPI_Comm_set_attr(comm,sc_shmem_array_keyval,&sc_shmem_array_types[type]);
  SC_CHECK_MPI(mpiret);
}

static sc_shmem_array_type_t
sc_shmem_array_get_type_default(sc_MPI_Comm comm)
{
  sc_shmem_array_type_t type = sc_shmem_array_get_type(comm);
  if (type == SC_SHMEM_ARRAY_NOT_SET) {
    type = sc_shmem_array_default_type;
    sc_shmem_array_set_type(comm,type);
  }
  return type;
}

/* BASIC implementation */
static void *
sc_shmem_array_alloc_basic(size_t elem_count, size_t elem_size, sc_MPI_Comm comm,
                           sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  return sc_malloc(sc_package_id,elem_count * elem_size);
}

static void
sc_shmem_array_free_basic(void *array, sc_MPI_Comm comm,
                           sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  sc_free(sc_package_id, array);
}

static int
sc_shmem_array_write_start_basic(void *array, sc_MPI_Comm comm,
                           sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  return 1;
}

static void
sc_shmem_array_write_end_basic(void *array, sc_MPI_Comm comm,
                           sc_MPI_Comm intranode, sc_MPI_Comm internode)
{}

static void
sc_shmem_array_memcpy_basic(void *destarray, void *srcarray, size_t bytes, sc_MPI_Comm comm,
                           sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  memcpy (destarray, srcarray, bytes);
}

static void
sc_shmem_array_allgather_basic(void *sendbuf, int sendcount, sc_MPI_Datatype sendtype,
                               void *recvbuf, int recvcount, sc_MPI_Datatype recvtype,
                               sc_MPI_Comm comm,
                           sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  int mpiret = sc_MPI_Allgather (sendbuf, sendcount, sendtype,
                                 recvbuf, recvcount, recvtype,
                                 comm);
  SC_CHECK_MPI(mpiret);
}

static void
sc_shmem_array_prefix_basic (void *sendbuf, int *recvbuf, int count,
                             sc_MPI_Datatype type, sc_MPI_Op op,
                             sc_MPI_Comm comm,
                           sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  int mpiret, size;
  size_t typesize = sc_mpi_sizeof (type);

  memset(recvbuf,0,typesize * count);
  mpiret = sc_MPI_Allgather (sendbuf, count, type, ((char *) recvbuf) + typesize * count, count, type, comm);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size (comm, &size);
  SC_CHECK_MPI(mpiret);
  sc_scan_on_array(recvbuf, size, count, typesize, type, op);
}

/* PRESCAN implementation */

static void
sc_shmem_array_prefix_prescan (void *sendbuf, int *recvbuf, int count,
                               sc_MPI_Datatype type, sc_MPI_Op op,
                               sc_MPI_Comm comm,
                           sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  int mpiret, size;
  size_t typesize = sc_mpi_sizeof (type);
  char *sendscan;

  sendscan = SC_ALLOC (char, typesize * count);
  mpiret = sc_MPI_Scan (sendbuf, sendscan, count, type, op, comm);

  memset(recvbuf,0,typesize * count);
  mpiret = sc_MPI_Allgather (sendscan, count, type, ((char *) recvbuf) + typesize * count, count, type, comm);
  SC_CHECK_MPI(mpiret);
}

/* common to SHARED and WINDOW */

static void
sc_shmem_array_memcpy_common(void *destarray, void *srcarray, size_t bytes, sc_MPI_Comm comm,
                             sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  if (sc_shmem_array_write_start (destarray, comm)) {
    memcpy (destarray, srcarray, bytes);
  }
  sc_shmem_array_write_end (destarray, comm);
}


static void
sc_shmem_array_allgather_common(void *sendbuf, int sendcount, sc_MPI_Datatype sendtype,
                                void *recvbuf, int recvcount, sc_MPI_Datatype recvtype,
                                sc_MPI_Comm comm, sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  size_t typesize;
  int mpiret, intrarank, intrasize;
  char *noderecvchar = NULL;

  typesize = sc_mpi_sizeof (recvtype);

  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(intranode,&intrasize);
  SC_CHECK_MPI(mpiret);

  /* node root gathers from node */
  if (!intrarank) {
    noderecvchar = SC_ALLOC (char,intrasize * recvcount * typesize);
  }
  mpiret = sc_MPI_Gather (sendbuf,sendcount,sendtype,noderecvchar,recvcount,recvtype,0,intranode);
  SC_CHECK_MPI(mpiret);

  /* node root allgathers between nodes */
  if (sc_shmem_array_write_start(recvbuf,comm)) {
    mpiret = sc_MPI_Allgather (noderecvchar,sendcount*intrasize,sendtype,recvbuf,recvcount*intrasize,
                               recvtype, internode);
    SC_CHECK_MPI(mpiret);
    SC_FREE (noderecvchar);
  }
  sc_shmem_array_write_end(recvbuf,comm);
}

static void
sc_shmem_array_prefix_common(void *sendbuf, void *recvbuf, int count, sc_MPI_Datatype type, sc_MPI_Op op,
                             sc_MPI_Comm comm, sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  size_t typesize;
  int mpiret, intrarank, intrasize, size;
  char *noderecvchar = NULL;

  typesize = sc_mpi_sizeof (type);

  mpiret = sc_MPI_Comm_size(comm,&size);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(intranode,&intrasize);
  SC_CHECK_MPI(mpiret);

  /* node root gathers from node */
  if (!intrarank) {
    noderecvchar = SC_ALLOC (char,intrasize * count * typesize);
  }
  mpiret = sc_MPI_Gather (sendbuf,count,type,noderecvchar,count,type,0,intranode);
  SC_CHECK_MPI(mpiret);

  /* node root allgathers between nodes */
  if (sc_shmem_array_write_start(recvbuf,comm)) {
    memset (recvbuf,0,count*typesize);
    mpiret = sc_MPI_Allgather (noderecvchar,count*intrasize,type,recvbuf+count*typesize,count*intrasize,
                               type, internode);
    SC_CHECK_MPI(mpiret);
    SC_FREE (noderecvchar);
    sc_scan_on_array(recvbuf, size, count, typesize, type, op);
  }
  sc_shmem_array_write_end(recvbuf,comm);
}

static void
sc_shmem_array_prefix_common_prescan(void *sendbuf, void *recvbuf, int count, sc_MPI_Datatype type, sc_MPI_Op op,
                                     sc_MPI_Comm comm, sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  size_t typesize;
  int mpiret, intrarank, intrasize, size;
  char *sendscan = NULL;
  char *noderecvchar = NULL;

  typesize = sc_mpi_sizeof (type);

  sendscan = SC_ALLOC (char, typesize * count);
  mpiret = sc_MPI_Scan (sendbuf, sendscan, count, type, op, comm);

  mpiret = sc_MPI_Comm_size(comm,&size);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(intranode,&intrasize);
  SC_CHECK_MPI(mpiret);

  /* node root gathers from node */
  if (!intrarank) {
    noderecvchar = SC_ALLOC (char,intrasize * count * typesize);
  }
  mpiret = sc_MPI_Gather (sendscan,count,type,noderecvchar,count,type,0,intranode);
  SC_CHECK_MPI(mpiret);
  SC_FREE(sendscan);

  /* node root allgathers between nodes */
  if (sc_shmem_array_write_start(recvbuf,comm)) {
    memset (recvbuf,0,count*typesize);
    mpiret = sc_MPI_Allgather (noderecvchar,count*intrasize,type,recvbuf+count*typesize,count*intrasize,
                               type, internode);
    SC_CHECK_MPI(mpiret);
    SC_FREE (noderecvchar);
  }
  sc_shmem_array_write_end(recvbuf,comm);
}

#if defined(__bgq__)
/* SHARED implementation */

static int
sc_shmem_array_write_start_shared(void *array, sc_MPI_Comm comm,
                           sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  int intrarank, mpiret;

  mpiret = sc_MPI_Comm_rank(intranode, &intrarank);
  SC_CHECK_MPI(mpiret);

  return !intrarank;
}

static void
sc_shmem_array_write_end_shared(void *array, sc_MPI_Comm comm,
                                sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  int mpiret;

  /* these memory sync's are included in Jeff's example */
  /* https://wiki.alcf.anl.gov/parts/index.php/Blue_Gene/Q#Abusing_the_common_heap */
  ppc_msync();
  mpiret = MPI_Barrier(intranode);
  SC_CHECK_MPI(mpiret);
}

static void *
sc_shmem_array_alloc_shared(size_t elem_size, size_t elem_count, sc_MPI_Comm comm,
                           sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  char * array = NULL;
  int mpiret;

  if (sc_shmem_array_write_start_shared (NULL, comm, intranode, internode)) {
    array = sc_malloc (sc_package_id, elem_size * elem_count);

  }
  sc_shmem_array_write_end_shared (NULL, comm, intranode, internode);

  /* node root broadcast array start in node */
  mpiret = sc_MPI_Bcast(&array,sizeof(char *),sc_MPI_BYTE,0,intranode);
  SC_CHECK_MPI(mpiret);

  /* these memory sync's are included in Jeff's example */
  /* https://wiki.alcf.anl.gov/parts/index.php/Blue_Gene/Q#Abusing_the_common_heap */
  ppc_msync();

  return (void *) array;
}

static void
sc_shmem_array_free_shared(void *array, sc_MPI_Comm comm, sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  if (sc_shmem_array_write_start_shared (NULL, comm, intranode, internode)) {
    sc_free (sc_package_id, array);
  }
  sc_shmem_array_write_end_shared (NULL, comm, intranode, internode);
}
#endif /* __bgq__ */

#if defined(SC_ENABLE_MPIWINSHARED)
/* MPI_Win implementation */

static MPI_Win
sc_shmem_array_get_win (void * array, sc_MPI_Comm comm, sc_MPI_Comm intranode,
                        sc_MPI_Comm internode)
{
  int mpiret, intrarank, intrasize;

  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(intranode,&intrasize);
  SC_CHECK_MPI(mpiret);
  return ((MPI_Win *) array)[-intrasize + intrarank];
}

static void *
sc_shmem_array_alloc_window(size_t elem_size, size_t elem_count, sc_MPI_Comm comm,
                            sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  char * array = NULL;
  int mpiret, disp_unit, intrarank, intrasize;
  MPI_Win win;
  MPI_Aint winsize = 0;

  disp_unit = SC_MAX (elem_size, sizeof (MPI_Win));
  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(intranode,&intrasize);
  SC_CHECK_MPI(mpiret);
  if (!intrarank) {
    winsize = elem_size * elem_count + intrasize * sizeof (MPI_Win);
    if (winsize % disp_unit) {
      winsize = ((winsize / disp_unit) + 1) * disp_unit;
    }
  }
  mpiret = MPI_Win_allocate_shared(winsize,disp_unit,MPI_INFO_NULL,intranode,&array,&win);
  SC_CHECK_MPI(mpiret);
  mpiret = MPI_Win_shared_query(win,0,&winsize,&disp_unit,&array);
  SC_CHECK_MPI(mpiret);
  /* store the windows at the front of the array */
  mpiret = sc_MPI_Gather (&win,sizeof(MPI_Win),sc_MPI_BYTE,
                          array,sizeof(MPI_Win),sc_MPI_BYTE,
                          0,intranode);
  SC_CHECK_MPI(mpiret);

  mpiret = MPI_Win_lock(MPI_LOCK_SHARED,0,MPI_MODE_NOCHECK,win);
  SC_CHECK_MPI(mpiret);

  return ((MPI_Win *) array) + intrasize;
}

static void
sc_shmem_array_free_window(void *array, sc_MPI_Comm comm,
                           sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  int mpiret;
  MPI_Win win;

  win = sc_shmem_array_get_win (array, comm, intranode, internode);

  mpiret = MPI_Win_unlock(0,win);
  SC_CHECK_MPI(mpiret);
  mpiret = MPI_Win_free(&win);
  SC_CHECK_MPI(mpiret);
}

static int
sc_shmem_array_write_start_window(void *array, sc_MPI_Comm comm,
                                  sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  int mpiret, intrarank;
  MPI_Win win;

  win = sc_shmem_array_get_win (array, comm, intranode, internode);

  mpiret = MPI_Win_unlock(0,win);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  if (!intrarank) {
    mpiret = MPI_Win_lock(MPI_LOCK_EXCLUSIVE,0,MPI_MODE_NOCHECK,win);
    SC_CHECK_MPI(mpiret);

    return 1;
  }
  return 0;
}

static void
sc_shmem_array_write_end_window(void *array, sc_MPI_Comm comm,
                                sc_MPI_Comm intranode, sc_MPI_Comm internode)
{
  int mpiret, intrarank;
  MPI_Win win;

  win = sc_shmem_array_get_win (array, comm, intranode, internode);

  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  if (!intrarank) {
    mpiret = MPI_Win_unlock(0,win);
    SC_CHECK_MPI(mpiret);
  }
  mpiret = sc_MPI_Barrier(intranode);
  SC_CHECK_MPI(mpiret);
  mpiret = MPI_Win_lock(MPI_LOCK_SHARED,0,MPI_MODE_NOCHECK,win);
  SC_CHECK_MPI(mpiret);
}

#endif /* SC_ENABLE_MPIWINSHARED */

void *
sc_shmem_array_alloc (size_t elem_size, size_t elem_count, sc_MPI_Comm comm)
{
  sc_shmem_array_type_t type;
  sc_MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;

  type = sc_shmem_array_get_type (comm);
  sc_mpi_comm_get_node_comms(comm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    type = SC_SHMEM_ARRAY_BASIC;
  }
  switch (type) {
  case SC_SHMEM_ARRAY_BASIC:
  case SC_SHMEM_ARRAY_PRESCAN:
    return sc_shmem_array_alloc_basic(elem_size, elem_count, comm, intranode, internode);
#if defined(__bgq__)
  case SC_SHMEM_ARRAY_SHARED:
  case SC_SHMEM_ARRAY_SHARED_PRESCAN:
    return sc_shmem_array_alloc_shared(elem_size, elem_count, comm, intranode, internode);
#endif
#if defined(SC_ENABLE_MPIWINSHARED)
  case SC_SHMEM_ARRAY_WINDOW:
  case SC_SHMEM_ARRAY_WINDOW_PRESCAN:
    return sc_shmem_array_alloc_window(elem_size, elem_count, comm, intranode, internode);
#endif
  default:
    SC_ABORT_NOT_REACHED();
  }
  return NULL;
}

void
sc_shmem_array_free (void *array, sc_MPI_Comm comm)
{
  sc_shmem_array_type_t type;
  sc_MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;

  type = sc_shmem_array_get_type (comm);
  sc_mpi_comm_get_node_comms(comm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    type = SC_SHMEM_ARRAY_BASIC;
  }
  switch (type) {
  case SC_SHMEM_ARRAY_BASIC:
  case SC_SHMEM_ARRAY_PRESCAN:
    sc_shmem_array_free_basic(array, comm, intranode, internode);
    break;
#if defined(__bgq__)
  case SC_SHMEM_ARRAY_SHARED:
  case SC_SHMEM_ARRAY_SHARED_PRESCAN:
    sc_shmem_array_free_shared(array, comm, intranode, internode);
    break;
#endif
#if defined(SC_ENABLE_MPIWINSHARED)
  case SC_SHMEM_ARRAY_WINDOW:
  case SC_SHMEM_ARRAY_WINDOW_PRESCAN:
    sc_shmem_array_free_window(array, comm, intranode, internode);
    break;
#endif
  default:
    SC_ABORT_NOT_REACHED();
  }
}

int
sc_shmem_array_write_start (void *array, sc_MPI_Comm comm)
{
  sc_shmem_array_type_t type;
  sc_MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;

  type = sc_shmem_array_get_type (comm);
  sc_mpi_comm_get_node_comms(comm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    type = SC_SHMEM_ARRAY_BASIC;
  }
  switch (type) {
  case SC_SHMEM_ARRAY_BASIC:
  case SC_SHMEM_ARRAY_PRESCAN:
    return sc_shmem_array_write_start_basic(array, comm, intranode, internode);
#if defined(__bgq__)
  case SC_SHMEM_ARRAY_SHARED:
  case SC_SHMEM_ARRAY_SHARED_PRESCAN:
    return sc_shmem_array_write_start_shared(array, comm, intranode, internode);
#endif
#if defined(SC_ENABLE_MPIWINSHARED)
  case SC_SHMEM_ARRAY_WINDOW:
  case SC_SHMEM_ARRAY_WINDOW_PRESCAN:
    return sc_shmem_array_write_start_window(array, comm, intranode, internode);
#endif
  default:
    SC_ABORT_NOT_REACHED();
  }
  return 0;
}

void
sc_shmem_array_write_end (void *array, sc_MPI_Comm comm)
{
  sc_shmem_array_type_t type;
  sc_MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;

  type = sc_shmem_array_get_type (comm);
  sc_mpi_comm_get_node_comms(comm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    type = SC_SHMEM_ARRAY_BASIC;
  }
  switch (type) {
  case SC_SHMEM_ARRAY_BASIC:
  case SC_SHMEM_ARRAY_PRESCAN:
    sc_shmem_array_write_end_basic(array, comm, intranode, internode);
    break;
#if defined(__bgq__)
  case SC_SHMEM_ARRAY_SHARED:
  case SC_SHMEM_ARRAY_SHARED_PRESCAN:
    sc_shmem_array_write_end_shared(array, comm, intranode, internode);
    break;
#endif
#if defined(SC_ENABLE_MPIWINSHARED)
  case SC_SHMEM_ARRAY_WINDOW:
  case SC_SHMEM_ARRAY_WINDOW_PRESCAN:
    sc_shmem_array_write_end_window(array, comm, intranode, internode);
    break;
#endif
  default:
    SC_ABORT_NOT_REACHED();
  }
}

void
sc_shmem_array_memcpy (void *destarray, void *srcarray, size_t bytes, sc_MPI_Comm comm)
{
  sc_shmem_array_type_t type;
  sc_MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;

  type = sc_shmem_array_get_type (comm);
  sc_mpi_comm_get_node_comms(comm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    type = SC_SHMEM_ARRAY_BASIC;
  }
  switch (type) {
  case SC_SHMEM_ARRAY_BASIC:
  case SC_SHMEM_ARRAY_PRESCAN:
    sc_shmem_array_memcpy_basic(destarray, srcarray, bytes, comm, intranode, internode);
    break;
#if defined(__bgq__) || defined(SC_ENABLE_MPIWINSHARED)
#if defined(__bgq__)
  case SC_SHMEM_ARRAY_SHARED:
  case SC_SHMEM_ARRAY_SHARED_PRESCAN:
#endif
#if defined(SC_ENABLE_MPIWINSHARED)
  case SC_SHMEM_ARRAY_WINDOW:
  case SC_SHMEM_ARRAY_WINDOW_PRESCAN:
#endif
    sc_shmem_array_memcpy_common(destarray, srcarray, bytes, comm, intranode, internode);
    break;
#endif
  default:
    SC_ABORT_NOT_REACHED();
  }
}

void
sc_shmem_array_allgather (void *sendbuf, int sendcount, sc_MPI_Datatype sendtype,
                          void *recvbuf, int recvcount, sc_MPI_Datatype recvtype,
                          sc_MPI_Comm comm)
{
  sc_shmem_array_type_t type;
  sc_MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;

  type = sc_shmem_array_get_type (comm);
  sc_mpi_comm_get_node_comms(comm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    type = SC_SHMEM_ARRAY_BASIC;
  }
  switch (type) {
  case SC_SHMEM_ARRAY_BASIC:
  case SC_SHMEM_ARRAY_PRESCAN:
    sc_shmem_array_allgather_basic(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, intranode, internode);
    break;
#if defined(__bgq__) || defined(SC_ENABLE_MPIWINSHARED)
#if defined(__bgq__)
  case SC_SHMEM_ARRAY_SHARED:
  case SC_SHMEM_ARRAY_SHARED_PRESCAN:
#endif
#if defined(SC_ENABLE_MPIWINSHARED)
  case SC_SHMEM_ARRAY_WINDOW:
  case SC_SHMEM_ARRAY_WINDOW_PRESCAN:
#endif
    sc_shmem_array_allgather_common(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, intranode, internode);
    break;
#endif
  default:
    SC_ABORT_NOT_REACHED();
  }
}

void
sc_shmem_array_prefix (void *sendbuf, void *recvbuf, int count, sc_MPI_Datatype dtype,
                       sc_MPI_Op op, sc_MPI_Comm comm)
{
  sc_shmem_array_type_t type;
  sc_MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;

  type = sc_shmem_array_get_type (comm);
  sc_mpi_comm_get_node_comms(comm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    type = SC_SHMEM_ARRAY_BASIC;
  }
  switch (type) {
  case SC_SHMEM_ARRAY_BASIC:
    sc_shmem_array_prefix_basic(sendbuf, recvbuf, count, dtype, op, comm, intranode, internode);
  case SC_SHMEM_ARRAY_PRESCAN:
    sc_shmem_array_prefix_prescan(sendbuf, recvbuf, count, dtype, op, comm, intranode, internode);
    break;
#if defined(__bgq__) || defined(SC_ENABLE_MPIWINSHARED)
#if defined(__bgq__)
  case SC_SHMEM_ARRAY_SHARED:
#endif
#if defined(SC_ENABLE_MPIWINSHARED)
  case SC_SHMEM_ARRAY_WINDOW:
#endif
    sc_shmem_array_prefix_common(sendbuf, recvbuf, count, dtype, op, comm, intranode, internode);
    break;
#endif
#if defined(__bgq__) || defined(SC_ENABLE_MPIWINSHARED)
#if defined(__bgq__)
  case SC_SHMEM_ARRAY_SHARED_PRESCAN:
#endif
#if defined(SC_ENABLE_MPIWINSHARED)
  case SC_SHMEM_ARRAY_WINDOW_PRESCAN:
#endif
    sc_shmem_array_prefix_common_prescan(sendbuf, recvbuf, count, dtype, op, comm, intranode, internode);
    break;
#endif
  default:
    SC_ABORT_NOT_REACHED();
  }
}

#if 0
void
sc_allgather_final_create_default(void *sendbuf, int sendcount, sc_MPI_Datatype sendtype,
                                  void **recvbuf, int recvcount, sc_MPI_Datatype recvtype,
                                  sc_MPI_Comm mpicomm)
{
  size_t typesize;
  int  mpiret, size;
  char *recvchar;

  mpiret = sc_MPI_Comm_size (mpicomm, &size);
  SC_CHECK_MPI(mpiret);

  typesize = sc_mpi_sizeof (recvtype);
  recvchar = SC_ALLOC(char,size * recvcount * typesize);

  mpiret = sc_MPI_Allgather(sendbuf,sendcount,sendtype,recvchar,recvcount,recvtype,mpicomm);
  SC_CHECK_MPI(mpiret);

  *recvbuf = (void *) recvchar;
}

void
sc_allgather_final_scan_create_default(void *sendbuf, void **recvbuf, int count,
                                       sc_MPI_Datatype type, sc_MPI_Op op, sc_MPI_Comm mpicomm)

{
  size_t typesize;
  int  mpiret, p, c, size;
  char *recvchar;

  mpiret = sc_MPI_Comm_size (mpicomm, &size);
  SC_CHECK_MPI(mpiret);

  typesize = sc_mpi_sizeof (type);
  recvchar = SC_ALLOC_ZERO(char,(size + 1) * count * typesize);

  mpiret = sc_MPI_Allgather(sendbuf,count,type,recvchar+count*typesize,count,type,mpicomm);
  SC_CHECK_MPI(mpiret);

  sc_scan_on_array(recvchar,size,count,typesize,type,op);

  *recvbuf = (void *) recvchar;
}

void
sc_allgather_final_scan_create_prescan(void *sendbuf, void **recvbuf, int count,
                                       sc_MPI_Datatype type, sc_MPI_Op op, sc_MPI_Comm mpicomm)

{
  size_t typesize;
  int  mpiret, p, c, size;
  char *prescan, *recvchar;

  mpiret = sc_MPI_Comm_size (mpicomm, &size);
  SC_CHECK_MPI(mpiret);

  typesize = sc_mpi_sizeof (type);
  prescan = SC_ALLOC(char,count * typesize);
  memcpy (prescan, sendbuf, count * typesize);

  mpiret = sc_MPI_Scan(sendbuf, prescan, count, type, op, mpicomm);
  SC_CHECK_MPI(mpiret);

  recvchar = SC_ALLOC_ZERO(char,(size + 1) * count * typesize);

  memset (recvchar,0,count*typesize);

  mpiret = sc_MPI_Allgather(prescan,count,type,recvchar+count*typesize,count,type,mpicomm);
  SC_CHECK_MPI(mpiret);
  SC_FREE(prescan);

  *recvbuf = (void *) recvchar;
}

void
sc_allgather_final_destroy_default(void *recvbuf, sc_MPI_Comm mpicomm)
{
  SC_FREE(recvbuf);
}

#if !defined(SC_AG_FINAL_CREATE_DEFAULT)
#define SC_AG_FINAL_CREATE_DEFAULT sc_allgather_final_create_default
#endif
sc_allgather_final_create_t sc_allgather_final_create = SC_AG_FINAL_CREATE_DEFAULT;

#if !defined(SC_AG_FINAL_SCAN_CREATE_DEFAULT)
#define SC_AG_FINAL_SCAN_CREATE_DEFAULT sc_allgather_final_scan_create_default
#endif
sc_allgather_final_scan_create_t sc_allgather_final_scan_create = SC_AG_FINAL_SCAN_CREATE_DEFAULT;


#if !defined(SC_AG_FINAL_DESTROY_DEFAULT)
#define SC_AG_FINAL_DESTROY_DEFAULT sc_allgather_final_destroy_default
#endif
sc_allgather_final_destroy_t sc_allgather_final_destroy = SC_AG_FINAL_DESTROY_DEFAULT;

/** implement sc_allgather_final when nodes have a shared address space, so
 * that raw pointers can be passed and dereferenced in nodes */
void
sc_allgather_final_create_shared(void *sendbuf, int sendcount, sc_MPI_Datatype sendtype,
                                 void **recvbuf, int recvcount, sc_MPI_Datatype recvtype,
                                 sc_MPI_Comm mpicomm)
{
  MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;
  size_t typesize;
  int  mpiret, size, interrank, intrarank, intrasize;
  char *noderecvchar = NULL;

  typesize = sc_mpi_sizeof (recvtype);

  sc_mpi_comm_get_node_comms(mpicomm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    sc_allgather_final_create_default(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,mpicomm);
    return;
  }

  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(intranode,&intrasize);
  SC_CHECK_MPI(mpiret);
  /* node root gathers from node */
  if (!intrarank) {
    noderecvchar = SC_ALLOC (char,intrasize * recvcount * typesize);
  }
  mpiret = sc_MPI_Gather (sendbuf,sendcount,sendtype,noderecvchar,recvcount,recvtype,0,intranode);
  SC_CHECK_MPI(mpiret);

  /* node root allgathers between nodes */
  if (!intrarank) {
    sc_allgather_final_create_default(noderecvchar,sendcount*intrasize,sendtype,recvbuf,recvcount*intrasize,
                                      recvtype, internode);
    SC_FREE(noderecvchar);
  }
  else {
    *recvbuf = NULL;
  }

#if defined(__bgq__)
  /* these memory sync's are included in Jeff's example */
  /* https://wiki.alcf.anl.gov/parts/index.php/Blue_Gene/Q#Abusing_the_common_heap */
  ppc_msync();
#endif
  mpiret = MPI_Barrier(intranode);
  SC_CHECK_MPI(mpiret);

  /* node root broadcast array start in node */
  mpiret = sc_MPI_Bcast(recvbuf,sizeof(char *),sc_MPI_BYTE,0,intranode);
  SC_CHECK_MPI(mpiret);

#if defined(__bgq__)
  /* these memory sync's are included in Jeff's example */
  /* https://wiki.alcf.anl.gov/parts/index.php/Blue_Gene/Q#Abusing_the_common_heap */
  ppc_msync();
#endif
}

void
sc_allgather_final_scan_create_shared(void *sendbuf, void **recvbuf, int count,
                                       sc_MPI_Datatype type, sc_MPI_Op op, sc_MPI_Comm mpicomm)

{
  MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;
  size_t typesize;
  int  mpiret, size, interrank, intrarank, intrasize, intersize;
  char *noderecvchar = NULL;

  typesize = sc_mpi_sizeof (type);

  sc_mpi_comm_get_node_comms(mpicomm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    sc_allgather_final_scan_create_default(sendbuf,recvbuf,count,type,op,mpicomm);
    return;
  }

  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(intranode,&intrasize);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(internode,&intersize);
  SC_CHECK_MPI(mpiret);
  /* node root gathers from node */
  if (!intrarank) {
    noderecvchar = SC_ALLOC (char,intrasize * count * typesize);
  }
  mpiret = sc_MPI_Gather (sendbuf,count,type,noderecvchar,count,type,0,intranode);
  SC_CHECK_MPI(mpiret);

  /* node root allgathers between nodes and scans */
  if (!intrarank) {
    *recvbuf = SC_ALLOC(char,(intrasize * intersize + 1) * count * typesize);

    memset (*recvbuf,0,count*typesize);

    mpiret = sc_MPI_Allgather(noderecvchar, count*intrasize, type, *recvbuf+count*typesize,
                              count*intrasize, type, internode);

    sc_scan_on_array(*recvbuf,intrasize * intersize, count, typesize, type, op);
    SC_FREE(noderecvchar);
  }
  else {
    *recvbuf = NULL;
  }

#if defined(__bgq__)
  /* these memory sync's are included in Jeff's example */
  /* https://wiki.alcf.anl.gov/parts/index.php/Blue_Gene/Q#Abusing_the_common_heap */
  ppc_msync();
#endif
  mpiret = MPI_Barrier(intranode);
  SC_CHECK_MPI(mpiret);

  /* node root broadcast array start in node */
  mpiret = sc_MPI_Bcast(recvbuf,sizeof(char *),sc_MPI_BYTE,0,intranode);
  SC_CHECK_MPI(mpiret);

#if defined(__bgq__)
  /* these memory sync's are included in Jeff's example */
  /* https://wiki.alcf.anl.gov/parts/index.php/Blue_Gene/Q#Abusing_the_common_heap */
  ppc_msync();
#endif
}

void
sc_allgather_final_scan_create_shared_prescan(void *sendbuf, void **recvbuf, int count,
                                              sc_MPI_Datatype type, sc_MPI_Op op, sc_MPI_Comm mpicomm)

{
  MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;
  size_t typesize;
  int  mpiret, size, interrank, intrarank, intrasize, intersize;
  char *noderecvchar = NULL;
  char *prescan = NULL;

  typesize = sc_mpi_sizeof (type);

  sc_mpi_comm_get_node_comms(mpicomm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    sc_allgather_final_scan_create_prescan(sendbuf,recvbuf,count,type,op,mpicomm);
    return;
  }

  prescan = SC_ALLOC(char,count * typesize);
  memcpy (prescan, sendbuf, count * typesize);

  mpiret = sc_MPI_Scan(sendbuf, prescan, count, type, op, mpicomm);
  SC_CHECK_MPI(mpiret);

  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(intranode,&intrasize);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(internode,&intersize);
  SC_CHECK_MPI(mpiret);
  /* node root gathers from node */
  if (!intrarank) {
    noderecvchar = SC_ALLOC (char,intrasize * count * typesize);
  }
  mpiret = sc_MPI_Gather (prescan,count,type,noderecvchar,count,type,0,intranode);
  SC_CHECK_MPI(mpiret);
  SC_FREE(prescan);

  /* node root allgathers between nodes and scans */
  if (!intrarank) {
    *recvbuf = SC_ALLOC(char,(intrasize * intersize + 1) * count * typesize);

    memset (*recvbuf,0,count*typesize);

    mpiret = sc_MPI_Allgather(noderecvchar, count*intrasize, type, *recvbuf+count*typesize,
                              count*intrasize, type, internode);
    SC_FREE(noderecvchar);
  }
  else {
    *recvbuf = NULL;
  }

#if defined(__bgq__)
  /* these memory sync's are included in Jeff's example */
  /* https://wiki.alcf.anl.gov/parts/index.php/Blue_Gene/Q#Abusing_the_common_heap */
  ppc_msync();
#endif
  mpiret = MPI_Barrier(intranode);
  SC_CHECK_MPI(mpiret);

  /* node root broadcast array start in node */
  mpiret = sc_MPI_Bcast(recvbuf,sizeof(char *),sc_MPI_BYTE,0,intranode);
  SC_CHECK_MPI(mpiret);

#if defined(__bgq__)
  /* these memory sync's are included in Jeff's example */
  /* https://wiki.alcf.anl.gov/parts/index.php/Blue_Gene/Q#Abusing_the_common_heap */
  ppc_msync();
#endif
}

void
sc_allgather_final_destroy_shared(void *recvbuf, sc_MPI_Comm mpicomm)
{
  MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;
  int  mpiret, size, intersize, interrank, intrarank, intrasize;
  char *noderecvchar = NULL;

  sc_mpi_comm_get_node_comms(mpicomm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    sc_allgather_final_destroy_default(recvbuf,mpicomm);
    return;
  }
  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  if (!intrarank) {
    SC_FREE(recvbuf);
  }
}

void
sc_allgather_final_create_window(void *sendbuf, int sendcount, sc_MPI_Datatype sendtype,
                                 void **recvbuf, int recvcount, sc_MPI_Datatype recvtype,
                                 sc_MPI_Comm mpicomm)
{
#if defined(SC_ENABLE_MPIWINSHARED)
  MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;
  MPI_Win win;
  int typesize;
  int disp_unit;
  int  mpiret, size, intersize, interrank, intrarank, intrasize;
  char *noderecvchar = NULL;
  MPI_Aint winsize = 0;

  typesize = (int) sc_mpi_sizeof (recvtype);

  sc_mpi_comm_get_node_comms(mpicomm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    sc_allgather_final_create_default(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,mpicomm);
    return;
  }

  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(intranode,&intrasize);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(internode,&intersize);
  SC_CHECK_MPI(mpiret);
  /* node root gathers from node */
  if (!intrarank) {
    noderecvchar = SC_ALLOC (char,intrasize * recvcount * typesize);
  }
  mpiret = sc_MPI_Gather (sendbuf,sendcount,sendtype,noderecvchar,recvcount,recvtype,0,intranode);
  SC_CHECK_MPI(mpiret);

  /* create the shared array */
  disp_unit = SC_MAX(typesize,sizeof (MPI_Win));
  if (!intrarank) {
    winsize = intrasize * intersize * recvcount * typesize + intrasize * sizeof (MPI_Win);
    if (winsize % disp_unit) {
      winsize = ((winsize / disp_unit) + 1) * disp_unit;
    }
  }
  mpiret = MPI_Win_allocate_shared(winsize,disp_unit,MPI_INFO_NULL,intranode,recvbuf,&win);
  SC_CHECK_MPI(mpiret);
  mpiret = MPI_Win_shared_query(win,0,&winsize,&disp_unit,recvbuf);
  SC_CHECK_MPI(mpiret);
  /* store the windows at the front of the array */
  mpiret = sc_MPI_Gather (&win,sizeof(MPI_Win),sc_MPI_BYTE,
                          *recvbuf,sizeof(MPI_Win),sc_MPI_BYTE,
                          0,intranode);
  SC_CHECK_MPI(mpiret);
  {
    MPI_Win *shiftedbuf = ((MPI_Win *) *recvbuf) + intrasize;
    *recvbuf = (void *) shiftedbuf;
  }
  /* node root allgathers between nodes */
  if (!intrarank) {
    mpiret = MPI_Win_lock(MPI_LOCK_EXCLUSIVE,0,MPI_MODE_NOCHECK,win);
    SC_CHECK_MPI(mpiret);
    mpiret = sc_MPI_Allgather(noderecvchar,intrasize*recvcount,recvtype,
                              *recvbuf,intrasize*recvcount,recvtype,internode);
    SC_CHECK_MPI(mpiret);
    mpiret = MPI_Win_unlock(0,win);
    SC_CHECK_MPI(mpiret);
    SC_FREE(noderecvchar);
  }
  mpiret = sc_MPI_Barrier(intranode);
  SC_CHECK_MPI(mpiret);

  /* open access */
  mpiret = MPI_Win_lock(MPI_LOCK_SHARED,0,MPI_MODE_NOCHECK,win);
  SC_CHECK_MPI(mpiret);
#else
  sc_allgather_final_create_default(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,mpicomm);
#endif
}

void
sc_allgather_final_scan_create_window(void *sendbuf, void **recvbuf, int count,
                                      sc_MPI_Datatype type, sc_MPI_Op op, sc_MPI_Comm mpicomm)

{
#if defined(SC_ENABLE_MPIWINSHARED)
  MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;
  size_t typesize;
  int  mpiret, size, interrank, intrarank, intrasize, intersize;
  char *noderecvchar = NULL;
  MPI_Aint winsize = 0;
  int disp_unit;
  MPI_Win win;

  typesize = sc_mpi_sizeof (type);

  sc_mpi_comm_get_node_comms(mpicomm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    sc_allgather_final_scan_create_default(sendbuf,recvbuf,count,type,op,mpicomm);
    return;
  }

  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(intranode,&intrasize);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(internode,&intersize);
  SC_CHECK_MPI(mpiret);
  /* node root gathers from node */
  if (!intrarank) {
    noderecvchar = SC_ALLOC (char,intrasize * count * typesize);
  }
  mpiret = sc_MPI_Gather (sendbuf,count,type,noderecvchar,count,type,0,intranode);
  SC_CHECK_MPI(mpiret);

  /* create the shared array */
  disp_unit = SC_MAX(typesize,sizeof (MPI_Win));
  if (!intrarank) {
    winsize = (intrasize * intersize + 1) * count * typesize + intrasize * sizeof (MPI_Win);
    if (winsize % disp_unit) {
      winsize = ((winsize / disp_unit) + 1) * disp_unit;
    }
  }
  mpiret = MPI_Win_allocate_shared(winsize,disp_unit,MPI_INFO_NULL,intranode,recvbuf,&win);
  SC_CHECK_MPI(mpiret);
  mpiret = MPI_Win_shared_query(win,0,&winsize,&disp_unit,recvbuf);
  SC_CHECK_MPI(mpiret);
  /* store the windows at the front of the array */
  mpiret = sc_MPI_Gather (&win,sizeof(MPI_Win),sc_MPI_BYTE,
                          *recvbuf,sizeof(MPI_Win),sc_MPI_BYTE,
                          0,intranode);
  SC_CHECK_MPI(mpiret);
  {
    MPI_Win *shiftedbuf = ((MPI_Win *) *recvbuf) + intrasize;
    *recvbuf = (void *) shiftedbuf;
  }
  /* node root allgathers between nodes */
  if (!intrarank) {
    mpiret = MPI_Win_lock(MPI_LOCK_EXCLUSIVE,0,MPI_MODE_NOCHECK,win);
    SC_CHECK_MPI(mpiret);
    memset (*recvbuf,0,count*typesize);
    mpiret = sc_MPI_Allgather(noderecvchar,intrasize*count,type,
                              *recvbuf+count*typesize,intrasize*count,type,internode);
    SC_CHECK_MPI(mpiret);
    sc_scan_on_array(*recvbuf,intrasize * intersize, count, typesize, type, op);
    mpiret = MPI_Win_unlock(0,win);
    SC_CHECK_MPI(mpiret);
    SC_FREE(noderecvchar);
  }
  mpiret = sc_MPI_Barrier(intranode);
  SC_CHECK_MPI(mpiret);

  /* open access */
  mpiret = MPI_Win_lock(MPI_LOCK_SHARED,0,MPI_MODE_NOCHECK,win);
  SC_CHECK_MPI(mpiret);
#else
  sc_allgather_final_scan_create_default(sendbuf,recvbuf,count,type,op,mpicomm);
#endif
}

void
sc_allgather_final_scan_create_window_prescan(void *sendbuf, void **recvbuf, int count,
                                              sc_MPI_Datatype type, sc_MPI_Op op, sc_MPI_Comm mpicomm)

{
#if defined(SC_ENABLE_MPIWINSHARED)
  MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;
  size_t typesize;
  int  mpiret, size, interrank, intrarank, intrasize, intersize;
  char *noderecvchar = NULL;
  int disp_unit;
  MPI_Aint winsize = 0;
  MPI_Win win;
  char *prescan = NULL;

  typesize = sc_mpi_sizeof (type);

  sc_mpi_comm_get_node_comms(mpicomm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    sc_allgather_final_scan_create_default(sendbuf,recvbuf,count,type,op,mpicomm);
    return;
  }

  prescan = SC_ALLOC(char,count * typesize);
  memcpy (prescan, sendbuf, count * typesize);

  mpiret = sc_MPI_Scan(sendbuf, prescan, count, type, op, mpicomm);
  SC_CHECK_MPI(mpiret);

  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(intranode,&intrasize);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(internode,&intersize);
  SC_CHECK_MPI(mpiret);
  /* node root gathers from node */
  if (!intrarank) {
    noderecvchar = SC_ALLOC (char,intrasize * count * typesize);
  }
  mpiret = sc_MPI_Gather (prescan,count,type,noderecvchar,count,type,0,intranode);
  SC_CHECK_MPI(mpiret);
  SC_FREE(prescan);

  /* create the shared array */
  disp_unit = SC_MAX(typesize,sizeof (MPI_Win));
  if (!intrarank) {
    winsize = (intrasize * intersize + 1) * count * typesize + intrasize * sizeof (MPI_Win);
    if (winsize % disp_unit) {
      winsize = ((winsize / disp_unit) + 1) * disp_unit;
    }
  }
  mpiret = MPI_Win_allocate_shared(winsize,disp_unit,MPI_INFO_NULL,intranode,recvbuf,&win);
  SC_CHECK_MPI(mpiret);
  mpiret = MPI_Win_shared_query(win,0,&winsize,&disp_unit,recvbuf);
  SC_CHECK_MPI(mpiret);
  /* store the windows at the front of the array */
  mpiret = sc_MPI_Gather (&win,sizeof(MPI_Win),sc_MPI_BYTE,
                          *recvbuf,sizeof(MPI_Win),sc_MPI_BYTE,
                          0,intranode);
  SC_CHECK_MPI(mpiret);
  {
    MPI_Win *shiftedbuf = ((MPI_Win *) *recvbuf) + intrasize;
    *recvbuf = (void *) shiftedbuf;
  }
  /* node root allgathers between nodes */
  if (!intrarank) {
    mpiret = MPI_Win_lock(MPI_LOCK_EXCLUSIVE,0,MPI_MODE_NOCHECK,win);
    SC_CHECK_MPI(mpiret);
    memset (*recvbuf,0,count*typesize);
    mpiret = sc_MPI_Allgather(noderecvchar,intrasize*count,type,
                              *recvbuf+count*typesize,intrasize*count,type,internode);
    SC_CHECK_MPI(mpiret);
    mpiret = MPI_Win_unlock(0,win);
    SC_CHECK_MPI(mpiret);
    SC_FREE(noderecvchar);
  }
  mpiret = sc_MPI_Barrier(intranode);
  SC_CHECK_MPI(mpiret);

  /* open access */
  mpiret = MPI_Win_lock(MPI_LOCK_SHARED,0,MPI_MODE_NOCHECK,win);
  SC_CHECK_MPI(mpiret);

#else
  sc_allgather_final_scan_create_default(sendbuf,recvbuf,count,type,op,mpicomm);
#endif
}

void
sc_allgather_final_destroy_window(void *recvbuf, sc_MPI_Comm mpicomm)
{
#if defined(SC_ENABLE_MPIWINSHARED)
  MPI_Comm intranode = sc_MPI_COMM_NULL, internode = sc_MPI_COMM_NULL;
  int  mpiret, size, intersize, interrank, intrarank, intrasize;
  char *noderecvchar = NULL;
  MPI_Win win;

  sc_mpi_comm_get_node_comms(mpicomm,&intranode,&internode);
  if (intranode == sc_MPI_COMM_NULL || internode == sc_MPI_COMM_NULL) {
    sc_allgather_final_destroy_default(recvbuf,mpicomm);
    return;
  }
  mpiret = sc_MPI_Comm_rank(intranode,&intrarank);
  SC_CHECK_MPI(mpiret);
  mpiret = sc_MPI_Comm_size(intranode,&intrasize);
  SC_CHECK_MPI(mpiret);

  /* get the stashed window */
  win = ((MPI_Win *) recvbuf)[-intrasize + intrarank];

  mpiret = MPI_Win_unlock(0,win);
  SC_CHECK_MPI(mpiret);

  mpiret = MPI_Win_free(&win);
#else
  sc_allgather_final_destroy_default(recvbuf,mpicomm);
#endif
}
#endif
