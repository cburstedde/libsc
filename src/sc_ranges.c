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

#include <sc_ranges.h>
#include <sc_statistics.h>

#ifdef SC_ALLGATHER
#include <sc_allgather.h>
#define MPI_Allgather sc_allgather
#endif

static int
sc_ranges_compare (const void *v1, const void *v2)
{
  return *(int *) v1 - *(int *) v2;
}

int
sc_ranges_compute (int package_id, int num_procs, const int *procs,
                   int rank, int first_peer, int last_peer,
                   int num_ranges, int *ranges)
{
  int                 i, j;
  int                 lastw, prev, nwin, length;
  int                 shortest_range, shortest_length;

  SC_ASSERT (rank >= 0 && rank < num_procs);

  /* initialize ranges as empty */
  nwin = 0;
  for (i = 0; i < num_ranges; ++i) {
    ranges[2 * i] = -1;
    ranges[2 * i + 1] = -2;
  }

  /* if no peers are present there are no ranges */
  if (first_peer > last_peer) {
    SC_ASSERT (first_peer == num_procs && last_peer == -1);
    return nwin;
  }

#ifdef SC_DEBUG
  SC_ASSERT (0 <= first_peer && first_peer <= last_peer &&
             last_peer < num_procs);
  SC_ASSERT (first_peer != rank && last_peer != rank);
  SC_ASSERT (procs[first_peer] && procs[last_peer]);
  for (j = 0; j < first_peer; ++j) {
    SC_ASSERT (j == rank || !procs[j]);
  }
  for (j = last_peer + 1; j < num_procs; ++j) {
    SC_ASSERT (j == rank || !procs[j]);
  }
#endif

  /* find a maximum of num_ranges - 1 empty ranges with (start, end) */
  lastw = num_ranges - 1;
  prev = -1;
  for (j = 0; j < num_procs; ++j) {
    if (!procs[j] || j == rank) {
      continue;
    }
    if (prev == -1) {
      prev = j;
      continue;
    }
    if (prev < j - 1) {
      length = j - 1 - prev;
      SC_GEN_LOGF (package_id, SC_LC_NORMAL, SC_LP_DEBUG,
                   "found empty range prev %d j %d length %d\n",
                   prev, j, length);

      /* claim unused range */
      for (i = 0; i < num_ranges; ++i) {
        if (ranges[2 * i] == -1) {
          ranges[2 * i] = prev + 1;
          ranges[2 * i + 1] = j - 1;
          break;
        }
      }
      SC_ASSERT (i < num_ranges);
      nwin = i + 1;

      /* if all ranges are used, remove the shortest */
      if (nwin == num_ranges) {
        nwin = lastw;
        shortest_range = -1;
        shortest_length = num_procs + 1;
        for (i = 0; i < num_ranges; ++i) {
          length = ranges[2 * i + 1] - ranges[2 * i] + 1;
          if (length < shortest_length) {
            shortest_range = i;
            shortest_length = length;
          }
        }
        SC_ASSERT (shortest_range >= 0 && shortest_range <= lastw);
        if (shortest_range < lastw) {
          ranges[2 * shortest_range] = ranges[2 * lastw];
          ranges[2 * shortest_range + 1] = ranges[2 * lastw + 1];
        }
        ranges[2 * lastw] = -1;
        ranges[2 * lastw + 1] = -2;
      }
    }
    prev = j;
  }
  SC_ASSERT (nwin >= 0 && nwin < num_ranges);

  /* sort empty ranges by start rank */
  qsort (ranges, (size_t) nwin, 2 * sizeof (int), sc_ranges_compare);

#ifdef SC_DEBUG
  /* check consistency of empty ranges */
  for (i = 0; i < nwin; ++i) {
    SC_ASSERT (ranges[2 * i] <= ranges[2 * i + 1]);
    if (i < nwin - 1) {
      SC_ASSERT (ranges[2 * i + 1] < ranges[2 * (i + 1)] - 1);
    }
  }
  for (i = nwin; i < num_ranges; ++i) {
    SC_ASSERT (ranges[2 * i] == -1);
    SC_ASSERT (ranges[2 * i + 1] == -2);
  }
  for (i = 0; i < nwin; ++i) {
    SC_GEN_LOGF (package_id, SC_LC_NORMAL, SC_LP_DEBUG,
                 "empty range %d from %d to %d\n",
                 i, ranges[2 * i], ranges[2 * i + 1]);
  }
#endif

  /* compute real ranges from empty ranges */
  ranges[2 * nwin + 1] = last_peer;
  for (i = nwin; i > 0; --i) {
    ranges[2 * i] = ranges[2 * i - 1] + 1;
    ranges[2 * i - 1] = ranges[2 * (i - 1)] - 1;
  }
  ranges[0] = first_peer;
  ++nwin;

#ifdef SC_DEBUG
  for (i = 0; i < nwin; ++i) {
    SC_ASSERT (ranges[2 * i] <= ranges[2 * i + 1]);
    if (i < nwin - 1) {
      SC_ASSERT (ranges[2 * i + 1] < ranges[2 * (i + 1)] - 1);
    }
  }
  for (i = nwin; i < num_ranges; ++i) {
    SC_ASSERT (ranges[2 * i] == -1);
    SC_ASSERT (ranges[2 * i + 1] == -2);
  }
  for (i = 0; i < nwin - 1; ++i) {
    for (j = ranges[2 * i + 1] + 1; j < ranges[2 * (i + 1)]; ++j) {
      SC_ASSERT (j == rank || !procs[j]);
    }
  }
  for (i = 0; i < nwin; ++i) {
    SC_GEN_LOGF (package_id, SC_LC_NORMAL, SC_LP_DEBUG,
                 "range %d from %d to %d\n", i,
                 ranges[2 * i], ranges[2 * i + 1]);
  }
#endif

  return nwin;
}

int
sc_ranges_adaptive (int package_id, sc_MPI_Comm mpicomm,
                    const int *procs, int *inout1, int *inout2,
                    int num_ranges, int *ranges, int **global_ranges)
{
  int                 mpiret;
  int                 j, num_procs, rank;
  int                 first_peer, last_peer;
  int                 local[2], global[2];
  int                 nwin, maxwin, twomaxwin;

  /* get processor related information */
  mpiret = sc_MPI_Comm_size (mpicomm, &num_procs);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &rank);
  SC_CHECK_MPI (mpiret);
  first_peer = *inout1;
  last_peer = *inout2;

  /* count max peers and compute the local ranges */
  local[0] = 0;
  for (j = 0; j < num_procs; ++j) {
    local[0] += (procs[j] > 0 && j != rank);
  }
  local[1] = nwin =
    sc_ranges_compute (package_id, num_procs, procs, rank,
                       first_peer, last_peer, num_ranges, ranges);

  /* communicate the maximum number of peers and ranges */
  mpiret = sc_MPI_Allreduce (local, global, 2, sc_MPI_INT, sc_MPI_MAX, mpicomm);
  SC_CHECK_MPI (mpiret);
  *inout1 = global[0];
  *inout2 = maxwin = global[1];
  twomaxwin = 2 * maxwin;
  SC_ASSERT (nwin <= maxwin && maxwin <= num_ranges);

  /* distribute everybody's range information */
  if (global_ranges != NULL) {
    *global_ranges = SC_ALLOC (int, twomaxwin * num_procs);
    mpiret = sc_MPI_Allgather (ranges, twomaxwin, sc_MPI_INT,
                            *global_ranges, twomaxwin, sc_MPI_INT, mpicomm);
    SC_CHECK_MPI (mpiret);
  }

  return nwin;
}

void
sc_ranges_decode (int num_procs, int rank,
                  int max_ranges, const int *global_ranges,
                  int *num_receivers, int *receiver_ranks,
                  int *num_senders, int *sender_ranks)
{
  int             i, j;
  int             nr, ns;
  const int      *the_ranges;

#ifdef SC_DEBUG
  int             done;

  /* verify consistency of ranges */
  for (j = 0; j < num_procs; ++j) {
    the_ranges = global_ranges + 2 * max_ranges * j;
    done = 0;
    for (i = 0; i < max_ranges; ++i) {
      if (the_ranges[2 * i] < 0) {
        done = 1;
      }
      if (!done) {
        SC_ASSERT (the_ranges[2 * i] <= the_ranges[2 * i + 1]);
        SC_ASSERT (i == 0 ||
                   the_ranges[2 * (i - 1) + 1] + 1 < the_ranges[2 * i]);
      }
      else {
        SC_ASSERT (the_ranges[2 * i] == -1 && the_ranges[2 * i + 1] == -2);
      }
    }
  }
#endif
  
  /* identify receivers */
  nr = 0;
  the_ranges = global_ranges + 2 * max_ranges * rank;
  for (i = 0; i < max_ranges; ++i) {
    if (the_ranges[2 * i] < 0) {
      /* this processor uses less ranges than the maximum */
      break;
    }
    for (j = the_ranges[2 * i]; j <= the_ranges[2 * i + 1]; ++j) {
      SC_ASSERT (0 <= j && j < num_procs);

      /* exclude self */
      if (j == rank) {
        continue;
      }
      receiver_ranks[nr++] = j;
    }
  }
  *num_receivers = nr;

  /* identify senders */
  ns = 0;
  for (j = 0; j < num_procs; ++j) {
    /* exclude self */
    if (j == rank) {
      continue;
    }

    /* look through that processor's ranges */
    the_ranges = global_ranges + 2 * max_ranges * j;
    for (i = 0; i < max_ranges; ++i) {
      if (the_ranges[2 * i] < 0) {
        /* processor j uses less ranges than the maximum */
        break;
      }
      if (rank <= the_ranges[2 * i + 1]) {
        if (rank >= the_ranges[2 * i]) {
          /* processor j is a potential sender to rank */
          sender_ranks[ns++] = j;
        }
        break;
      }
    }
  }
  *num_senders = ns;
}

void
sc_ranges_statistics (int package_id, int log_priority,
                      sc_MPI_Comm mpicomm, int num_procs, const int *procs,
                      int rank, int num_ranges, int *ranges)
{
  int                 i, j;
  int                 empties;
  sc_statinfo_t       si;

  empties = 0;
  for (i = 0; i < num_ranges; ++i) {
    SC_ASSERT (ranges[2 * i] >= 0 || ranges[2 * i] > ranges[2 * i + 1]);
    for (j = ranges[2 * i]; j <= ranges[2 * i + 1]; ++j) {
      empties += (j != rank && procs[j] == 0);
    }
  }

  sc_stats_set1 (&si, (double) empties, NULL);
  sc_stats_compute (mpicomm, 1, &si);
  SC_GEN_LOGF (package_id, SC_LC_GLOBAL, log_priority,
               "Ranges %d nonpeer %g +- %g min/max %g %g\n",
               num_ranges, si.average, si.standev, si.min, si.max);
}
