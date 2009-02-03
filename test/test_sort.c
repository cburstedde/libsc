/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2008 Carsten Burstedde, Lucas Wilcox.

  The SC Library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with the SC Library.  If not, see <http://www.gnu.org/licenses/>.
*/

/* sc.h comes first in every compilation unit */
#include <sc.h>
#include <sc_allgather.h>
#include <sc_sort.h>

#ifdef SC_ALLGATHER
#include <sc_allgather.h>
#define MPI_Allgather sc_allgather
#endif

int
main (int argc, char **argv)
{
#ifdef SC_DEBUG
  int                 mpiret;
  int                 rank, num_procs;
  int                 i, isizet;
  int                 k, printed;
  int                *recvc, *displ;
  bool                timing;
  size_t              zz;
  size_t              lcount, gtotal;
  size_t             *nmemb;
  double             *ldata, *gdata;
  MPI_Comm            mpicomm;
  char                buffer[BUFSIZ];

  mpiret = MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpicomm = MPI_COMM_WORLD;
  mpiret = MPI_Comm_size (mpicomm, &num_procs);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Comm_rank (mpicomm, &rank);
  SC_CHECK_MPI (mpiret);

  sc_init (rank, NULL, NULL, NULL, SC_LP_DEFAULT);

  if (argc >= 2) {
    timing = true;
    lcount = (size_t) strtol (argv[1], NULL, 0);
  }
  else {
    timing = false;
    srand ((unsigned) rank << 15);
    lcount = 8 + (size_t) (16. * rand () / (RAND_MAX + 1.0));
  }

  /* create partition information */
  SC_INFOF ("Local values %d\n", (int) lcount);
  nmemb = SC_ALLOC (size_t, num_procs);
  isizet = (int) sizeof (size_t);
  mpiret = MPI_Allgather (&lcount, isizet, MPI_BYTE,
                          nmemb, isizet, MPI_BYTE, mpicomm);
  SC_CHECK_MPI (mpiret);

  /* create data and sort it */
  ldata = SC_ALLOC (double, lcount);
  for (zz = 0; zz < lcount; ++zz) {
    ldata[zz] = -50. + (100. * rand () / (RAND_MAX + 1.0));
  }
  sc_psort (mpicomm, ldata, nmemb, sizeof (double), sc_double_compare);

  /* output result */
  if (!timing) {
    sleep ((unsigned) rank);
    for (zz = 0; zz < lcount;) {
      printed = 0;
      for (k = 0; zz < lcount && k < 8; ++zz, ++k) {
        printed += snprintf (buffer + printed, BUFSIZ - printed,
                             "%8.3g", ldata[zz]);
      }
      SC_STATISTICSF ("%s\n", buffer);
    }
  }

  /* verify result */
  if (!timing || lcount < 1000) {
    SC_GLOBAL_PRODUCTION ("Verifying\n");
    gtotal = 0;
    recvc = NULL;
    displ = NULL;
    gdata = NULL;
    if (rank == 0) {
      recvc = SC_ALLOC (int, num_procs);
      displ = SC_ALLOC (int, num_procs + 1);
      displ[0] = 0;
      for (i = 0; i < num_procs; ++i) {
        recvc[i] = (int) nmemb[i];
        displ[i + 1] = displ[i] + recvc[i];
      }
      gtotal = (size_t) displ[num_procs];
      gdata = SC_ALLOC (double, gtotal);
    }
    mpiret = MPI_Gatherv (ldata, (int) lcount, MPI_DOUBLE,
                          gdata, recvc, displ, MPI_DOUBLE, 0, mpicomm);
    SC_CHECK_MPI (mpiret);
    if (rank == 0) {
      for (zz = 0; zz < gtotal - 1; ++zz) {
        SC_CHECK_ABORT (gdata[zz] <= gdata[zz + 1], "Parallel sort failed");
      }
    }
    SC_FREE (gdata);
    SC_FREE (displ);
    SC_FREE (recvc);
  }

  /* clean up and exit */
  SC_FREE (ldata);
  SC_FREE (nmemb);

  sc_finalize ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);
#endif

  return 0;
}

/* EOF test_sort.c */
