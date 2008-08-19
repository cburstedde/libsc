/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

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

#include <sc.h>
#include <sc_statistics.h>

#ifdef SC_PAPI
#include <papi.h>
#endif

#ifdef SC_MPI

static void
sc_statinfo_mpifunc (void *invec, void *inoutvec, int *len,
                     MPI_Datatype * datatype)
{
  int                 i;
  double             *in = invec;
  double             *inout = inoutvec;

  for (i = 0; i < *len; ++i) {
    /* sum count, values and their squares */
    inout[0] += in[0];
    inout[1] += in[1];
    inout[2] += in[2];

    /* compute minimum and its rank */
    if (in[3] < inout[3]) {
      inout[3] = in[3];
      inout[5] = in[5];
    }
    else if (in[3] == inout[3]) {       /* ignore the comparison warning */
      inout[5] = SC_MIN (in[5], inout[5]);
    }

    /* compute maximum and its rank */
    if (in[4] > inout[4]) {
      inout[4] = in[4];
      inout[6] = in[6];
    }
    else if (in[4] == inout[4]) {       /* ignore the comparison warning */
      inout[6] = SC_MIN (in[6], inout[6]);
    }

    /* advance to next data set */
    in += 7;
    inout += 7;
  }
}

#endif /* SC_MPI */

void
sc_statinfo_set1 (sc_statinfo_t * stats, double value, const char *variable)
{
  stats->count = 1;
  stats->sum_values = value;
  stats->sum_squares = value * value;
  stats->min = value;
  stats->max = value;
  stats->variable = variable;
}

void
sc_statinfo_compute (MPI_Comm mpicomm, int nvars, sc_statinfo_t * stats)
{
  int                 i;
  int                 mpiret;
  int                 rank;
  double              cnt, avg;
  double             *flat;
  double             *flatin;
  double             *flatout;
#ifdef SC_MPI
  MPI_Op              op;
  MPI_Datatype        ctype;
#endif

  if (mpicomm == MPI_COMM_NULL) {
    rank = 0;
  }
  else {
    mpiret = MPI_Comm_rank (mpicomm, &rank);
    SC_CHECK_MPI (mpiret);
  }

  flat = SC_ALLOC (double, 2 * 7 * nvars);
  flatin = flat;
  flatout = flat + 7 * nvars;

  for (i = 0; i < nvars; ++i) {
    flatin[7 * i + 0] = (double) stats[i].count;
    flatin[7 * i + 1] = stats[i].sum_values;
    flatin[7 * i + 2] = stats[i].sum_squares;
    flatin[7 * i + 3] = stats[i].min;
    flatin[7 * i + 4] = stats[i].max;
    flatin[7 * i + 5] = (double) rank;  /* rank that attains minimum */
    flatin[7 * i + 6] = (double) rank;  /* rank that attains maximum */
  }

  memcpy (flatout, flatin, 7 * nvars * sizeof (*flatout));
#ifdef SC_MPI
  if (mpicomm != MPI_COMM_NULL) {
    mpiret = MPI_Type_contiguous (7, MPI_DOUBLE, &ctype);
    SC_CHECK_MPI (mpiret);

    mpiret = MPI_Type_commit (&ctype);
    SC_CHECK_MPI (mpiret);

    mpiret = MPI_Op_create (sc_statinfo_mpifunc, 1, &op);
    SC_CHECK_MPI (mpiret);

    mpiret = MPI_Allreduce (flatin, flatout, nvars, ctype, op, mpicomm);
    SC_CHECK_MPI (mpiret);

    mpiret = MPI_Op_free (&op);
    SC_CHECK_MPI (mpiret);

    mpiret = MPI_Type_free (&ctype);
    SC_CHECK_MPI (mpiret);
  }
#endif /* SC_MPI */

  for (i = 0; i < nvars; ++i) {
    cnt = flatout[7 * i + 0];
    stats[i].count = (long) cnt;
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
  }

  SC_FREE (flat);
}

void
sc_statinfo_compute1 (MPI_Comm mpicomm, int nvars, sc_statinfo_t * stats)
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

  sc_statinfo_compute (mpicomm, nvars, stats);
}

void
sc_statinfo_print (int log_priority,
                   int nvars, sc_statinfo_t * stats, bool full, bool summary)
{
  int                 i;
  sc_statinfo_t      *si;
  char                buffer[BUFSIZ];

  if (full) {
    for (i = 0; i < nvars; ++i) {
      si = &stats[i];
      if (si->variable != NULL) {
        SC_GLOBAL_LOGF (log_priority, "Statistics for %s\n", si->variable);
      }
      else {
        SC_GLOBAL_LOGF (log_priority, "Statistics for %d\n", i);
      }
      SC_GLOBAL_LOGF (log_priority, "   Global number of values: %5ld\n",
                      si->count);
      if (si->average != 0.) {  /* ignore the comparison warning */
        SC_GLOBAL_LOGF (log_priority,
                        "   Mean value (std. dev.):         %g (%.3g = %.3g%%)\n",
                        si->average, si->standev,
                        100. * si->standev / fabs (si->average));
      }
      else {
        SC_GLOBAL_LOGF (log_priority,
                        "   Mean value (std. dev.):         %g (%.3g)\n",
                        si->average, si->standev);
      }
      SC_GLOBAL_LOGF (log_priority, "   Minimum attained at rank %5d: %g\n",
                      si->min_at_rank, si->min);
      SC_GLOBAL_LOGF (log_priority, "   Maximum attained at rank %5d: %g\n",
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
        SC_GLOBAL_LOGF (log_priority,
                        "Mean value (std. dev.) %-28s %g (%.3g = %.3g%%)\n",
                        buffer, si->average, si->standev,
                        100. * si->standev / fabs (si->average));
      }
      else {
        SC_GLOBAL_LOGF (log_priority,
                        "Mean value (std. dev.) %-28s %g (%.3g)\n", buffer,
                        si->average, si->standev);
      }
    }
  }

  if (summary) {
    SC_GLOBAL_LOG (log_priority, "Summary = ");
    for (i = 0; i < nvars; ++i) {
      si = &stats[i];
      SC_GLOBAL_LOGF (log_priority, "%s%g", i == 0 ? "[ " : " ", si->average);
    }
    SC_GLOBAL_LOG (log_priority, " ];\n");
  }
}

void
sc_papi_start (float *irtime, float *iptime, long long *iflpops,
               float *imflops)
{
#ifdef SC_PAPI
  int                 retval;
  retval = PAPI_flops (irtime, iptime, iflpops, imflops);
  SC_CHECK_ABORT (retval == PAPI_OK, "Papi not happy");
#else
  *irtime = *iptime = *imflops = 0.;
  *iflpops = 0;
#endif
}

void
sc_flopinfo_start (sc_flopinfo_t * fi)
{
  fi->seconds = -MPI_Wtime ();

  sc_papi_start (&fi->irtime, &fi->iptime, &fi->iflpops, &fi->imflops);
}

void
sc_papi_stop (float *rtime, float *ptime, long long *flpops, float *mflops)
{
#ifdef SC_PAPI
  int                 retval;
  retval = PAPI_flops (rtime, ptime, flpops, mflops);
  SC_CHECK_ABORT (retval == PAPI_OK, "Papi not happy");
#else
  *rtime = *ptime = *mflops = 0.;
  *flpops = 0;
#endif
}

void
sc_flopinfo_stop (sc_flopinfo_t * fi)
{
  sc_papi_stop (&fi->rtime, &fi->ptime, &fi->flpops, &fi->mflops);
  fi->rtime -= fi->irtime;
  fi->ptime -= fi->iptime;
  fi->flpops -= fi->iflpops;

  fi->seconds += MPI_Wtime ();
}

/* EOF sc_statistics.c */
