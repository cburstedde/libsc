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

#include <pthread.h>
#include <sc_options.h>

typedef struct thread_data
{
  pthread_t           thread;
  int                 id;
  MPI_Comm            mpicomm;
}
thread_data_t;

static void        *
start_thread (void *v)
{
  thread_data_t      *td = (thread_data_t *) v;
  int                 j;
  int                *p;
  int                 mpiret;
  int                 mpisize;

  /* randomize thread startup time */
  sleep (4. * rand () / (RAND_MAX + 1.));
  SC_INFOF ("This is thread %d\n", td->id);

  /* create some data */
  p = SC_ALLOC (int, 1000);
  for (j = 0; j < 1000; ++j) {
    p[j] = j + 17 * td->id;
  }

  /* duplicate communicator and execute a collective MPI call */
  mpiret = sc_MPI_Comm_size (td->mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Allreduce (p, p + 500, 500, sc_MPI_INT, sc_MPI_SUM,
                             td->mpicomm);
  SC_CHECK_MPI (mpiret);

  /* check the results and free data */
  for (j = 500; j < 1000; ++j) {
    SC_CHECK_ABORT (p[j] == (j - 500 + 17 * td->id) * mpisize,
                    "Communication mismatch");
  }
  SC_FREE (p);

  /* automatically calls pthread_exit (v) */
  return v;
}

static void
test_threads (int N)
{
  int                 mpiret;
  int                 i;
  int                 pth;
  void               *exitval;
  pthread_attr_t      attr;
  thread_data_t      *td;

  /* allocate thread data */
  td = SC_ALLOC (thread_data_t, N);

  /* create and run threads */
  pth = pthread_attr_init (&attr);
  SC_CHECK_ABORT (pth == 0, "Fail in pthread_attr_init");
  pth = pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
  SC_CHECK_ABORT (pth == 0, "Fail in pthread_attr_setdetachstate");
  for (i = 0; i < N; ++i) {
    mpiret = sc_MPI_Comm_dup (sc_MPI_COMM_WORLD, &td[i].mpicomm);
    SC_CHECK_MPI (mpiret);
    td[i].id = i;
    pth = pthread_create (&td[i].thread, &attr, &start_thread, &td[i]);
    SC_CHECK_ABORT (pth == 0, "Fail in pthread_create");
  }

  /* wait for threads to finish */
  for (i = 0; i < N; ++i) {
    pth = pthread_join (td[i].thread, &exitval);
    SC_CHECK_ABORT (pth == 0, "Fail in pthread_join");
    SC_ASSERT (exitval == &td[i]);
    mpiret = sc_MPI_Comm_free (&td[i].mpicomm);
    SC_CHECK_MPI (mpiret);
  }

  /* destroy attribute and thread data */
  pth = pthread_attr_destroy (&attr);
  SC_CHECK_ABORT (pth == 0, "Fail in pthread_attr_destroy");
  SC_FREE (td);
}

int
main (int argc, char **argv)
{
  int                 mpiret;
  int                 mpithr;
  int                 first_arg;
  int                 N;
  sc_options_t       *opt;

  mpiret = sc_MPI_Init_thread (&argc, &argv, sc_MPI_THREAD_MULTIPLE, &mpithr);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

  opt = sc_options_new (argv[0]);
  sc_options_add_int (opt, 'N', "num-threads", &N, 0, "Number of threads");

  first_arg = sc_options_parse (sc_package_id, SC_LP_ERROR, opt, argc, argv);
  if (first_arg != argc || N < 0) {
    sc_options_print_usage (sc_package_id, SC_LP_ERROR, opt, NULL);
    sc_abort_collective ("Option parsing failed");
  }
  else {
    sc_options_print_summary (sc_package_id, SC_LP_PRODUCTION, opt);
  }

  if (mpithr < sc_MPI_THREAD_MULTIPLE) {
    SC_GLOBAL_PRODUCTIONF ("MPI thread support is only %d\n", mpithr);
  }
  else {
    test_threads (N);
  }

  sc_options_destroy (opt);
  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
