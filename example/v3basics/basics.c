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
#include <sc3_log.h>
#include <sc3_mpi.h>
#include <sc3_omp.h>
#include <sc3_trace.h>

static int          provoke_fatal = 0;

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
make_io_error (sc3_allocator_t * a,
               const char *filename, int line, const char *errmsg)
{
  sc3_error_t        *e;

  SC3A_IS (sc3_allocator_is_setup, a);

  SC3E (sc3_error_new (a, &e));
  SC3E (sc3_error_set_location (e, filename, line));
  SC3E (sc3_error_set_message (e, errmsg));
  SC3E (sc3_error_set_kind (e, SC3_ERROR_IO));
  SC3E (sc3_error_setup (e));

  return e;
}

#define SC3_BASICS_IO_ERROR(a,m) (make_io_error (a, __FILE__, __LINE__, m))

/* TODO: differentiate file names by rank */
static sc3_error_t *
run_io (sc3_allocator_t * a, int result)
{
  FILE               *file;

  SC3A_IS (sc3_allocator_is_setup, a);

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
process_io_error (sc3_log_t * log, sc3_error_t ** ioe, int *num_io)
{
  sc3_error_kind_t    kind;

  SC3A_IS (sc3_log_is_setup, log);
  SC3A_CHECK (ioe != NULL && *ioe != NULL);
  SC3A_CHECK (num_io != NULL && *num_io >= 0);

  /* A fatal error would occur due to bugs or miscalling of I/O routines,
     not due to I/O errors such as file not found, disk full, etc. */
  if (sc3_error_is_fatal (*ioe, NULL)) {
    return sc3_error_new_stack (ioe, __FILE__, __LINE__,
                                "Fatal error during I/O");
  }

  /* Now we only expect an I/O error */
  SC3E (sc3_error_get_kind (*ioe, &kind));
  SC3A_CHECK (kind == SC3_ERROR_IO);
  ++*num_io;

  /* print I/O error stack */
  sc3_log_error (log, 0, SC3_LOG_THREAD0, SC3_LOG_ERROR, *ioe);
  SC3E (sc3_error_destroy (ioe));

  return NULL;
}

static sc3_error_t *
run_prog (sc3_trace_t * t, sc3_allocator_t * origa, sc3_log_t * log,
          int input, int *result, int *num_io)
{
  sc3_trace_t         stacktrace;
  sc3_error_t        *e;
  sc3_allocator_t    *a;

  /* Push call trace and indent log message */
  sc3_trace_push (&t, &stacktrace, 1, "run_prog", NULL);
  sc3_log (log, t->idepth, SC3_LOG_THREAD0, SC3_LOG_TOP, "In run_prog");

  SC3A_IS (sc3_allocator_is_setup, origa);
  SC3A_IS (sc3_log_is_setup, log);
  SC3A_CHECK (result != NULL);
  SC3A_CHECK (num_io != NULL);

  /* Test assertions */
  if (provoke_fatal) {
    SC3E (parent_function (input, result));
  }

  /* Make allocator for this context block */
  SC3E (sc3_allocator_new (origa, &a));
  SC3E (sc3_allocator_setup (a));

  /* Test file input/output and recoverable errors */
  if ((e = run_io (a, *result)) != NULL) {
    SC3E (process_io_error (log, &e, num_io));
  }

  SC3E (sc3_allocator_destroy (&a));
  return NULL;
}

static sc3_error_t *
make_log (sc3_trace_t * t, sc3_allocator_t * ator, sc3_log_t ** plog)
{
  sc3_trace_t         stacktrace;
  sc3_trace_push (&t, &stacktrace, 1, "make_log", NULL);

  SC3E (sc3_log_new (ator, plog));
  SC3E (sc3_log_set_level (*plog, SC3_LOG_INFO));
  SC3E (sc3_log_set_comm (*plog, SC3_MPI_COMM_WORLD));
  SC3E (sc3_log_set_indent (*plog, 3));
  SC3E (sc3_log_setup (*plog));

  return NULL;
}

static sc3_error_t *
test_alloc (sc3_trace_t * t, sc3_allocator_t * ator)
{
  int                 i, j, k;
  char               *abc, *def, *ghi;
  void               *p;
  sc3_allocator_t    *aligned;
  sc3_array_t        *arr;
  const char         *arraytest = "Array test";
  sc3_trace_t         stacktrace;
  sc3_trace_push (&t, &stacktrace, 1, "test_alloc", NULL);

  SC3A_IS (sc3_allocator_is_setup, ator);
  SC3E (sc3_allocator_strdup (ator, "abc", &abc));

  for (i = 0; i < 3; ++i) {
    SC3E (sc3_allocator_new (ator, &aligned));
    SC3E (sc3_allocator_set_align (aligned, i * 8));
    SC3E_DEMIS (sc3_allocator_is_new, aligned);
    SC3E (sc3_allocator_setup (aligned));
    SC3E_DEMIS (!sc3_allocator_is_new, aligned);
    SC3E_DEMIS (sc3_allocator_is_setup, aligned);

    SC3E (sc3_allocator_calloc_one (aligned, SC3_BUFSIZE, &def));
    SC3_BUFCOPY (def, "def");
    SC3E (sc3_allocator_realloc (aligned, strlen (def) + 1, &def));
    SC3E_DEMAND (!memcmp (def, "def", strlen (def)), "String comparison 1");

    SC3E (sc3_allocator_malloc (aligned, 0, &ghi));
    SC3E (sc3_allocator_realloc (aligned, strlen (def) + 1, &ghi));
    snprintf (ghi, 4, "%s", def);
    SC3E_DEMAND (!memcmp (ghi, def, strlen (ghi)), "String comparison 2");

    SC3E (sc3_allocator_free (aligned, def));
    SC3E (sc3_allocator_free (aligned, ghi));

    for (j = 0; j < 3; ++j) {
      SC3E (sc3_array_new (aligned, &arr));
      SC3E (sc3_array_set_elem_size (arr, j * 173));
      SC3E (sc3_array_set_resizable (arr, 1));
      SC3E (sc3_array_set_tighten (arr, 1));
      SC3E_DEMIS (sc3_array_is_new, arr);
      SC3E (sc3_array_setup (arr));
      SC3E_DEMIS (!sc3_array_is_new, arr);
      SC3E_DEMIS (sc3_array_is_setup, arr);

      SC3E (sc3_array_resize (arr, 5329));
      for (k = 0; k < 148; ++k) {
        SC3E (sc3_array_index (arr, k, &p));
        if (j > 0) {
          memcpy (p, arraytest, strlen (arraytest) + 1);
        }
      }
      SC3E (sc3_array_resize (arr, (j + 1) % 3));

      SC3E (sc3_array_destroy (&arr));
    }
    SC3E (sc3_allocator_destroy (&aligned));
  }

  SC3E (sc3_allocator_free (ator, abc));
  return NULL;
}

static sc3_error_t *
test_mpi (sc3_allocator_t * alloc,
          sc3_trace_t * t, sc3_log_t * log, int *rank)
{
  sc3_MPI_Comm_t      mpicomm = SC3_MPI_COMM_WORLD;
  sc3_MPI_Comm_t      sharedcomm, headcomm;
  sc3_MPI_Win_t       sharedwin;
  sc3_MPI_Aint_t      bytesize, querysize;
  int                 disp_unit;
  int                 size, sharedsize, sharedrank, headsize, headrank;
  int                *sharedptr, *queryptr, *headptr;
  int                 p;
  sc3_trace_t         stacktrace;

  SC3A_IS (sc3_allocator_is_setup, alloc);

  /* Push call trace and indent log message */
  sc3_trace_push (&t, &stacktrace, 1, "test_mpi", NULL);
  sc3_log (log, t->idepth, SC3_LOG_THREAD0, SC3_LOG_TOP, "In test_mpi");

  SC3E (sc3_MPI_Comm_set_errhandler (mpicomm, SC3_MPI_ERRORS_RETURN));

  SC3E (sc3_MPI_Comm_size (mpicomm, &size));
  SC3E (sc3_MPI_Comm_rank (mpicomm, rank));

  SC3E_DEMAND (0 <= *rank && *rank < size, "Rank out of range");
  sc3_logf (log, t->idepth, SC3_LOG_THREAD0, SC3_LOG_INFO,
            "MPI size %d rank %d", size, *rank);

  /* create intra-node communicator */
  SC3E (sc3_MPI_Comm_split_type (mpicomm, SC3_MPI_COMM_TYPE_SHARED,
                                 0, SC3_MPI_INFO_NULL, &sharedcomm));
  SC3E (sc3_MPI_Comm_size (sharedcomm, &sharedsize));
  SC3E (sc3_MPI_Comm_rank (sharedcomm, &sharedrank));
  sc3_logf (log, t->idepth, SC3_LOG_THREAD0, SC3_LOG_INFO,
            "MPI size %d rank %d shared size %d rank %d",
            size, *rank, sharedsize, sharedrank);

  /* allocate shared memory */
  bytesize = sharedrank == 0 ? sizeof (int) : 0;
  SC3E (sc3_MPI_Win_allocate_shared (bytesize, 1, SC3_MPI_INFO_NULL,
                                     sharedcomm, &sharedptr, &sharedwin));
  if (sharedrank == 0) {
    SC3E (sc3_MPI_Win_lock (SC3_MPI_LOCK_EXCLUSIVE, 0,
                            SC3_MPI_MODE_NOCHECK, sharedwin));
    sharedptr[0] = 1;
  }
  for (p = 0; p < sharedsize; ++p) {
    SC3E (sc3_MPI_Win_shared_query (sharedwin, p,
                                    &querysize, &disp_unit, &queryptr));
    SC3E_DEMAND (querysize == (sc3_MPI_Aint_t) (p == 0 ? sizeof (int) : 0),
                 "Remote size mismatch");
    SC3E_DEMAND (disp_unit == 1, "Disp unit mismatch");
    if (p == sharedrank) {
      SC3E_DEMAND (queryptr == sharedptr, "Shared pointer mismatch");
      if (sharedrank == 0) {
        SC3E_DEMAND (queryptr[0] == sharedptr[0], "Shared content mismatch");
      }
    }
  }

  /* create communicator with the first rank on each node */
  SC3E (sc3_MPI_Comm_split (mpicomm, sharedrank == 0 ? 0 :
                            SC3_MPI_UNDEFINED, 0, &headcomm));
  SC3A_CHECK ((sharedrank != 0) == (headcomm == SC3_MPI_COMM_NULL));
  if (headcomm != SC3_MPI_COMM_NULL) {
    SC3E (sc3_MPI_Comm_size (headcomm, &headsize));
    SC3E (sc3_MPI_Comm_rank (headcomm, &headrank));
    sc3_logf (log, t->idepth, SC3_LOG_THREAD0, SC3_LOG_INFO,
              "MPI size %d rank %d "
              "shared size %d rank %d head size %d rank %d",
              size, *rank, sharedsize, sharedrank, headsize, headrank);

    SC3E (sc3_allocator_malloc (alloc, headsize * sizeof (int), &headptr));
    sharedptr[0] = headrank;
    SC3E (sc3_MPI_Allgather (sharedptr, 1, SC3_MPI_INT,
                             headptr, 1, SC3_MPI_INT, headcomm));
    for (p = 0; p < headsize; ++p) {
      SC3E_DEMAND (headptr[p] == p, "Head rank mismatch");
    }
    SC3E (sc3_allocator_free (alloc, headptr));
    SC3E (sc3_MPI_Comm_free (&headcomm));
    sc3_logf (log, t->idepth, SC3_LOG_THREAD0, SC3_LOG_INFO,
              "Head comm rank %d ok", headrank);
    SC3E (sc3_MPI_Win_unlock (0, sharedwin));
  }

  /* clean up user communicators */
  SC3E (sc3_MPI_Win_free (&sharedwin));
  SC3E (sc3_MPI_Comm_free (&sharedcomm));
  SC3E (sc3_MPI_Barrier (mpicomm));

  return NULL;
}

static sc3_error_t *
omp_work (sc3_allocator_t * talloc)
{
  SC3A_IS (sc3_allocator_is_setup, talloc);
#if 0
  /* TODO: write some decent OpenMP snippet */
  SC3A_CHECK (sc3_omp_thread_num () % 3 == 1);
#endif

  return NULL;
}

static sc3_error_t *
omp_info (sc3_allocator_t * origa)
{
#pragma GCC diagnostic ignored "-Wunknown-pragmas"

  int                 tmax = sc3_omp_max_threads ();
  int                 minid, maxid, tcount;
  sc3_omp_esync_t     esync, *s = &esync;
  sc3_omp_esync_t     esync2, *s2 = &esync2;
  sc3_error_t        *ompe, *e;

  SC3A_IS (sc3_allocator_is_setup, origa);

  printf ("Max threads %d\n", tmax);

  /* Test 1 -- thread counting */
  minid = tmax;
  maxid = -1;
  tcount = 0;
#pragma omp parallel reduction (min: minid) \
                     reduction (max: maxid) \
                     reduction (+: tcount)
  {
    int                 tnum = sc3_omp_num_threads ();
    int                 tid = sc3_omp_thread_num ();

    printf ("Thread %d out of %d\n", tid, tnum);

    minid = SC3_MIN (minid, tid);
    maxid = SC3_MAX (maxid, tid);
    ++tcount;
  }
  printf ("Reductions min %d max %d count %d\n", minid, maxid, tcount);
  SC3E_DEMAND (0 <= minid && minid <= maxid && maxid < tmax,
               "Thread ids out of range");
  SC3E_DEMAND (maxid < tcount && tcount <= tmax, "Thread ids inconsistent");

  /* Test 2 -- per-thread memory allocation */
  SC3E (sc3_omp_esync_init (s));
  SC3E (sc3_omp_esync_init (s2));
#pragma omp parallel
  {
    sc3_allocator_t    *talloc;
    sc3_error_t        *e = NULL;

    /* initialize thread and per-thread allocator */
#pragma omp critical
    {
      SC3E_SET (e, sc3_allocator_new (origa, &talloc));
      SC3E_NULL_SET (e, sc3_allocator_setup (talloc));
      sc3_omp_esync_in_critical (s, &e);
    }
#pragma omp barrier
    /* now the error status is synchronized between threads */

    if (sc3_omp_esync_is_clean (s)) {
      /* do parallel work in all threads */
      SC3E_SET (e, omp_work (talloc));
    }
#pragma omp barrier
    /* barrier is needed in order not to disturb is_clean above */
    sc3_omp_esync (s, &e);
    /* we require a barrier for s, which is implicit at end of parallel */

    /* clean up thread */
#pragma omp critical
    {
      /* we must unref, not destroy the thread allocator
         since it may have been used to create error objects */
      SC3E_SET (e, sc3_allocator_unref (&talloc));
      sc3_omp_esync_in_critical (s2, &e);
    }
    /* we require a barrier for s2, which is implicit at end of parallel */
  }

  /* create an esync summary error return value */
  ompe = NULL;
  printf ("Threads 1 weird %d error %d count\n", s->rcount, s->ecount);
  e = sc3_omp_esync_summary (s);
  SC3E (sc3_error_accumulate (origa, &ompe, &e, __FILE__, __LINE__, "s1"));
  printf ("Threads 2 weird %d error %d count\n", s2->rcount, s2->ecount);
  e = sc3_omp_esync_summary (s2);
  SC3E (sc3_error_accumulate (origa, &ompe, &e, __FILE__, __LINE__, "s2"));
  return ompe;
}

static sc3_error_t *
run_main (sc3_trace_t * t, int *argc, char ***argv)
{
  const int           inputs[3] = { 167, 84, 23 };
  int                 mpirank;
  int                 i;
  int                 input;
  int                 result;
  int                 num_io;
  sc3_allocator_t    *mainalloc, *a;
  sc3_log_t          *mainlog;
  sc3_trace_t         stacktrace;

  sc3_trace_push (&t, &stacktrace, 1, "run_main", NULL);

  SC3E (sc3_MPI_Init (argc, argv));

  mainalloc = sc3_allocator_nothread ();
  SC3E (sc3_allocator_new (mainalloc, &a));
  SC3E (sc3_allocator_setup (a));

  SC3E (make_log (t, a, &mainlog));
  sc3_logf (mainlog, t->idepth, SC3_LOG_PROCESS0, SC3_LOG_TOP,
            "Main run is %s", "here");

  SC3E (test_alloc (t, a));
  sc3_log (mainlog, t->idepth, SC3_LOG_THREAD0, SC3_LOG_TOP, "Alloc test ok");

  SC3E (test_mpi (a, t, mainlog, &mpirank));
  sc3_log (mainlog, t->idepth, SC3_LOG_THREAD0, SC3_LOG_TOP, "MPI code ok");

  SC3E (omp_info (a));
  sc3_log (mainlog, t->idepth, SC3_LOG_THREAD0, SC3_LOG_TOP,
           "OpenMP code ok");

  num_io = 0;
  for (i = 0; i < 3; ++i) {
    result = input = inputs[i];
    SC3E (run_prog (t, a, mainlog, input, &result, &num_io));
    sc3_logf (mainlog, t->idepth, SC3_LOG_THREAD0, SC3_LOG_TOP,
              "Clean execution with input %d result %d io %d",
              input, result, num_io);
  }

  sc3_logf (mainlog, t->idepth, SC3_LOG_PROCESS0, SC3_LOG_TOP,
            "Main run is %s", "done");
  SC3E (sc3_log_destroy (&mainlog));

  SC3E (sc3_allocator_destroy (&a));
  SC3A_IS (sc3_allocator_is_free, mainalloc);

  SC3E (sc3_MPI_Finalize ());

  return NULL;
}

int
main (int argc, char **argv)
{
  sc3_trace_t         stacktrace, *t = &stacktrace;
  sc3_error_t        *rune;

  sc3_trace_init (t, "main", NULL);

  if (argc >= 2) {
    if (strchr (argv[1], 'F')) {
      provoke_fatal = 1;
    }
  }

  if ((rune = run_main (t, &argc, &argv)) != NULL) {
    sc3_log_error (sc3_log_predef (), 0,
                   SC3_LOG_THREAD0, SC3_LOG_ERROR, rune);
    sc3_error_destroy (&rune);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
