#include <sc.h>
#include <omp.h>

omp_lock_t          writelock;

void
openmp_print_tid (void)
{
  omp_set_lock (&writelock);
  SC_PRODUCTIONF ("Hello from thread %i.\n", omp_get_thread_num ());
  omp_unset_lock (&writelock);
}

int
main (int argc, char *argv[])
{
  int                 mpiret, mpisize;
  int                 thread_lvl, num_threads;

  mpiret =
    sc_MPI_Init_thread (&argc, &argv, sc_MPI_THREAD_MULTIPLE, &thread_lvl);
  SC_CHECK_MPI (mpiret);
  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

  if (thread_lvl < sc_MPI_THREAD_MULTIPLE) {
    SC_GLOBAL_PRODUCTIONF ("Mpi only supports thread level %d\n", thread_lvl);
  }
  else {
    mpiret = sc_MPI_Comm_size (sc_MPI_COMM_WORLD, &mpisize);
    SC_CHECK_MPI (mpiret);
    num_threads = omp_get_max_threads ();
    SC_GLOBAL_PRODUCTIONF ("Running on %i processes with %i threads each.\n",
                           mpisize, num_threads);
    omp_set_num_threads (num_threads);
    omp_init_lock (&writelock);
#pragma omp parallel
    {
      openmp_print_tid ();
    }
  }
  return 0;
}
