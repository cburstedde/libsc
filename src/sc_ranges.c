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
#include <sc_ranges.h>
#include <sc_statistics.h>

static int
sc_ranges_compare (const void *v1, const void *v2)
{
  return *(int *) v1 - *(int *) v2;
}

int
sc_ranges_compute (int num_procs, int *procs,
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
      SC_LDEBUGF ("found empty range prev %d j %d length %d\n",
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
  qsort (ranges, nwin, 2 * sizeof (int), sc_ranges_compare);

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
    SC_LDEBUGF ("empty range %d from %d to %d\n",
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
    SC_LDEBUGF ("range %d from %d to %d\n",
                i, ranges[2 * i], ranges[2 * i + 1]);
  }
#endif

  return nwin;
}

void
sc_ranges_statistics (int log_priority,
                      MPI_Comm mpicomm, int num_procs, int *procs,
                      int rank, int num_ranges, int *ranges)
{
  int                 i, j;
  int                 empties;
  sc_statinfo_t       si;

  empties = 0;
  for (i = 0; i < num_ranges; ++i) {
    for (j = ranges[2 * i]; j <= ranges[2 * i + 1]; ++j) {
      empties += (j != rank && procs[j] == 0);
    }
  }

  sc_statinfo_set1 (&si, (double) empties, NULL);
  sc_statinfo_compute (mpicomm, 1, &si);
  SC_GLOBAL_LOGF (log_priority, "Ranges %d nonpeer %g +- %g min/max %g %g\n",
                  num_ranges, si.average, si.standev, si.min, si.max);
}

/* EOF sc_ranges.c */
