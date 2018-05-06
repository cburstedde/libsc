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

#include <sc_notify.h>
#include <sc_statistics.h>

/** Remove duplicates from an array of non-decreasing non-negative integers */
static void
sc_uniq (int *list, int *count)
{
  int                 incount, skip;
  int                 prev_item;
  int                 i;

  SC_ASSERT (list != NULL);
  SC_ASSERT (count != NULL);

  incount = *count;
  SC_ASSERT (0 <= incount);
  if (incount == 0) {
    return;
  }

  skip = 0;
  prev_item = -1;
  for (i = 0; i < incount; ++i) {
    SC_ASSERT (list[i] >= 0);
    SC_ASSERT (list[i] >= prev_item);

    if (list[i] == prev_item) {
      ++skip;
      continue;
    }
    prev_item = list[i];
    if (skip > 0) {
      SC_ASSERT (i >= skip);
      list[i - skip] = prev_item;
    }
  }

  SC_ASSERT (skip < incount);
  *count = incount - skip;
}

typedef enum sc_test_stats
{
  SC_STAT_NOTIFY_ALLG,
  SC_STAT_NOTIFY_NARY,
  SC_STAT_NOTIFY_PAYL,
  SC_STAT_NOTIFY_NATI,
  SC_STAT_NOTIFY_LAST
}
sc_test_stats_t;

int
main (int argc, char **argv)
{
  int                 i;
  int                 mpiret;
  int                 mpisize, mpirank;
  int                *senders1, num_senders1;
  int                *senders2, num_senders2;
  int                *senders3, num_senders3;
  int                *senders4, num_senders4;
  int                *receivers, num_receivers;
  int                 ntop, nint, nbot;
  double              elapsed_allgather;
  double              elapsed_nary;
  double              elapsed_native;
  double              elapsed_payl;
  sc_MPI_Comm         mpicomm;
  sc_array_t         *rec2, *snd2, *rec4, *pay4;
  sc_statinfo_t       stats[SC_STAT_NOTIFY_LAST];

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpicomm = sc_MPI_COMM_WORLD;
  mpiret = sc_MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  sc_init (mpicomm, 1, 1, NULL, SC_LP_DEFAULT);

  /* grap parameters for notify_nary from command line */
  ntop = nint = nbot = 2;
  if (argc == 4) {
    ntop = atoi (argv[1]);
    nint = atoi (argv[2]);
    nbot = atoi (argv[3]);
    if (ntop < 2) {
      sc_abort_collective ("First argument ntop must be at least 2");
    }
    if (nint < 2) {
      sc_abort_collective ("Second argument nint must be at least 2");
    }
    if (nbot < 2) {
      sc_abort_collective ("Third argument nbot must be at least 2");
    }
  }

  num_receivers = (mpirank * (mpirank % 100)) % 7;
  num_receivers = SC_MIN (num_receivers, mpisize);
  receivers = SC_ALLOC (int, num_receivers);
  for (i = 0; i < num_receivers; ++i) {
    receivers[i] = (3 * mpirank + 17 * i) % mpisize;
  }
  qsort (receivers, num_receivers, sizeof (int), sc_int_compare);
  sc_uniq (receivers, &num_receivers);

  SC_GLOBAL_INFO ("Testing sc_notify_allgather\n");
  senders1 = SC_ALLOC (int, mpisize);
  elapsed_allgather = -sc_MPI_Wtime ();
  mpiret = sc_notify_allgather (receivers, num_receivers,
                                senders1, &num_senders1, mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_allgather += sc_MPI_Wtime ();
  sc_stats_set1 (stats + SC_STAT_NOTIFY_ALLG, elapsed_allgather, "Allgather");

  mpiret = sc_MPI_Barrier (mpicomm);
  SC_CHECK_MPI (mpiret);

  SC_GLOBAL_INFOF ("Testing sc_notify_ext with %d %d %d\n", ntop, nint, nbot);
  rec2 = sc_array_new_data (receivers, sizeof (int), num_receivers);
  snd2 = sc_array_new (sizeof (int));
  elapsed_nary = -sc_MPI_Wtime ();
  sc_notify_ext (rec2, snd2, NULL, ntop, nint, nbot, mpicomm);
  elapsed_nary += sc_MPI_Wtime ();
  senders2 = (int *) snd2->array;
  num_senders2 = (int) snd2->elem_count;
  sc_stats_set1 (stats + SC_STAT_NOTIFY_NARY, elapsed_nary, "Nary");

  mpiret = sc_MPI_Barrier (mpicomm);
  SC_CHECK_MPI (mpiret);

  SC_GLOBAL_INFOF ("Testing sc_notify_ext with %d %d %d and payload\n",
                   ntop, nint, nbot);
  rec4 = sc_array_new_count (sizeof (int), num_receivers);
  pay4 = sc_array_new_count (sizeof (int), num_receivers);
  for (i = 0; i < num_receivers; ++i) {
    *(int *) sc_array_index_int (rec4, i) = receivers[i];
    *(int *) sc_array_index_int (pay4, i) = 2 * mpirank + 3;
  }
  elapsed_payl = -sc_MPI_Wtime ();
  sc_notify_ext (rec4, NULL, pay4, ntop, nint, nbot, mpicomm);
  elapsed_payl += sc_MPI_Wtime ();
  senders4 = (int *) rec4->array;
  num_senders4 = (int) rec4->elem_count;
  SC_ASSERT ((int) pay4->elem_count == num_senders4);
  sc_stats_set1 (stats + SC_STAT_NOTIFY_PAYL, elapsed_payl, "Payload");

  mpiret = sc_MPI_Barrier (mpicomm);
  SC_CHECK_MPI (mpiret);

  SC_GLOBAL_INFO ("Testing native sc_notify\n");
  senders3 = SC_ALLOC (int, mpisize);
  elapsed_native = -sc_MPI_Wtime ();
  mpiret = sc_notify (receivers, num_receivers,
                      senders3, &num_senders3, mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_native += sc_MPI_Wtime ();
  sc_stats_set1 (stats + SC_STAT_NOTIFY_NATI, elapsed_native, "Native");

  SC_CHECK_ABORT (num_senders1 == num_senders2, "Mismatch 12 sender count");
  SC_CHECK_ABORT (num_senders1 == num_senders3, "Mismatch 13 sender count");
  SC_CHECK_ABORT (num_senders1 == num_senders4, "Mismatch 14 sender count");
  for (i = 0; i < num_senders1; ++i) {
    SC_CHECK_ABORTF (senders1[i] == senders2[i], "Mismatch 12 sender %d", i);
    SC_CHECK_ABORTF (senders1[i] == senders3[i], "Mismatch 13 sender %d", i);
    SC_CHECK_ABORTF (senders1[i] == senders4[i], "Mismatch 14 sender %d", i);
    SC_CHECK_ABORTF (*(int *) sc_array_index_int (pay4, i) ==
                     2 * senders4[i] + 3, "Mismatch payload %d", i);
  }

  SC_FREE (receivers);
  SC_FREE (senders1);
  sc_array_destroy (rec2);
  sc_array_destroy (snd2);
  SC_FREE (senders3);
  sc_array_destroy (rec4);
  sc_array_destroy (pay4);

  sc_stats_compute (mpicomm, SC_STAT_NOTIFY_LAST, stats);
  sc_stats_print (sc_package_id, SC_LP_STATISTICS,
                  SC_STAT_NOTIFY_LAST, stats, 1, 1);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
