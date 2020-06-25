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

#include <sc_allgather.h>
#include <sc_sort.h>

int
main (int argc, char **argv)
{

  int                 mpiret;
  int                 rank, num_procs;
  int                 i, isizet;
  int                 k, printed;
  int                *recvc, *displ;
  int                 timing;
  size_t              zz;
  size_t              lcount, gtotal;
  size_t             *nmemb;
  double             *ldata, *gdata;
  sc_MPI_Comm         mpicomm;
  char                buffer[BUFSIZ];

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpicomm = sc_MPI_COMM_WORLD;
  mpiret = sc_MPI_Comm_size (mpicomm, &num_procs);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &rank);
  SC_CHECK_MPI (mpiret);

  sc_init (mpicomm, 1, 1, NULL, SC_LP_DEFAULT);

  /*
     Positive integer first argument: print nicely if global count < 1000
     Other first argument: random local count between 8 and 23, print < 1000
     No argument: random local counts between 8 and 23, no print/delay
   */
  srand ((unsigned) rank << 15);
  timing = 1;                   /* meaning we don't want to delay the program */
  lcount = 8 + (size_t) (16. * rand () / (RAND_MAX + 1.0));
  if (argc >= 2) {
    const long          stol = strtol (argv[1], NULL, 0);
    if (stol > 0) {
      /* overwrite local counts and activate timing and print the numbers */
      lcount = (size_t) stol;
    }
    timing = 0;                 /* effectively enable staggered output */
  }

  /* create partition information */
  SC_INFOF ("Local values %ld\n", (long) lcount);
  nmemb = SC_ALLOC (size_t, num_procs);
  isizet = (int) sizeof (size_t);
  mpiret = sc_MPI_Allgather (&lcount, isizet, sc_MPI_BYTE,
                             nmemb, isizet, sc_MPI_BYTE, mpicomm);
  SC_CHECK_MPI (mpiret);
  gtotal = 0;
  for (i = 0; i < num_procs; ++i) {
    gtotal += nmemb[i];
  }

  /* create data and sort it */
  ldata = SC_ALLOC (double, lcount);
  for (zz = 0; zz < lcount; ++zz) {
    ldata[zz] = -50. + (100. * rand () / (RAND_MAX + 1.0));
  }

  /* output result before sort */
  if (!timing && gtotal < 1000) {
    SC_GLOBAL_PRODUCTION ("Values before sort\n");
    sleep ((unsigned) rank);
    for (zz = 0; zz < lcount;) {
      printed = 0;
      for (k = 0; zz < lcount && k < 8; ++zz, ++k) {
        printed += snprintf (buffer + printed, BUFSIZ - printed,
                             "%8.4g", ldata[zz]);
      }
      SC_STATISTICSF ("%s\n", buffer);
    }
    sc_MPI_Barrier (mpicomm);
    sleep (1);
  }

  /* call parallel sort */
  SC_GLOBAL_PRODUCTIONF ("Sorting %ld\n", (long) gtotal);
  sc_psort (mpicomm, ldata, nmemb, sizeof (double), sc_double_compare);

  /* output result */
  if (!timing && gtotal < 1000) {
    sc_MPI_Barrier (mpicomm);
    SC_GLOBAL_PRODUCTION ("Values after sort\n");
    sleep ((unsigned) rank);
    for (zz = 0; zz < lcount;) {
      printed = 0;
      for (k = 0; zz < lcount && k < 8; ++zz, ++k) {
        printed += snprintf (buffer + printed, BUFSIZ - printed,
                             "%8.4g", ldata[zz]);
      }
      SC_STATISTICSF ("%s\n", buffer);
    }
    sc_MPI_Barrier (mpicomm);
    sleep (1);
  }

  /* verify result always, if the numbers are not too many */
  if (gtotal < 100000) {
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
    mpiret = sc_MPI_Gatherv (ldata, (int) lcount, sc_MPI_DOUBLE,
                             gdata, recvc, displ, sc_MPI_DOUBLE, 0, mpicomm);
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

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
