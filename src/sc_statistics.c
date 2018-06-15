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

#include <sc_statistics.h>

#ifdef SC_ENABLE_MPI

static void
sc_stats_mpifunc (void *invec, void *inoutvec, int *len,
                  sc_MPI_Datatype * datatype)
{
  int                 i;
  double             *in = (double *) invec;
  double             *inout = (double *) inoutvec;

  for (i = 0; i < *len; ++i) {
    /* sum count, values and their squares */
    inout[0] += in[0];
    if (in[0]) {                /* ignore statistics when no count */
      inout[1] += in[1];
      inout[2] += in[2];

      /* compute minimum and its rank */
      if (in[3] < inout[3]) {
        inout[3] = in[3];
        inout[5] = in[5];
      }
      else if (in[3] == inout[3]) {     /* ignore the comparison warning */
        inout[5] = SC_MIN (in[5], inout[5]);
      }

      /* compute maximum and its rank */
      if (in[4] > inout[4]) {
        inout[4] = in[4];
        inout[6] = in[6];
      }
      else if (in[4] == inout[4]) {     /* ignore the comparison warning */
        inout[6] = SC_MIN (in[6], inout[6]);
      }
    }

    /* advance to next data set */
    in += 7;
    inout += 7;
  }
}

#endif /* SC_ENABLE_MPI */

const int           sc_stats_group_all = -2;
const int           sc_stats_prio_all = -3;

void
sc_stats_set1 (sc_statinfo_t * stats, double value, const char *variable)
{
  stats->dirty = 1;
  stats->count = 1;
  stats->sum_values = value;
  stats->sum_squares = value * value;
  stats->min = value;
  stats->max = value;
  stats->average = 0.;
  stats->variable = variable;
  stats->group = sc_stats_group_all;
  stats->prio = sc_stats_prio_all;
}

void
sc_stats_init_ext (sc_statinfo_t * stats, const char *variable,
                   int stats_group, int stats_prio)
{
  SC_ASSERT (stats_group == sc_stats_group_all || stats_group >= 0);
  SC_ASSERT (stats_prio == sc_stats_prio_all || stats_prio >= 0);

  stats->dirty = 1;
  stats->count = 0;
  stats->sum_values = stats->sum_squares = 0.;
  stats->min = stats->max = 0.;
  stats->average = 0.;
  stats->variable = variable;
  stats->group = stats_group;
  stats->prio = stats_prio;
}

void
sc_stats_init (sc_statinfo_t * stats, const char *variable)
{
  sc_stats_init_ext (stats, variable, sc_stats_group_all, sc_stats_prio_all);
}

void
sc_stats_set_group_prio (sc_statinfo_t * stats,
                         int stats_group, int stats_prio)
{
  SC_ASSERT (stats_group == sc_stats_group_all || stats_group >= 0);
  SC_ASSERT (stats_prio == sc_stats_prio_all || stats_prio >= 0);

  stats->group = stats_group;
  stats->prio = stats_prio;
}

void
sc_stats_accumulate (sc_statinfo_t * stats, double value)
{
  SC_ASSERT (stats->dirty);
  if (stats->count) {
    stats->count++;
    stats->sum_values += value;
    stats->sum_squares += value * value;
    stats->min = SC_MIN (stats->min, value);
    stats->max = SC_MAX (stats->max, value);
  }
  else {
    stats->count = 1;
    stats->sum_values = value;
    stats->sum_squares = value * value;
    stats->min = value;
    stats->max = value;
  }
}

void
sc_stats_compute (sc_MPI_Comm mpicomm, int nvars, sc_statinfo_t * stats)
{
  int                 i;
  int                 mpiret;
  int                 rank;
  double              cnt, avg;
  double             *flat;
  double             *flatin;
  double             *flatout;
#ifdef SC_ENABLE_MPI
  sc_MPI_Op           op;
  sc_MPI_Datatype     ctype;
#endif

  mpiret = sc_MPI_Comm_rank (mpicomm, &rank);
  SC_CHECK_MPI (mpiret);

  flat = SC_ALLOC (double, 2 * 7 * nvars);
  flatin = flat;
  flatout = flat + 7 * nvars;

  for (i = 0; i < nvars; ++i) {
    if (!stats[i].dirty) {
      memset (flatin + 7 * i, 0, 7 * sizeof (*flatin));
      continue;
    }
    flatin[7 * i + 0] = (double) stats[i].count;
    flatin[7 * i + 1] = stats[i].sum_values;
    flatin[7 * i + 2] = stats[i].sum_squares;
    flatin[7 * i + 3] = stats[i].min;
    flatin[7 * i + 4] = stats[i].max;
    flatin[7 * i + 5] = (double) rank;  /* rank that attains minimum */
    flatin[7 * i + 6] = (double) rank;  /* rank that attains maximum */
  }

#ifndef SC_ENABLE_MPI
  memcpy (flatout, flatin, 7 * nvars * sizeof (*flatout));
#else
  mpiret = MPI_Type_contiguous (7, MPI_DOUBLE, &ctype);
  SC_CHECK_MPI (mpiret);

  mpiret = MPI_Type_commit (&ctype);
  SC_CHECK_MPI (mpiret);

  mpiret = MPI_Op_create ((MPI_User_function *) sc_stats_mpifunc, 1, &op);
  SC_CHECK_MPI (mpiret);

  mpiret = MPI_Allreduce (flatin, flatout, nvars, ctype, op, mpicomm);
  SC_CHECK_MPI (mpiret);

  mpiret = MPI_Op_free (&op);
  SC_CHECK_MPI (mpiret);

  mpiret = MPI_Type_free (&ctype);
  SC_CHECK_MPI (mpiret);
#endif /* SC_ENABLE_MPI */

  for (i = 0; i < nvars; ++i) {
    if (!stats[i].dirty) {
      continue;
    }
    cnt = flatout[7 * i + 0];
    stats[i].count = (long) cnt;
    if (!cnt) {
      continue;
    }
    stats[i].sum_values = flatout[7 * i + 1];
    stats[i].sum_squares = flatout[7 * i + 2];
    stats[i].min = flatout[7 * i + 3];
    stats[i].max = flatout[7 * i + 4];
    stats[i].min_at_rank = (int) flatout[7 * i + 5];
    stats[i].max_at_rank = (int) flatout[7 * i + 6];
    stats[i].average = avg = stats[i].sum_values / cnt;
    stats[i].variance = stats[i].sum_squares / cnt - avg * avg;
    stats[i].variance = SC_MAX (stats[i].variance, 0.);
    stats[i].variance_mean = stats[i].variance / cnt;
    stats[i].standev = sqrt (stats[i].variance);
    stats[i].standev_mean = sqrt (stats[i].variance_mean);
    stats[i].dirty = 0;
  }

  SC_FREE (flat);
}

void
sc_stats_compute1 (sc_MPI_Comm mpicomm, int nvars, sc_statinfo_t * stats)
{
  int                 i;
  double              value;

  for (i = 0; i < nvars; ++i) {
    value = stats[i].sum_values;

    stats[i].count = 1;
    stats[i].sum_squares = value * value;
    stats[i].min = value;
    stats[i].max = value;
  }

  sc_stats_compute (mpicomm, nvars, stats);
}

void
sc_stats_print_ext (int package_id, int log_priority,
                    int nvars, sc_statinfo_t * stats,
                    int stats_group, int stats_prio, int full, int summary)
{
  int                 i, count;
  sc_statinfo_t      *si;
  char                buffer[BUFSIZ];

  SC_ASSERT (stats_group == sc_stats_group_all || stats_group >= 0);
  SC_ASSERT (stats_prio == sc_stats_prio_all || stats_prio >= 0);

  if (full) {
    for (i = 0; i < nvars; ++i) {
      si = &stats[i];

      /* filter output by group and priority */
      if (stats_group != sc_stats_group_all &&
          si->group != sc_stats_group_all && si->group != stats_group) {
        continue;
      }
      if (stats_prio != sc_stats_prio_all &&
          si->prio != sc_stats_prio_all && si->prio < stats_prio) {
        continue;
      }

      /* begin printing */
      if (si->variable != NULL) {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, log_priority,
                     "Statistics for %s\n", si->variable);
      }
      else {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, log_priority,
                     "Statistics for %d\n", i);
      }
      SC_GEN_LOGF (package_id, SC_LC_GLOBAL, log_priority,
                   "   Global number of values: %5ld\n", si->count);
      if (!si->count) {
        continue;
      }
      if (si->average != 0.) {  /* ignore the comparison warning */
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, log_priority,
                     "   Mean value (std. dev.):         %g (%.3g = %.3g%%)\n",
                     si->average, si->standev,
                     100. * si->standev / fabs (si->average));
      }
      else {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, log_priority,
                     "   Mean value (std. dev.):         %g (%.3g)\n",
                     si->average, si->standev);
      }
      SC_GEN_LOGF (package_id, SC_LC_GLOBAL, log_priority,
                   "   Minimum attained at rank %5d: %g\n",
                   si->min_at_rank, si->min);
      SC_GEN_LOGF (package_id, SC_LC_GLOBAL, log_priority,
                   "   Maximum attained at rank %5d: %g\n",
                   si->max_at_rank, si->max);
    }
  }
  else {
    for (i = 0; i < nvars; ++i) {
      si = &stats[i];
      if (si->variable != NULL) {
        snprintf (buffer, BUFSIZ, "for %s:", si->variable);
      }
      else {
        snprintf (buffer, BUFSIZ, "for %d:", i);
      }
      if (si->average != 0.) {  /* ignore the comparison warning */
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, log_priority,
                     "Mean (sigma) %-28s %g (%.3g = %.3g%%)\n",
                     buffer, si->average, si->standev,
                     100. * si->standev / fabs (si->average));
      }
      else {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, log_priority,
                     "Mean (sigma) %-28s %g (%.3g)\n", buffer,
                     si->average, si->standev);
      }
    }
  }

  if (summary) {
    count = snprintf (buffer, BUFSIZ, "Summary = ");
    for (i = 0; i < nvars && count >= 0 && (size_t) count < BUFSIZ; ++i) {
      si = &stats[i];
      count += snprintf (buffer + count, BUFSIZ - count,
                         "%s%g", i == 0 ? "[ " : " ", si->average);
    }
    if (count >= 0 && (size_t) count < BUFSIZ) {
      snprintf (buffer + count, BUFSIZ - count, "%s", " ];\n");
      SC_GEN_LOG (package_id, SC_LC_GLOBAL, log_priority, buffer);
    }
    else {
      SC_GEN_LOG (package_id, SC_LC_GLOBAL, log_priority,
                  "Summary overflow\n");
    }
    count = snprintf (buffer, BUFSIZ, "Maximum = ");
    for (i = 0; i < nvars && count >= 0 && (size_t) count < BUFSIZ; ++i) {
      si = &stats[i];
      count += snprintf (buffer + count, BUFSIZ - count,
                         "%s%g", i == 0 ? "[ " : " ", si->max);
    }
    if (count >= 0 && (size_t) count < BUFSIZ) {
      snprintf (buffer + count, BUFSIZ - count, "%s", " ];\n");
      SC_GEN_LOG (package_id, SC_LC_GLOBAL, log_priority, buffer);
    }
    else {
      SC_GEN_LOG (package_id, SC_LC_GLOBAL, log_priority,
                  "Maximum overflow\n");
    }
  }
}

void
sc_stats_print (int package_id, int log_priority,
                int nvars, sc_statinfo_t * stats, int full, int summary)
{
  sc_stats_print_ext (package_id, log_priority, nvars, stats,
                      sc_stats_group_all, sc_stats_prio_all, full, summary);
}

sc_statistics_t    *
sc_statistics_new (sc_MPI_Comm mpicomm)
{
  sc_statistics_t    *stats;

  stats = SC_ALLOC (sc_statistics_t, 1);
  stats->mpicomm = mpicomm;
  stats->kv = sc_keyvalue_new ();
  stats->sarray = sc_array_new (sizeof (sc_statinfo_t));

  return stats;
}

void
sc_statistics_destroy (sc_statistics_t * stats)
{
  sc_keyvalue_destroy (stats->kv);
  sc_array_destroy (stats->sarray);

  SC_FREE (stats);
}

void
sc_statistics_add (sc_statistics_t * stats, const char *name)
{
  int                 i;
  sc_statinfo_t      *si;

  /* always check for wrong usage and output adequate error message */
  SC_CHECK_ABORTF (!sc_keyvalue_exists (stats->kv, name),
                   "Statistics variable \"%s\" exists already", name);

  i = (int) stats->sarray->elem_count;
  si = (sc_statinfo_t *) sc_array_push (stats->sarray);
  sc_stats_set1 (si, 0, name);

  sc_keyvalue_set_int (stats->kv, name, i);
}

void
sc_statistics_set (sc_statistics_t * stats, const char *name, double value)
{
  int                 i;
  sc_statinfo_t      *si;

  i = sc_keyvalue_get_int (stats->kv, name, -1);

  /* always check for wrong usage and output adequate error message */
  SC_CHECK_ABORTF (i >= 0, "Statistics variable \"%s\" does not exist", name);

  si = (sc_statinfo_t *) sc_array_index_int (stats->sarray, i);

  sc_stats_set1 (si, value, name);
}

void
sc_statistics_add_empty (sc_statistics_t * stats, const char *name)
{
  int                 i;
  sc_statinfo_t      *si;

  /* always check for wrong usage and output adequate error message */
  SC_CHECK_ABORTF (!sc_keyvalue_exists (stats->kv, name),
                   "Statistics variable \"%s\" exists already", name);

  i = (int) stats->sarray->elem_count;
  si = (sc_statinfo_t *) sc_array_push (stats->sarray);
  sc_stats_init (si, name);

  sc_keyvalue_set_int (stats->kv, name, i);
}

int
sc_statistics_has (sc_statistics_t * stats, const char *name)
{
  return sc_keyvalue_exists (stats->kv, name);
}

void
sc_statistics_accumulate (sc_statistics_t * stats, const char *name,
                          double value)
{
  int                 i;
  sc_statinfo_t      *si;

  i = sc_keyvalue_get_int (stats->kv, name, -1);

  /* always check for wrong usage and output adequate error message */
  SC_CHECK_ABORTF (i >= 0, "Statistics variable \"%s\" does not exist", name);

  si = (sc_statinfo_t *) sc_array_index_int (stats->sarray, i);

  sc_stats_accumulate (si, value);
}

void
sc_statistics_compute (sc_statistics_t * stats)
{
  sc_stats_compute (stats->mpicomm, (int) stats->sarray->elem_count,
                    (sc_statinfo_t *) stats->sarray->array);
}

void
sc_statistics_print (sc_statistics_t * stats,
                     int package_id, int log_priority, int full, int summary)
{
  sc_stats_print (package_id, log_priority,
                  (int) stats->sarray->elem_count,
                  (sc_statinfo_t *) stats->sarray->array, full, summary);
}
