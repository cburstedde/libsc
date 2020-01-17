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

#include <sc3_array.h>
#include <sc3_error.h>
#include <sc3_openmp.h>
#include <sc3_mpi.h>

#if 0
#define SC3_BASICS_DEALLOCATE
#endif

static sc3_error_t *
unravel_error (sc3_error_t ** ep)
{
  int                 j;
  int                 line;
  char               *bname;
  const char         *filename, *errmsg;
  sc3_error_t        *e, *stack;

  SC3E_INULLP (ep, e);

  j = 0;
  while (e != NULL) {
    /* print error information */
    SC3E (sc3_error_get_location (e, &filename, &line));
    if ((bname = strdup (filename)) != NULL) {
      filename = sc3_basename (bname);
    }
    SC3E (sc3_error_get_message (e, &errmsg));
    printf ("Error stack %d:%s:%d: %s\n", j, filename, line, errmsg);
    free (bname);

    /* go down the stack */
    SC3E (sc3_error_get_stack (e, &stack));
    SC3E (sc3_error_destroy (&e));
    e = stack;
    ++j;
  }
  return NULL;
}

static sc3_error_t *
child_function (int a, int *result)
{
  SC3E_RETVAL (result, 0);
  SC3A_CHECK (a < 50);

  *result = a + 1;
  return NULL;
}

static sc3_error_t *
parent_function (int a, int *result)
{
  SC3E_RETVAL (result, 0);
  SC3A_CHECK (a < 100);
  SC3E (child_function (a, result));

  *result *= 3;
  return NULL;
}

static sc3_error_t *
io_error (sc3_allocator_t * a,
          const char *filename, int line, const char *errmsg)
{
  sc3_error_t        *e;

  SC3E (sc3_error_new (a, &e));
  SC3E (sc3_error_set_location (e, filename, line));
  SC3E (sc3_error_set_message (e, errmsg));
  SC3E (sc3_error_set_severity (e, SC3_ERROR_RUNTIME));
  SC3E (sc3_error_setup (e));

  return e;
}

#define SC3_BASICS_IO_ERROR(a,m) (io_error (a, __FILE__, __LINE__, m))

static sc3_error_t *
run_io (sc3_allocator_t * a, int result)
{
  FILE               *file;

  if ((file = fopen ("sc3_basics_run_io.txt", "wb")) == NULL) {
    return SC3_BASICS_IO_ERROR (a, "File open failed");
  }
  if (fprintf (file, "Hello world from sc3_basics %d\n", result) < 0) {
    (void) fclose (file);
    return SC3_BASICS_IO_ERROR (a, "File fprintf failed");
  }
  if (fclose (file)) {
    return SC3_BASICS_IO_ERROR (a, "File close failed");
  }

  return NULL;
}

static sc3_error_t *
run_prog (sc3_allocator_t * origa, int input, int *result, int *num_io)
{
  sc3_error_t        *e, *e2;
  sc3_allocator_t    *a;

  /* Test assertions */
  SC3E (parent_function (input, result));
  SC3A_CHECK (num_io != NULL);

  /* Make allocator for this context block */
  SC3E (sc3_allocator_new (origa, &a));
  SC3E (sc3_allocator_setup (a));

  /* Test file input/output and recoverable errors */
  if ((e = run_io (a, *result)) != NULL) {
    ++*num_io;

#ifdef SC3_BASICS_DEALLOCATE
    /* do something with the runtime error */
    SC3E (unravel_error (&e));

    /* return a new error to the outside */
    /* TODO: this will not be practicable.
     * origa must not be used since it may be shared between threads. */
    SC3E (sc3_error_new (origa, &e2));
#else
    /* return the original error to the outside */
    SC3E (sc3_error_new (a, &e2));
    SC3E (sc3_error_set_stack (e2, &e));
#endif
    SC3E (sc3_error_set_location (e2, __FILE__, __LINE__));
    SC3E (sc3_error_set_message (e2, "Encountered I/O error"));
    SC3E (sc3_error_set_severity (e2, SC3_ERROR_RUNTIME));
    SC3E (sc3_error_setup (e2));
    SC3A_CHECK (e == NULL);
    e = e2;
  }

  /* If we return before here, we will never destroy the allocator.
     This is ok if we only do this on fatal errors. */

  /* TODO: If e is set and we return a new fatal error, we will never
     report on e, even if e is inderectly responsible for the error. */

#ifdef SC3_BASICS_DEALLOCATE
  /* The allocator is now done.
     Must not pass any allocated objects to the outside of this function. */
  SC3E (sc3_allocator_destroy (&a));
#else
  /* We allow allocated objects to be passed to the outside.
     These carry references to this allocator beyond this scope. */
  SC3E (sc3_allocator_unref (&a));
#endif

  /* Make sure not to mess with this error variable in between */
  return e;
}

static int
main_error_check (sc3_error_t ** ep, int *num_fatal, int *num_weird)
{
  sc3_error_t        *e;

  if (num_fatal == NULL || num_weird == NULL) {
    return -1;
  }

  if (ep != NULL && *ep != NULL) {
    if (sc3_error_is_fatal (*ep, NULL)) {
      ++*num_fatal;
    }

    /* unravel error stack and print messages */
    if ((e = unravel_error (ep)) != NULL) {
      ++num_weird;
      if (sc3_error_destroy (&e) != NULL) {
        ++num_weird;
      }
    }
    return -1;
  }
  return 0;
}

static sc3_error_t *
test_array (sc3_allocator_t * ator)
{
  char               *abc;
  sc3_array_t        *arr;

  SC3E (sc3_allocator_strdup (ator, "abc", &abc));
  SC3E_ALLOCATOR_FREE (ator, char, abc);

  SC3E (sc3_array_new (ator, &arr));
  SC3E (sc3_array_set_elem_size (arr, 0));
  SC3E_DEMIS (sc3_array_is_new, arr);

  SC3E (sc3_array_setup (arr));
  SC3E_DEMIS (!sc3_array_is_new, arr);
  SC3E_DEMIS (sc3_array_is_setup, arr);

  SC3E (sc3_array_destroy (&arr));
  return NULL;
}

static sc3_error_t *
test_mpi (int *rank)
{
  sc3_MPI_Comm_t      mpicomm = SC3_MPI_COMM_WORLD;
  sc3_MPI_Comm_t      sharedcomm, headcomm;
  sc3_MPI_Win_t       sharedwin;
  int                 size, sharedsize, sharedrank, headsize, headrank;
  int                *sharedptr;
  int                 bytesize;

  SC3E (sc3_MPI_Comm_set_errhandler (mpicomm, SC3_MPI_ERRORS_RETURN));

  SC3E (sc3_MPI_Comm_size (mpicomm, &size));
  SC3E (sc3_MPI_Comm_rank (mpicomm, rank));

  SC3E_DEMAND (0 <= *rank && *rank < size, "Rank out of range");
  printf ("MPI size %d rank %d\n", size, *rank);

  /* create intra-node communicator */
  SC3E (sc3_MPI_Comm_split_type (mpicomm, SC3_MPI_COMM_TYPE_SHARED,
                                 0, SC3_MPI_INFO_NULL, &sharedcomm));
  SC3E (sc3_MPI_Comm_size (sharedcomm, &sharedsize));
  SC3E (sc3_MPI_Comm_rank (sharedcomm, &sharedrank));
  printf ("MPI size %d rank %d shared size %d rank %d\n",
          size, *rank, sharedsize, sharedrank);

  /* allocate shared memory */
  bytesize = sharedrank == 0 ? sizeof (int) : 0;
  SC3E (sc3_MPI_Win_allocate_shared (bytesize, 1, SC3_MPI_INFO_NULL,
                                     sharedcomm, &sharedptr, &sharedwin));
  if (sharedrank == 0) {
    sharedptr[0] = 0;
  }
  SC3E (sc3_MPI_Win_free (&sharedwin));

  /* create communicator with the first rank on each node */
  SC3E (sc3_MPI_Comm_split (mpicomm, sharedrank == 0 ? 0 :
                            SC3_MPI_UNDEFINED, 0, &headcomm));
  SC3A_CHECK ((sharedrank != 0) == (headcomm == SC3_MPI_COMM_NULL));
  if (headcomm != SC3_MPI_COMM_NULL) {
    SC3E (sc3_MPI_Comm_size (headcomm, &headsize));
    SC3E (sc3_MPI_Comm_rank (headcomm, &headrank));
    printf
      ("MPI size %d rank %d shared size %d rank %d head size %d rank %d\n",
       size, *rank, sharedsize, sharedrank, headsize, headrank);
    SC3E (sc3_MPI_Comm_free (&headcomm));
  }

  /* clean up user communicators */
  SC3E (sc3_MPI_Comm_free (&sharedcomm));

  return NULL;
}

static sc3_error_t *
openmp_info (void)
{
  int                 tmax = sc3_openmp_get_max_threads ();
  int                 minid, maxid, tcount;

  printf ("Max threads %d\n", tmax);

  minid = tmax;
  maxid = -1;
  tcount = 0;

#pragma omp parallel reduction (min: minid) \
                     reduction (max: maxid) \
                     reduction (+: tcount)
  {
    int                 tnum = sc3_openmp_get_num_threads ();
    int                 tid = sc3_openmp_get_thread_num ();

    printf ("Thread %d out of %d\n", tid, tnum);

    minid = SC3_MIN (minid, tid);
    maxid = SC3_MAX (maxid, tid);
    ++tcount;
  }
  printf ("Reductions min %d max %d count %d\n", minid, maxid, tcount);

  SC3E_DEMAND (0 <= minid && minid <= maxid && maxid < tmax,
               "Thread numbers out of range");
  return NULL;
}

int
main (int argc, char **argv)
{
  const int           inputs[3] = { 167, 84, 23 };
  int                 mpirank;
  int                 input;
  int                 result;
  int                 num_fatal, num_weird, num_io;
  int                 i;
  char                reason[SC3_BUFSIZE];
  sc3_error_t        *e;
  sc3_allocator_t    *a;
  sc3_allocator_t    *mainalloc;

  mainalloc = sc3_allocator_nothread ();
  num_fatal = num_weird = num_io = 0;

  SC3E_SET (e, sc3_MPI_Init (&argc, &argv));
  if (main_error_check (&e, &num_fatal, &num_weird)) {
    printf ("MPI_Init failed\n");
    goto main_end;
  }

  SC3E_SET (e, sc3_allocator_new (mainalloc, &a));
  SC3E_NULL_SET (e, sc3_allocator_setup (a));
  if (main_error_check (&e, &num_fatal, &num_weird)) {
    printf ("Main allocator_new failed\n");
    goto main_end;
  }

  SC3E_SET (e, test_array (a));
  if (!main_error_check (&e, &num_fatal, &num_weird)) {
    printf ("Array test ok\n");
  }

  SC3E_SET (e, test_mpi (&mpirank));
  if (!main_error_check (&e, &num_fatal, &num_weird)) {
    printf ("MPI code ok\n");
  }

  SC3E_SET (e, openmp_info ());
  if (!main_error_check (&e, &num_fatal, &num_weird)) {
    printf ("OpenMP code ok\n");
  }

  for (i = 0; i < 3; ++i) {
    input = inputs[i];
    SC3E_SET (e, run_prog (a, input, &result, &num_io));
    if (!main_error_check (&e, &num_fatal, &num_weird)) {
      printf ("Clean execution with input %d result %d\n", input, result);
    }
  }

  SC3E_SET (e, sc3_allocator_destroy (&a));
  if (main_error_check (&e, &num_fatal, &num_weird)) {
    printf ("Main allocator destroy failed\n");
  }
  if (!sc3_allocator_is_free (mainalloc, reason)) {
    printf ("Static allocator not free: %s\n", reason);
  }

  SC3E_SET (e, sc3_MPI_Finalize ());
  if (main_error_check (&e, &num_fatal, &num_weird)) {
    printf ("MPI_Finalize failed\n");
  }

main_end:
  printf ("Fatal errors %d weird %d IO %d\n", num_fatal, num_weird, num_io);

  return EXIT_SUCCESS;
}
