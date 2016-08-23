/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System
  Additional copyright (C) 2011 individual authors

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

typedef struct global_data global_data_t;

typedef struct thread_data
{
  int                 id;
  int                 working, done;
  pthread_t           thread;
  global_data_t      *gd;
}
thread_data_t;

struct global_data
{
  int                 N, T;
  int                 setup;
  int                 scheduled, started;
  pthread_mutex_t     mutex;
  pthread_cond_t      cond_setup;
  pthread_cond_t      cond_start;
  pthread_cond_t      cond_stop;
  pthread_attr_t      attr;
  thread_data_t      *td;
};

static void        *
start_thread (void *v)
{
  thread_data_t      *td = (thread_data_t *) v;
  global_data_t      *g = td->gd;
  int                 j;

  /* setup phase do work here depending on td->i */
  SC_INFOF ("T%02d setup working\n", td->id);

  /* setup phase end: increment global state */
  pthread_mutex_lock (&g->mutex);
  ++g->setup;
  pthread_mutex_unlock (&g->mutex);
  pthread_cond_signal (&g->cond_setup);

  SC_INFOF ("T%02d setup done\n", td->id);

  for (j = 0;; ++j) {

    SC_INFOF ("T%02d task waiting\n", td->id);

    /* task phase begin: wait for start or exit signal */
    pthread_mutex_lock (&g->mutex);
    while (g->scheduled == g->started) {
      SC_INFOF ("T%02d task into cond_wait with %d\n", td->id, g->scheduled);
      pthread_cond_wait (&g->cond_start, &g->mutex);
    }
    SC_INFOF ("T%02d task skip cond_wait\n", td->id);
    if (g->scheduled == -1) {
      SC_INFOF ("T%02d task to exit\n", td->id);
      pthread_mutex_unlock (&g->mutex);
      pthread_exit (NULL);
    }

    /* set this task to work */
    SC_ASSERT (0 <= g->started);
    SC_ASSERT (g->started < g->scheduled);
    SC_ASSERT (g->scheduled <= g->N);
    SC_INFOF ("T%02d task now is id %d\n", td->id, g->started);

    /* this thread picks the next scheduled id */
    td = &g->td[g->started++];
    SC_ASSERT (td->working == 0);
    SC_ASSERT (td->done == 0);
    td->working = 1;
    pthread_mutex_unlock (&g->mutex);

    /* task phase do work here depending on td->i */
    SC_INFOF ("T%02d task working\n", td->id);

    /* signal that the work is done */
    pthread_mutex_lock (&g->mutex);
    SC_ASSERT (td->done == 0);
    SC_ASSERT (td->working == 1);
    td->done = 1;
    td->working = 0;
    pthread_mutex_unlock (&g->mutex);
    pthread_cond_signal (&g->cond_stop);

    SC_INFOF ("T%02d task done\n", td->id);
  }

  pthread_exit (v);
}

static void
condvar_setup (global_data_t * g)
{
  int                 i;
  int                 pth;
  thread_data_t      *td;

  /*
   * The main thread starts worker threads.
   * The worker threads do their setup work in undefined order.
   * The main thread waits until all of them are are done with their setup.
   * The threads go to sleep and the main thread does stuff for a while.
   * The main thread then gives each worker something to do in order.
   * The main thread waits for the workers to finish in reverse order.
   */

  SC_INFO ("Main setup begin\n");

  /* start threads */
  g->setup = 0;
  g->scheduled = g->started = 0;
  pthread_mutex_init (&g->mutex, NULL);
  pthread_cond_init (&g->cond_setup, NULL);
  pthread_cond_init (&g->cond_start, NULL);
  pthread_cond_init (&g->cond_stop, NULL);
  pthread_attr_init (&g->attr);
  pthread_attr_setdetachstate (&g->attr, PTHREAD_CREATE_JOINABLE);
  g->td = SC_ALLOC (thread_data_t, g->N);
  for (i = 0; i < g->N; ++i) {
    td = &g->td[i];
    td->id = i;
    td->gd = g;
    td->working = td->done = 0;
    pth = pthread_create (&td->thread, &g->attr, &start_thread, td);
    SC_CHECK_ABORTF (pth == 0, "pthread_create error %d", pth);
  }

  SC_INFO ("Main setup waiting\n");

  /* wait until the threads have done their setup */
  pthread_mutex_lock (&g->mutex);
  while (g->setup < g->N) {
    pth = pthread_cond_wait (&g->cond_setup, &g->mutex);
    SC_CHECK_ABORT (pth == 0, "pthread_cond_wait");
  }
  pthread_mutex_unlock (&g->mutex);

  SC_INFO ("Main setup done\n");
}

static void
condvar_work (global_data_t * g)
{
  int                 i, j;
  thread_data_t      *td;

  for (j = 0; j < g->T; ++j) {

    /* main thread does some stuff */
    SC_INFOF ("Main cycle %d start\n", j);
    pthread_mutex_lock (&g->mutex);
    SC_ASSERT (g->scheduled == 0 && g->started == 0);
    pthread_mutex_unlock (&g->mutex);

    for (i = 0; i < g->N; ++i) {
      /* signal for some task to start working */
      pthread_mutex_lock (&g->mutex);
      SC_ASSERT (g->scheduled == i);
      ++g->scheduled;
      pthread_mutex_unlock (&g->mutex);
      pthread_cond_signal (&g->cond_start);

      /* main thread does some stuff */
    }

    SC_INFOF ("Main cycle %d waiting\n", j);
    /* main thread does some stuff */

    for (i = g->N - 1; i >= 0; --i) {
      td = &g->td[i];

      /* main thread waits for tasks to end in reverse order */
      pthread_mutex_lock (&g->mutex);
      while (td->done != 1) {
        pthread_cond_wait (&g->cond_stop, &g->mutex);
      }
      SC_ASSERT (td->working == 0);
      td->done = 0;
      pthread_mutex_unlock (&g->mutex);

      /* main thread does some stuff */
    }

    /* main thread does some stuff */
    SC_INFOF ("Main cycle %d stop\n", j);

    /* reset thread schedule after task is processed */
    pthread_mutex_lock (&g->mutex);
    SC_ASSERT (g->scheduled == g->N && g->started == g->N);
    g->scheduled = g->started = 0;
    pthread_mutex_unlock (&g->mutex);
  }
}

static void
condvar_teardown (global_data_t * g)
{
  int                 i;
  int                 pth;
  void               *exitval;
  thread_data_t      *td;

  SC_INFO ("Main teardown begin\n");

  /* signal all threads to exit */
  pthread_mutex_lock (&g->mutex);
  g->scheduled = -1;
  pthread_mutex_unlock (&g->mutex);
  pth = pthread_cond_broadcast (&g->cond_start);
  SC_CHECK_ABORT (pth == 0, "pthread_cond_broadcast");

  SC_INFO ("Main teardown join\n");

  /* wait for all threads to terminate */
  for (i = 0; i < g->N; ++i) {
    td = &g->td[i];
    SC_INFOF ("Main teardown join %02d\n", i);
    pth = pthread_join (td->thread, &exitval);
    SC_CHECK_ABORT (pth == 0, "pthread_join");
    SC_INFOF ("Main teardown done %02d\n", i);
    SC_ASSERT (exitval == NULL);
    SC_ASSERT (td->working == 0);
    SC_ASSERT (td->done == 0);
  }

  SC_INFO ("Main teardown done\n");

  /* cleanup storage */
  pthread_attr_destroy (&g->attr);
  pthread_cond_destroy (&g->cond_stop);
  pthread_cond_destroy (&g->cond_start);
  pthread_cond_destroy (&g->cond_setup);
  pthread_mutex_destroy (&g->mutex);
  SC_FREE (g->td);
}

static void
condvar_run (global_data_t * g)
{
  condvar_setup (g);
  condvar_work (g);
  condvar_teardown (g);
}

int
main (int argc, char **argv)
{
  int                 mpiret;
  int                 mpithr;
  int                 first_arg;
  int                 N, T;
  sc_options_t       *opt;
  global_data_t       sg, *g = &sg;

  mpiret = sc_MPI_Init_thread (&argc, &argv, sc_MPI_THREAD_MULTIPLE, &mpithr);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

  opt = sc_options_new (argv[0]);
  sc_options_add_int (opt, 'N', "num-threads", &N, 0, "Number of threads");
  sc_options_add_int (opt, 'T', "num-tasks", &T, 0, "Number of tasks");

  first_arg = sc_options_parse (sc_package_id, SC_LP_ERROR, opt, argc, argv);
  if (first_arg != argc || N < 0 || T < 0) {
    sc_options_print_usage (sc_package_id, SC_LP_ERROR, opt, NULL);
    sc_abort_collective ("Option parsing failed");
  }
  else {
    sc_options_print_summary (sc_package_id, SC_LP_PRODUCTION, opt);
  }

  g->N = N;
  g->T = T;
  condvar_run (g);

  sc_options_destroy (opt);
  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
