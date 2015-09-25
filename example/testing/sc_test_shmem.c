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

#define DATA_SIZE 10

/* This struct stores data which we use to test shared
 * memory arrays.
 */
typedef struct
{
  int                 rank;     /*< This entry stores the rank of the creating process */
  double              data[DATA_SIZE];  /*< This field can store arbitrary data */
} data_t;

/* For each process print the integer entry of
 * each element in an array of type data_t.
 */
void
test_shmem_print_int (data_t * array, sc_MPI_Comm comm)
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

/* Check whether a given data item has entries
 * data.rank = i
 * data.data = {0,...,DATA_SIZE-1}
 */
int
test_shmem_correct_data (data_t * data, int i)
{
  int                 j;
  if (data->rank != i) {
    return 0;
  }
  for (j = 0; j < DATA_SIZE; j++) {
    if (data->data[j] != (double) j) {
      return 0;
    }
  }
  return 1;
}

/* Fill the array of one data item with
 * the numbers 0,...,DATA_SIZE -1.
 */
void
test_shmem_fill_data (data_t * data)
{
  int                 i;

  for (i = 0; i < DATA_SIZE; i++) {
    data->data[i] = (double) i;
  }
}

/* For a given shmem type, allocate a shared array
 * and fill it with data via a call to shmem_allgather.
 * We check wether all data was gathered correctly and
 * free the array.
 */
void
test_shmem_allgather (sc_shmem_type_t type)
{
  data_t             *data_array;
  data_t              data;
  int                 mpirank, mpisize, mpiret;
  int                 i;

  SC_GLOBAL_ESSENTIALF ("Testing allgather with type %s.\n",
                        sc_shmem_type_to_string[type]);
  mpiret = sc_MPI_Comm_size (sc_MPI_COMM_WORLD, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (sc_MPI_COMM_WORLD, &mpirank);
  SC_CHECK_MPI (mpiret);
  data.rank = mpirank;
  test_shmem_fill_data (&data);

  sc_shmem_set_type (sc_MPI_COMM_WORLD, type);

  data_array = SC_SHMEM_ALLOC (data_t, mpisize, sc_MPI_COMM_WORLD);
  SC_CHECK_ABORT (data_array != NULL, "Allocation failed");

  sc_shmem_allgather (&data, sizeof (data_t), sc_MPI_BYTE, data_array,
                      sizeof (data_t), sc_MPI_BYTE, sc_MPI_COMM_WORLD);
  for (i = 0; i < mpisize; i++) {
    SC_CHECK_ABORTF (test_shmem_correct_data (&data_array[i], i),
                     "Error in shmem_allgather. Array entry %i is not correct.",
                     i);
  }
  SC_SHMEM_FREE (data_array, sc_MPI_COMM_WORLD);

  SC_GLOBAL_ESSENTIALF ("Testing type %s succesful.\n",
                        sc_shmem_type_to_string[type]);
}

void
test_shmem_test1 ()
{
  sc_shmem_type_t     type;

  for (type = SC_SHMEM_BASIC; type < SC_SHMEM_NUM_TYPES; type++) {
    test_shmem_allgather (type);
  }
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
