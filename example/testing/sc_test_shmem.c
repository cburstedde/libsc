/*
  This file is part of t8code.
  t8code is a C library to manage a collection (a forest) of multiple
  connected adaptive space-trees of general element types in parallel.

  Copyright (C) 2010 The University of Texas System
  Written by Carsten Burstedde, Lucas C. Wilcox, and Tobin Isaac

  t8code is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  t8code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with t8code; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <mpi.h>
#include <sc_refcount.h>
#include <sc_shmem.h>

#define DATA_SIZE 1000

typedef struct {
	int    rank;
	double data[DATA_SIZE];
} data_t;

void
test_shmem_print_int (data_t *array, sc_MPI_Comm comm)
{
  int                 i, p;
  MPI_Aint            address;
  int                 mpirank, mpisize, mpiret;
  char                outstring[BUFSIZ];

  mpiret = sc_MPI_Comm_size (comm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (comm, &mpirank);
  SC_CHECK_MPI (mpiret);

  mpiret = MPI_Get_address ((void *) array, &address);
  SC_CHECK_MPI (mpiret);
  outstring[0] = '\0';
  snprintf (outstring + strlen (outstring), BUFSIZ - strlen (outstring),
            "Array at %li:\t", (long) address);
  for (i = 0; i < mpisize; i++)
    /* loop over array entries */
  {
    snprintf (outstring + strlen (outstring), BUFSIZ - strlen (outstring),
              "%i ", array[i].rank);
  }

  for (p = 0; p < mpisize; p++)
    /* loop over procs */
  {
    if (mpirank == p) {
      printf ("[H %i] %s\n", mpirank, outstring);
      outstring[0] = '\0';
      fflush (stdout);
    }
    sc_MPI_Barrier (comm);
  }

}

void
test_shmem_test1 ()
{
  sc_shmem_type_t     type = SC_SHMEM_NOT_SET;
  int                 mpirank, mpisize, mpiret;
  data_t             *rankarray;

  mpiret = sc_MPI_Comm_size (sc_MPI_COMM_WORLD, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (sc_MPI_COMM_WORLD, &mpirank);
  SC_CHECK_MPI (mpiret);

#if defined SC_ENABLE_MPIWINSHARED
  type = SC_SHMEM_WINDOW;
#endif
#if defined __bgq__
  type = SC_SHMEM_BGQ;
#endif

  if (type == SC_SHMEM_NOT_SET) {
    type = SC_SHMEM_BASIC;
  }
  sc_shmem_set_type (sc_MPI_COMM_WORLD, type);
  SC_GLOBAL_ESSENTIALF ("My type is %s.\n", sc_shmem_type_to_string[type]);

  rankarray = SC_SHMEM_ALLOC (data_t, mpisize, sc_MPI_COMM_WORLD);

  sc_shmem_allgather (&mpirank, sizeof(data_t), sc_MPI_BYTE, rankarray, sizeof(data_t), sc_MPI_BYTE,
                      sc_MPI_COMM_WORLD);

  test_shmem_print_int (rankarray, sc_MPI_COMM_WORLD);

  SC_SHMEM_FREE (rankarray, sc_MPI_COMM_WORLD);
}

int
main (int argc, char *argv[])
{
  int                 mpiret;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_ESSENTIAL);

  test_shmem_test1 ();

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);
  return 0;
}
