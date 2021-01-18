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

#include <sc_amr.h>

void
sc_amr_error_stats (sc_MPI_Comm mpicomm, long num_elements,
                    const double *errors, sc_amr_control_t * amr)
{
  sc_statinfo_t      *si = &amr->estats;
  int                 mpiret;
  int                 mpisize;
  long                i;
  double              sum, squares, emin, emax;

  mpiret = sc_MPI_Comm_size (mpicomm, &mpisize);
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
  si->dirty = 1;
  si->count = num_elements;
  si->sum_values = sum;
  si->sum_squares = squares;
  si->min = emin;
  si->max = emax;
  si->variable = NULL;
  sc_stats_compute (mpicomm, 1, si);

  amr->mpicomm = mpicomm;
  amr->num_procs_long = (long) mpisize;
  amr->num_total_estimated = amr->num_total_elements = si->count;
  amr->coarsen_threshold = si->min;
  amr->refine_threshold = si->max;
  amr->num_total_coarsen = amr->num_total_refine = 0;
}

void
sc_amr_coarsen_specify (int package_id,
                        sc_amr_control_t * amr, double coarsen_threshold,
                        sc_amr_count_coarsen_fn cfn, void *user_data)
{
  int                 mpiret;
  long                local_coarsen, global_coarsen;

  if (cfn == NULL) {
    amr->coarsen_threshold = amr->estats.min;
    local_coarsen = global_coarsen = 0;
  }
  else {
    amr->coarsen_threshold = coarsen_threshold;
    SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_STATISTICS,
                 "Set coarsen threshold %g assuming %ld refinements\n",
                 amr->coarsen_threshold, amr->num_total_refine);

    local_coarsen = cfn (amr, user_data);
    mpiret = sc_MPI_Allreduce (&local_coarsen, &global_coarsen, 1,
                               sc_MPI_LONG, sc_MPI_SUM, amr->mpicomm);
    SC_CHECK_MPI (mpiret);
  }

  amr->num_total_coarsen = global_coarsen;
  amr->num_total_estimated =
    amr->num_total_elements + amr->num_total_refine - global_coarsen;

  SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_STATISTICS,
               "Global number of coarsenings = %ld\n",
               amr->num_total_coarsen);
}

void
sc_amr_coarsen_search (int package_id, sc_amr_control_t * amr,
                       long num_total_low, double coarsen_threshold_high,
                       double target_window, int max_binary_steps,
                       sc_amr_count_coarsen_fn cfn, void *user_data)
{
  const sc_statinfo_t *errors = &amr->estats;
  const long          num_total_elements = amr->num_total_elements;
  const long          num_total_refine = amr->num_total_refine;
  int                 mpiret;
  int                 binary_count;
  long                local_coarsen, global_coarsen;
  long                num_total_high, num_total_estimated;
  double              coarsen_threshold_low;

  SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_STATISTICS,
               "Search for coarsen threshold assuming %ld refinements\n",
               num_total_refine);

  /* assign initial threshold range and check */
  coarsen_threshold_low = errors->min;
  if (cfn == NULL ||
      coarsen_threshold_low >= coarsen_threshold_high ||
      num_total_elements + num_total_refine <= num_total_low) {

    SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_STATISTICS,
                 "Search for coarsening skipped with low = %g, up = %g\n",
                 coarsen_threshold_low, coarsen_threshold_high);

    amr->coarsen_threshold = errors->min;
    amr->num_total_coarsen = 0;
    amr->num_total_estimated = num_total_elements + num_total_refine;
    return;
  }

  /* fix range of acceptable total element counts */
  num_total_high = (long) (num_total_low / target_window);
  SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_INFO,
               "Range of acceptable total element counts %ld %ld\n",
               num_total_low, num_total_high);

  /* start binary search at the upper end */
  amr->coarsen_threshold = coarsen_threshold_high;
  for (binary_count = 0;; ++binary_count) {

    /* call back to count the elements to coarsen locally */
    local_coarsen = cfn (amr, user_data);
    mpiret = sc_MPI_Allreduce (&local_coarsen, &global_coarsen, 1,
                               sc_MPI_LONG, sc_MPI_SUM, amr->mpicomm);
    SC_CHECK_MPI (mpiret);
    num_total_estimated =
      num_total_elements + num_total_refine - global_coarsen;
    SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_STATISTICS,
                 "At %g total %ld estimated %ld coarsen %ld\n",
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
    else {                      /* binary search successful */
      break;
    }
    SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_STATISTICS,
                 "Binary search for %ld elements at low = %g, up = %g\n",
                 num_total_low, coarsen_threshold_low,
                 coarsen_threshold_high);

    /* compute next guess for binary search */
    amr->coarsen_threshold =
      (coarsen_threshold_low + coarsen_threshold_high) / 2.;
  }
  amr->num_total_coarsen = global_coarsen;
  amr->num_total_estimated = num_total_estimated;

  /* binary search is ended */
  SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_STATISTICS,
               "Search for coarsen stopped after %d steps with threshold %g\n",
               binary_count, amr->coarsen_threshold);
  SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_STATISTICS,
               "Global number of coarsenings = %ld\n",
               amr->num_total_coarsen);
  SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_INFO,
               "Estimated global number of elements = %ld\n",
               amr->num_total_estimated);
}

void
sc_amr_refine_search (int package_id, sc_amr_control_t * amr,
                      long num_total_high, double refine_threshold_low,
                      double target_window, int max_binary_steps,
                      sc_amr_count_refine_fn rfn, void *user_data)
{
  const sc_statinfo_t *errors = &amr->estats;
  const long          num_total_elements = amr->num_total_elements;
  const long          num_total_coarsen = amr->num_total_coarsen;
  int                 mpiret;
  int                 binary_count;
  long                local_refine, global_refine;
  long                num_total_low, num_total_estimated;
  double              refine_threshold_high;

  SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_STATISTICS,
               "Search for refine threshold assuming %ld coarsenings\n",
               num_total_coarsen);

  /* assign initial threshold range and check */
  refine_threshold_high = errors->max;
  if (rfn == NULL ||
      refine_threshold_low >= refine_threshold_high ||
      num_total_elements - num_total_coarsen >= num_total_high) {

    SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_STATISTICS,
                 "Search for refinement skipped with low = %g, up = %g\n",
                 refine_threshold_low, refine_threshold_high);

    amr->refine_threshold = errors->max;
    amr->num_total_refine = 0;
    amr->num_total_estimated = num_total_elements - num_total_coarsen;
    return;
  }

  /* fix range of acceptable total element counts */
  num_total_low = (long) (num_total_high * target_window);
  SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_INFO,
               "Range of acceptable total element counts %ld %ld\n",
               num_total_low, num_total_high);

  /* start binary search at the lower end */
  amr->refine_threshold = refine_threshold_low;
  for (binary_count = 0;; ++binary_count) {

    /* call back to count the elements to refine locally */
    local_refine = rfn (amr, user_data);
    mpiret = sc_MPI_Allreduce (&local_refine, &global_refine, 1, sc_MPI_LONG,
                               sc_MPI_SUM, amr->mpicomm);
    SC_CHECK_MPI (mpiret);
    num_total_estimated =
      num_total_elements + global_refine - num_total_coarsen;
    SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_STATISTICS,
                 "At %g total %ld estimated %ld refine %ld\n",
                 amr->refine_threshold, num_total_elements,
                 num_total_estimated, global_refine);

    /* check loop condition */
    if (binary_count == max_binary_steps) {
      break;
    }

    /* binary search action */
    if (num_total_estimated < num_total_low) {
      if (binary_count == 0) {
        break;                  /* impossible to refine more */
      }
      refine_threshold_high = amr->refine_threshold;
    }
    else if (num_total_estimated > num_total_high) {
      refine_threshold_low = amr->refine_threshold;
    }
    else {                      /* binary search successful */
      break;
    }
    SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_STATISTICS,
                 "Binary search for %ld elements at low = %g, up = %g\n",
                 num_total_high, refine_threshold_low, refine_threshold_high);

    /* compute next guess for binary search */
    amr->refine_threshold =
      (refine_threshold_low + refine_threshold_high) / 2.;
  }
  amr->num_total_refine = global_refine;
  amr->num_total_estimated = num_total_estimated;

  /* binary search is ended */
  SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_STATISTICS,
               "Search for refine stopped after %d steps with threshold %g\n",
               binary_count, amr->refine_threshold);
  SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_STATISTICS,
               "Global number of refinements = %ld\n", amr->num_total_refine);
  SC_GEN_LOGF (package_id, SC_LC_GLOBAL, SC_LP_INFO,
               "Estimated global number of elements = %ld\n",
               amr->num_total_estimated);
}
