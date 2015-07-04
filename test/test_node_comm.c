/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2015 The University of Texas System

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

#include <sc.h>
#include <sc_mpi.h>
#include <sc_shmem_array.h>

void
test_shmem_array (int count, sc_MPI_Comm comm, sc_shmem_array_type_t type)
{
  int i, p, size, mpiret, check;
  long int *myval, *recv_self, *recv_shmem, *scan_self, *scan_shmem;

  SC_GLOBAL_PRODUCTIONF ("type %d\n", type);

  sc_shmem_array_set_type(comm, type);

  mpiret = sc_MPI_Comm_size(comm, &size);
  SC_CHECK_MPI(mpiret);

  myval = SC_ALLOC(long int,count);
  for (i = 0; i < count; i++) {
    myval[i] = random();
  }

  recv_self = SC_ALLOC(long int,count * size);
  scan_self = SC_ALLOC(long int,count * (size + 1));
  mpiret = sc_MPI_Allgather(myval,count,sc_MPI_LONG,
                            recv_self,count,sc_MPI_LONG, comm);
  SC_CHECK_MPI(mpiret);

  for (i = 0; i < count; i++) {
    scan_self[i] = 0;
  }
  for (p = 0; p < size; p++) {
    for (i = 0; i < count; i++) {
      scan_self[count * (p + 1) + i] = scan_self[count * p + i] + recv_self[count * p + i];
    }
  }

  recv_shmem = sc_shmem_array_alloc(sizeof (long int),count * size,comm);
  sc_shmem_array_allgather(myval,count,sc_MPI_LONG,
                           recv_shmem,count,sc_MPI_LONG,
                           comm);
  check = memcmp(recv_self,recv_shmem,count * sizeof(long int)*size);
  SC_CHECK_ABORTF(!check,"sc_shmem_array_allgather does not reproduce for type %d, count %d\n",type,count);
  sc_shmem_array_free(recv_shmem,comm);

  scan_shmem = sc_shmem_array_alloc(sizeof (long int),count * (size + 1),comm);
  sc_shmem_array_prefix(myval,scan_shmem,count,sc_MPI_LONG,sc_MPI_SUM,comm);
  check = memcmp(scan_self,scan_shmem,count * sizeof(long int)*(size + 1));
  SC_CHECK_ABORTF(!check,"sc_shmem_array_prefix does not reproduce for type %d, count %d\n",type,count);
  sc_shmem_array_free(scan_shmem,comm);

  SC_FREE (scan_self);
  SC_FREE (recv_self);
  SC_FREE (myval);
}

int
main (int argc, char **argv)
{
  sc_MPI_Comm   intranode = sc_MPI_COMM_NULL;
  sc_MPI_Comm   internode = sc_MPI_COMM_NULL;
  int           mpiret, node_size = 1, rank, size;
  int           first, intrarank, interrank, count;
  sc_shmem_array_type_t type;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_size (MPI_COMM_WORLD, &size);
  SC_CHECK_MPI (mpiret);

  sc_init (MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

  srandom(rank);
  for (type = 0; type < SC_SHMEM_ARRAY_NUM_TYPES; type++) {
    for (count = 1; count <= 3; count++) {
      test_shmem_array (count, MPI_COMM_WORLD, type);
    }
  }

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);
  return 0;
}
