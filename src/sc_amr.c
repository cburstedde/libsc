/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2007,2008 Carsten Burstedde, Lucas Wilcox.

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
#include <sc_amr.h>

void
sc_amr_error_stats (MPI_Comm mpicomm, long num_elements,
                    const double *errors, sc_amr_control_t * amr)
{
  sc_statinfo_t      *si = &amr->estats;
  int                 mpiret;
  int                 mpisize;
  long                i;
  double              sum, squares, emin, emax;

  mpiret = MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);

  amr->errors = errors;

  sum = squares = 0.;
  emin = DBL_MAX;
  emax = -DBL_MAX;
  for (i = 0; i < num_elements; ++i) {
    sum += errors[i];
    squares += errors[i] * errors[i];
    emin = SC_MIN (emin, errors[i]);
    emax = SC_MAX (emax, errors[i]);
  }
  si->count = num_elements;
  si->sum_values = sum;
  si->sum_squares = squares;
  si->min = emin;
  si->max = emax;
  si->variable = NULL;
  sc_statinfo_compute (mpicomm, 1, si);

  amr->mpicomm = mpicomm;
  amr->num_procs_long = (long) mpisize;
  amr->num_total_estimated = amr->num_total_elements = si->count;
  amr->coarsen_threshold = si->min;
  amr->refine_threshold = si->max;
  amr->num_total_coarsen = amr->num_total_refine = 0;
}

void
sc_amr_coarsen_specify (sc_amr_control_t * amr, double coarsen_threshold,
                        sc_amr_count_coarsen_fn cfn, void * user_data)
{
  int                 mpiret;
  long                local_coarsen, global_coarsen;

  if (cfn == NULL) {
    local_coarsen = global_coarsen = 0;
    amr->coarsen_threshold = amr->estats.min;
  }
  else {
    SC_GLOBAL_STATISTICSF ("Set coarsen threshold %.3g"
                           " assuming %ld refinements\n",
                           coarsen_threshold, amr->num_total_refine);

    local_coarsen = cfn (amr, user_data);
    mpiret = MPI_Allreduce (&local_coarsen, &global_coarsen, 1, MPI_LONG,
                            MPI_SUM, amr->mpicomm);
    SC_CHECK_MPI (mpiret);
    amr->coarsen_threshold = coarsen_threshold;
  }

  amr->num_total_coarsen = global_coarsen;
  amr->num_total_estimated =
    amr->num_total_elements + amr->num_total_refine - global_coarsen;

  SC_GLOBAL_INFOF ("Global number of coarsenings = %ld\n",
                   amr->num_total_coarsen);
}

void
sc_amr_coarsen_search (sc_amr_control_t * amr,
                       double coarsen_ratio, double coarsen_threshold_high,
                       double target_window, int max_binary_steps,
                       sc_amr_count_coarsen_fn cfn, void *user_data)
{
  const sc_statinfo_t *errors = &amr->estats;
  const long          num_total_elements = amr->num_total_elements;
  const long          num_total_refine = amr->num_total_refine;
  int                 mpiret;
  int                 binary_count;
  long                local_coarsen, global_coarsen;
  long                num_total_low, num_total_high, num_total_estimated;
  double              coarsen_threshold_low;

  num_total_low = (long) (num_total_elements * (1. - coarsen_ratio));

  SC_GLOBAL_STATISTICSF ("Search for coarsen threshold"
                         " assuming %ld refinements\n", num_total_refine);

  /* assign initial threshold range and check */
  coarsen_threshold_low = errors->min;
  if (cfn == NULL ||
      coarsen_threshold_low >= coarsen_threshold_high ||
      num_total_elements + num_total_refine <= num_total_low) {

    SC_GLOBAL_STATISTICSF ("Search for coarsening skipped with"
                           " low = %.3g, up = %.3g\n",
                           coarsen_threshold_low, coarsen_threshold_high);

    amr->coarsen_threshold = errors->min;
    amr->num_total_coarsen = 0;
    amr->num_total_estimated = num_total_elements + num_total_refine;
    return;
  }

  /* fix range of acceptable total element counts */
  num_total_high = (long) (num_total_low / target_window);
  SC_GLOBAL_INFOF ("Range of acceptable total element counts %ld %ld\n",
                   num_total_low, num_total_high);

  /* start binary search at the upper end */
  amr->coarsen_threshold = coarsen_threshold_high;
  for (binary_count = 0;; ++binary_count) {

    /* call back to count the elements to coarsen locally */
    local_coarsen = cfn (amr, user_data);
    mpiret = MPI_Allreduce (&local_coarsen, &global_coarsen, 1, MPI_LONG,
                            MPI_SUM, amr->mpicomm);
    SC_CHECK_MPI (mpiret);
    num_total_estimated =
      num_total_elements + num_total_refine - global_coarsen;
    SC_GLOBAL_LDEBUGF ("At %.3g total %ld estimated %ld coarsen %ld\n",
                       amr->coarsen_threshold, num_total_elements,
                       num_total_estimated, global_coarsen);

    /* check loop condition */
    if (binary_count == max_binary_steps) {
      break;
    }

    /* binary search action */
    if (num_total_estimated < num_total_low) {
      coarsen_threshold_high = amr->coarsen_threshold;
    }
    else if (num_total_estimated > num_total_high) {
      if (binary_count == 0) {
        break;                  /* impossible to coarsen more */
      }
      coarsen_threshold_low = amr->coarsen_threshold;
    }
    else {                      /* binary search sucessful */
      break;
    }
    SC_GLOBAL_INFOF ("Binary search for %ld elements"
                     " at low = %.3g, up = %.3g\n", num_total_estimated,
                     coarsen_threshold_low, coarsen_threshold_high);

    /* compute next guess for binary search */
    amr->coarsen_threshold =
      (coarsen_threshold_low + coarsen_threshold_high) / 2.;
  }
  amr->num_total_coarsen = global_coarsen;
  amr->num_total_estimated = num_total_estimated;

  /* binary search is ended */
  SC_GLOBAL_INFOF ("Search for coarsen stopped after %d steps"
                   " with threshold %.3g\n",
                   binary_count, amr->coarsen_threshold);
  SC_GLOBAL_INFOF ("Global number of coarsenings = %ld\n",
                   amr->num_total_coarsen);
  SC_GLOBAL_INFOF ("Estimated global number of elements = %ld\n",
                   amr->num_total_estimated);
}

void
sc_amr_refine_threshold (sc_amr_control_t * amr,
                         sc_amr_count_refine_fn rfn,
                         long num_local_ideal,
                         double binary_fudge,
                         int max_binary_steps, void *user_data)
{
  const sc_statinfo_t *errors = &amr->estats;
  const long          num_total_elements = amr->num_total_elements;
  const long          num_total_coarsen = amr->num_total_coarsen;
  int                 mpiret;
  int                 binary_count;
  long                local_refine, global_refine;
  long                num_total_low, num_total_high, num_total_estimated;
  double              refine_threshold_low, refine_threshold_high;

  SC_ASSERT (binary_fudge < 1.);
  SC_GLOBAL_STATISTICSF ("Search for refine threshold"
                         " assuming %ld coarsenings\n", num_total_coarsen);

  /* assign initial threshold range */
  num_total_high = num_local_ideal * amr->num_procs_long;
  refine_threshold_low = errors->min;
  refine_threshold_high = errors->max;
  if (refine_threshold_low >= refine_threshold_high ||
      num_total_elements - num_total_coarsen >= num_total_high) {

    SC_GLOBAL_STATISTICSF ("Binary search anomaly"
                           " with low = %.3g, up = %.3g\n",
                           refine_threshold_low, refine_threshold_high);

    amr->refine_threshold = errors->max;
    amr->num_total_estimated = num_total_elements - num_total_coarsen;
    return;
  }

  /* fix range of acceptable total element counts */
  num_total_low = (long) (num_total_high * binary_fudge);
  SC_GLOBAL_INFOF ("Range of acceptable total element counts %ld %ld\n",
                   num_total_low, num_total_high);

  for (binary_count = 0; binary_count < max_binary_steps; ++binary_count) {

    /* estimate expected number of elements */
    amr->refine_threshold =
      (refine_threshold_low + refine_threshold_high) / 2.;
    local_refine = rfn (amr, user_data);
    mpiret = MPI_Allreduce (&local_refine, &global_refine, 1, MPI_LONG,
                            MPI_SUM, amr->mpicomm);
    SC_CHECK_MPI (mpiret);
    num_total_estimated =
      num_total_elements + global_refine - num_total_coarsen;

    /* print status message */
    SC_GLOBAL_LDEBUGF ("Total %ld estimated %ld refine %ld\n",
                       num_total_elements, num_total_estimated,
                       global_refine);
    SC_GLOBAL_INFOF ("Binary search for %ld elements"
                     " at low = %.3g, up = %.3g\n", num_total_estimated,
                     refine_threshold_low, refine_threshold_high);

    /* binary search action */
    if (num_total_estimated < num_total_low) {
      refine_threshold_high = amr->refine_threshold;
    }
    else if (num_total_estimated > num_total_high) {
      refine_threshold_low = amr->refine_threshold;
    }
    else {                      /* binary search sucessful */
      break;
    }
  }

  /* binary search is ended */
  SC_GLOBAL_INFOF ("Binary search for refine stopped after %d steps"
                   " with threshold %.3g\n",
                   binary_count, amr->refine_threshold);
  SC_GLOBAL_INFOF ("Global number of refinements = %ld\n", global_refine);
  SC_GLOBAL_INFOF ("Estimated global number of elements = %ld\n",
                   num_total_estimated);

  amr->num_total_estimated = num_total_estimated;
}

/* EOF sc_amr.c */
