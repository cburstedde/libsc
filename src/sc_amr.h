/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

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

#ifndef SC_AMR_H
#define SC_AMR_H

#include <sc_statistics.h>

typedef struct sc_amr_control
{
  const double       *errors;
  sc_statinfo_t       estats;
  MPI_Comm            mpicomm;
  long                num_procs_long;
  long                num_total_elements;
  double              coarsen_threshold;
  double              refine_threshold;
  long                num_total_coarsen;
  long                num_total_refine;
  long                num_total_estimated;
}
sc_amr_control_t;

/** Compute global error statistics.
 * \param [in] mpicomm        MPI communicator to use.
 * \param [in] mpisize        Number of MPI processes in this communicator.
 * \param [in] num_local_elements   Number of local elements.
 * \param [in] errors         The error values, one per local element.
 * \param [out] amr           Structure will be initialized and estats filled.
 */
void                sc_amr_error_stats (MPI_Comm mpicomm,
                                        long num_local_elements,
                                        const double *errors,
                                        sc_amr_control_t * amr);

/** Count the local number of elements that will be coarsened.
 *
 * This is all elements whose error is below threshold
 * and where there are no other conditions preventing coarsening
 * (such as not all siblings may be coarsened or are on another processor).
 *
 * \return          Returns the net loss of local elements.
 */
typedef long        (*sc_amr_count_coarsen_fn) (sc_amr_control_t * amr,
                                                void *user_data);

/** Count the local number of elements that will be refined.
 *
 * This is all elements whose error is above the threshold
 * and where there are no other conditions preventing refinement
 * (such as refinement would not make the element too small).
 *
 * \return          Returns the net gain of local elements.
 */
typedef long        (*sc_amr_count_refine_fn) (sc_amr_control_t * amr,
                                               void *user_data);

/** Binary search for coarsening threshold without refinement.
 *
 * \param [in,out] amr           AMR control structure.
 * \param [in] cfn               Callback to count local coarsenings.
 * \param [in] num_local_ideal   Target number of local elements.
 * \param [in] binary_fudge      Target tolerance < 1.
 * \param [in] max_binary_steps  Upper bound on binary search steps.
 * \param [in] user_data         Will be passed to the cfn callback.
 */
void                sc_amr_coarsen_threshold (sc_amr_control_t * amr,
                                              sc_amr_count_coarsen_fn cfn,
                                              long num_local_ideal,
                                              double binary_fudge,
                                              int max_binary_steps,
                                              void *user_data);

#endif /* !SC_AMR_H */
