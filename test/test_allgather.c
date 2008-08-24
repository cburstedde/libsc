/*
 * Copyright (c) 2008 Carsten Burstedde <carsten@ices.utexas.edu>
 *
 * May only be used with the Mangll, Rhea and P4est codes.
 * Any other use is prohibited.
 */

#include <sc.h>
#include <sc_allgather.h>

#ifdef SC_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef SC_HAVE_TIME_H
#include <time.h>
#endif

int
main (int argc, char **argv)
{
  MPI_Comm            mpicomm;
  int                 mpiret;
  int                 mpisize;
  int                 mpirank;
  int                 i;
#ifdef SC_MPI
  int                *idata;
  double              elapsed_alltoall = 0.;
  double              elapsed_recursive;
#endif
  double              dsend;
  double             *ddata1;
  double             *ddata2;
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

#ifdef SC_MPI
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

  SC_GLOBAL_INFO ("Testing sc_ag_recursive\n");

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
#endif

  ddata1 = SC_ALLOC (double, mpisize);
  ddata2 = SC_ALLOC (double, mpisize);

  SC_GLOBAL_INFO ("Testing allgather and replacement\n");

  gettimeofday (&tv, NULL);
  srand48 (tv.tv_usec + 1e6 * getpid ());
  dsend = drand48 ();

  mpiret = MPI_Barrier (mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_allgather = -MPI_Wtime ();
  mpiret = MPI_Allgather (&dsend, 1, MPI_DOUBLE, ddata1, 1, MPI_DOUBLE,
                          mpicomm);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Barrier (mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_allgather += MPI_Wtime ();

  mpiret = MPI_Barrier (mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_replacement = -MPI_Wtime ();
  mpiret = sc_allgather (&dsend, 1, MPI_DOUBLE, ddata2, 1, MPI_DOUBLE,
                         mpicomm);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Barrier (mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_replacement += MPI_Wtime ();

  for (i = 0; i < mpisize; ++i) {
    SC_ASSERT (ddata1[i] == ddata2[i]); /* exact match wanted */
  }
  SC_ASSERT (ddata1[mpirank] == dsend); /* exact match wanted */

  SC_FREE (ddata1);
  SC_FREE (ddata2);

  SC_GLOBAL_STATISTICSF ("Timings with threshold %d on %d cores\n",
                         SC_AG_ALLTOALL_MAX, mpisize);
#ifdef SC_MPI
  SC_GLOBAL_STATISTICSF ("   alltoall %g\n", elapsed_alltoall);
  SC_GLOBAL_STATISTICSF ("   recursive %g\n", elapsed_recursive);
#endif
  SC_GLOBAL_STATISTICSF ("   allgather %g\n", elapsed_allgather);
  SC_GLOBAL_STATISTICSF ("   replacement %g\n", elapsed_replacement);

  sc_finalize ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}

/* EOF test_allgather.c */
