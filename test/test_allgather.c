/*
 * Copyright (c) 2008 Carsten Burstedde <carsten@ices.utexas.edu>
 *
 * May only be used with the Mangll, Rhea and P4est codes.
 * Any other use is prohibited.
 */

#include <sc.h>
#include <sc_allgather.h>

#include <sys/time.h>
#include <time.h>

int
main (int argc, char **argv)
{
  MPI_Comm            mpicomm;
  int                 mpiret;
  int                 mpisize;
  int                 mpirank;
  int                 i;
  int                *idata;
  double             *ddata1;
  double             *ddata2;
  double              dsend;
  double              elapsed_alltoall = 0.;
  double              elapsed_recursive;
  double              elapsed_allgather;
  double              elapsed_replacement;
  struct timeval      tv;

  mpiret = MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpicomm = MPI_COMM_WORLD;
  mpiret = MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  sc_init (mpirank, NULL, NULL, NULL, SC_LP_DEFAULT);

  idata = SC_ALLOC (int, mpisize);

  if (mpisize <= 64) {
    SC_GLOBAL_INFO ("Testing sc_ag_alltoall\n");

    for (i = 0; i < mpisize; ++i) {
      idata[i] = (i == mpirank) ? mpirank : -1;
    }
    elapsed_alltoall = -MPI_Wtime ();
    sc_ag_alltoall (mpicomm, (char *) idata, (int) sizeof (int),
                    mpisize, mpirank, mpirank);
    elapsed_alltoall += MPI_Wtime ();
    for (i = 0; i < mpisize; ++i) {
      SC_ASSERT (idata[i] == i);
    }
  }

  SC_GLOBAL_INFOF ("Testing sc_ag_recursive with threshold %d\n",
                   SC_AG_ALLTOALL_MAX);

  for (i = 0; i < mpisize; ++i) {
    idata[i] = (i == mpirank) ? mpirank : -1;
  }
  elapsed_recursive = -MPI_Wtime ();
  sc_ag_recursive (mpicomm, (char *) idata, (int) sizeof (int),
                   mpisize, mpirank, mpirank);
  elapsed_recursive += MPI_Wtime ();
  for (i = 0; i < mpisize; ++i) {
    SC_ASSERT (idata[i] == i);
  }

  SC_FREE (idata);
  ddata1 = SC_ALLOC (double, mpisize);
  ddata2 = SC_ALLOC (double, mpisize);

  SC_GLOBAL_INFO ("Testing allgather and replacement\n");

  gettimeofday (&tv, NULL);
  srand48 (tv.tv_usec + 1e6 * getpid ());
  dsend = drand48 ();

  elapsed_allgather = -MPI_Wtime ();
  mpiret = MPI_Allgather (&dsend, 1, MPI_DOUBLE, ddata1, 1, MPI_DOUBLE,
                          mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_allgather += MPI_Wtime ();

  elapsed_replacement = -MPI_Wtime ();
  mpiret = sc_allgather (&dsend, 1, MPI_DOUBLE, ddata2, 1, MPI_DOUBLE,
                         mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_replacement += MPI_Wtime ();

  for (i = 0; i < mpisize; ++i) {
    SC_ASSERT (ddata1[i] == ddata2[i]); /* exact match wanted */
  }

  SC_FREE (ddata1);
  SC_FREE (ddata2);

  SC_GLOBAL_STATISTICS ("Timings\n");
  SC_GLOBAL_STATISTICSF ("   alltoall %g\n", elapsed_alltoall);
  SC_GLOBAL_STATISTICSF ("   recursive %g\n", elapsed_recursive);
  SC_GLOBAL_STATISTICSF ("   allgather %g\n", elapsed_allgather);
  SC_GLOBAL_STATISTICSF ("   replacement %g\n", elapsed_replacement);

  sc_finalize ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}

/* EOF test_allgather.c */
