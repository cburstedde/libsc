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

static void
compute_superset_trivial (sc_array_t * receivers,
                          sc_array_t * extra_receivers,
                          sc_array_t * super_senders, sc_notify_t * notify,
                          void *ctx)
{
  sc_MPI_Comm         comm = sc_notify_get_comm (notify);
  sc_array_t         *recv_sorted = NULL;
  int                 size, i;
  int                *isenders;
  int                 mpiret;

  if (sc_array_is_sorted (receivers, sc_int_compare)) {
    recv_sorted = receivers;
  }
  else {
    recv_sorted = sc_array_new_count (sizeof (int), receivers->elem_count);
    sc_array_copy (recv_sorted, receivers);
    sc_array_sort (recv_sorted, sc_int_compare);
  }

  mpiret = sc_MPI_Comm_size (comm, &size);
  SC_CHECK_MPI (mpiret);

  sc_array_resize (super_senders, (size_t) size);
  isenders = (int *) super_senders->array;
  sc_array_resize (extra_receivers, (size_t) size - recv_sorted->elem_count);
  sc_array_truncate (extra_receivers);

  for (i = 0; i < size; i++) {
    isenders[i] = i;
    if (sc_array_bsearch (recv_sorted, &i, sc_int_compare) < 0) {
      *((int *) sc_array_push (extra_receivers)) = i;
    }
  }
  if (recv_sorted != receivers) {
    sc_array_destroy (recv_sorted);
  }
}

int
main (int argc, char **argv)
{
  int                 i, j, k;
  int                 mpiret;
  int                 mpisize, mpirank;
  int                *senders1, num_senders1;
  int                *senders2, num_senders2;
  int                *senders3, num_senders3;
  int                *senders4, num_senders4;
  int                *senders5, num_senders5;
  int                *receivers, num_receivers;
  int                *off5;
  int                *pay5;
  int                 ntop, nint, nbot;
  double              elapsed_allgather;
  double              elapsed_native;
  double              elapsed_nopayl;
  double              elapsed_payl;
  double              elapsed_paylv;
  sc_MPI_Comm         mpicomm;
  sc_array_t         *rec2, *snd2, *rec4, *pay4, *rec5, *snd5, *inpay5,
    *outpay5, *inoff5, *outoff5;
  sc_statinfo_t       stats[3 * SC_NOTIFY_NUM_TYPES + 2];
  sc_notify_t        *notify;
  char                namep[SC_NOTIFY_NUM_TYPES][2][BUFSIZ];

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpicomm = sc_MPI_COMM_WORLD;
  mpiret = sc_MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  sc_init (mpicomm, 1, 1, NULL, SC_LP_DEFAULT);

  /* grap parameters for notify_nary from command line */
  ntop = sc_notify_nary_ntop_default;
  nint = sc_notify_nary_nint_default;
  nbot = sc_notify_nary_nbot_default;
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
  mpiret = sc_MPI_Barrier (mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_allgather = -sc_MPI_Wtime ();
  mpiret = sc_notify_allgather (receivers, num_receivers,
                                senders1, &num_senders1, mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_allgather += sc_MPI_Wtime ();
  sc_stats_set1 (stats + 3 * SC_NOTIFY_NUM_TYPES, elapsed_allgather,
                 "Allgather");

  mpiret = sc_MPI_Barrier (mpicomm);
  SC_CHECK_MPI (mpiret);
  SC_GLOBAL_INFO ("Testing native sc_notify\n");
  senders3 = SC_ALLOC (int, mpisize);
  mpiret = sc_MPI_Barrier (mpicomm);
  SC_CHECK_MPI (mpiret);

  elapsed_native = -sc_MPI_Wtime ();
  mpiret = sc_notify (receivers, num_receivers,
                      senders3, &num_senders3, mpicomm);
  SC_CHECK_MPI (mpiret);
  elapsed_native += sc_MPI_Wtime ();
  sc_stats_set1 (stats + 3 * SC_NOTIFY_NUM_TYPES + 1, elapsed_native,
                 "Native");

  SC_CHECK_ABORT (num_senders1 == num_senders3, "Mismatch 13 sender count");
  for (i = 0; i < num_senders1; ++i) {
    SC_CHECK_ABORTF (senders1[i] == senders3[i], "Mismatch 13 sender %d", i);
  }

  for (j = 0; j < SC_NOTIFY_NUM_TYPES; j++) {
    const char         *name = sc_notify_type_strings[j];

    /* temporarily skip this; we need to catch this softly for non-MPI */
    if (j == SC_NOTIFY_PCX || j == SC_NOTIFY_RSX || j == SC_NOTIFY_NBX ||
        j == SC_NOTIFY_SUPERSET) {
      continue;
    }

    snprintf (namep[j][0], BUFSIZ - 1, "%s payload", name);
    snprintf (namep[j][1], BUFSIZ - 1, "%s payloadv", name);

    mpiret = sc_MPI_Barrier (mpicomm);
    SC_CHECK_MPI (mpiret);
    SC_GLOBAL_INFOF ("Testing sc_notify_payload %s\n", name);
    notify = sc_notify_new (mpicomm);
    sc_notify_set_type (notify, (sc_notify_type_t) j);
    if (j == SC_NOTIFY_NARY) {
      SC_GLOBAL_INFOF ("  SC_NOTIFY_NARY widths %d %d %d\n", ntop, nint,
                       nbot);
      sc_notify_nary_set_widths (notify, ntop, nint, nbot);
    }
    if (j == SC_NOTIFY_SUPERSET) {
      sc_notify_superset_set_callback (notify, compute_superset_trivial,
                                       NULL);
    }
    rec2 = sc_array_new_data (receivers, sizeof (int), num_receivers);
    snd2 = sc_array_new (sizeof (int));
    mpiret = sc_MPI_Barrier (mpicomm);
    SC_CHECK_MPI (mpiret);
    elapsed_nopayl = -sc_MPI_Wtime ();
    sc_notify_payload (rec2, snd2, NULL, NULL, 1, notify);
    elapsed_nopayl += sc_MPI_Wtime ();
    senders2 = (int *) snd2->array;
    num_senders2 = (int) snd2->elem_count;
    sc_stats_set1 (stats + 3 * j, elapsed_nopayl, name);

    mpiret = sc_MPI_Barrier (mpicomm);
    SC_CHECK_MPI (mpiret);
    SC_GLOBAL_INFOF ("Testing sc_notify_payload %s with payload\n", name);
    rec4 = sc_array_new_count (sizeof (int), num_receivers);
    pay4 = sc_array_new_count (sizeof (int), num_receivers);
    for (i = 0; i < num_receivers; ++i) {
      *(int *) sc_array_index_int (rec4, i) = receivers[i];
      *(int *) sc_array_index_int (pay4, i) = 2 * mpirank + 3;
    }
    mpiret = sc_MPI_Barrier (mpicomm);
    SC_CHECK_MPI (mpiret);
    elapsed_payl = -sc_MPI_Wtime ();
    sc_notify_payload (rec4, NULL, pay4, NULL, 1, notify);
    elapsed_payl += sc_MPI_Wtime ();
    senders4 = (int *) rec4->array;
    num_senders4 = (int) rec4->elem_count;
    SC_ASSERT ((int) pay4->elem_count == num_senders4);
    sc_stats_set1 (stats + 3 * j + 1, elapsed_payl, namep[j][0]);

    mpiret = sc_MPI_Barrier (mpicomm);
    SC_CHECK_MPI (mpiret);
    SC_GLOBAL_INFOF ("Testing sc_notify_payloadv %s\n", name);
    rec5 = sc_array_new_count (sizeof (int), num_receivers);
    snd5 = sc_array_new (sizeof (int));
    inpay5 = sc_array_new_count (sizeof (int), num_receivers * mpirank);
    outpay5 = sc_array_new (sizeof (int));
    inoff5 = sc_array_new_count (sizeof (int), num_receivers + 1);
    outoff5 = sc_array_new (sizeof (int));
    *(int *) sc_array_index (inoff5, 0) = 0;
    for (i = 0; i < num_receivers; ++i) {
      *(int *) sc_array_index_int (rec5, i) = receivers[i];
      *(int *) sc_array_index_int (inoff5, i + 1) = mpirank * (i + 1);
      for (k = 0; k < mpirank; k++) {
        *(int *) sc_array_index_int (inpay5, mpirank * i + k) =
          3 * mpirank + 5;
      }
    }
    mpiret = sc_MPI_Barrier (mpicomm);
    SC_CHECK_MPI (mpiret);
    elapsed_paylv = -sc_MPI_Wtime ();
    sc_notify_payloadv (rec5, snd5, inpay5, outpay5, inoff5, outoff5, 1,
                        notify);
    elapsed_paylv += sc_MPI_Wtime ();
    senders5 = (int *) snd5->array;
    pay5 = (int *) outpay5->array;
    off5 = (int *) outoff5->array;
    num_senders5 = (int) snd5->elem_count;
    sc_stats_set1 (stats + 3 * j + 2, elapsed_paylv, namep[j][1]);

    sc_notify_destroy (notify);

    SC_CHECK_ABORT (num_senders1 == num_senders2, "Mismatch 12 sender count");
    SC_CHECK_ABORT (num_senders1 == num_senders4, "Mismatch 14 sender count");
    SC_CHECK_ABORT (num_senders1 == num_senders5, "Mismatch 15 sender count");
    for (i = 0; i < num_senders1; ++i) {
      SC_CHECK_ABORTF (senders1[i] == senders2[i], "Mismatch 12 sender %d",
                       i);
      SC_CHECK_ABORTF (senders1[i] == senders4[i], "Mismatch 14 sender %d",
                       i);
      SC_CHECK_ABORTF (senders1[i] == senders5[i], "Mismatch 15 sender %d",
                       i);
      SC_CHECK_ABORTF (*(int *) sc_array_index_int (pay4, i) ==
                       2 * senders4[i] + 3, "Mismatch payload %d", i);
      SC_CHECK_ABORTF (off5[i + 1] - off5[i] == senders1[i],
                       "Mismatch payloadv size %d", i);
      for (k = 0; k < senders1[i]; k++) {
        SC_CHECK_ABORTF (pay5[off5[i] + k] == 3 * senders1[i] + 5,
                         "Mismatch payloadv %d", i);
      }
    }
    sc_array_destroy (rec2);
    sc_array_destroy (snd2);
    sc_array_destroy (rec4);
    sc_array_destroy (pay4);
    sc_array_destroy (rec5);
    sc_array_destroy (snd5);
    sc_array_destroy (inpay5);
    sc_array_destroy (outpay5);
    sc_array_destroy (inoff5);
    sc_array_destroy (outoff5);
  }

  SC_FREE (receivers);
  SC_FREE (senders1);
  SC_FREE (senders3);

  sc_stats_compute (mpicomm, 3 * SC_NOTIFY_NUM_TYPES + 2, stats);
  sc_stats_print (sc_package_id, SC_LP_STATISTICS,
                  3 * SC_NOTIFY_NUM_TYPES + 2, stats, 1, 1);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
