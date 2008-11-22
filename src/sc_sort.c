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
#include <sc_sort.h>

/** Sort a distributed set of values in parallel.
 * This algorithm uses bitonic sort between processors and qsort locally.
 * The partition of the data can be arbitrary and is not changed.
 * \param [in] mpicomm          Communicator to use.
 * \param [in] base             Pointer to the local subset of data.
 * \param [in] nmemb            Array of mpisize counts of local data.
 * \param [in] size             Size in bytes of each data value.
 * \param [in] compar           Comparison function to use.
 */
void
sc_psort (MPI_Comm mpicomm, void *base, size_t * nmemb, size_t size,
          int (*compar) (const void *, const void *))
{
  int                 mpiret;
  int                 rank, num_procs;

  /* get basic MPI information */
  mpiret = MPI_Comm_rank (mpicomm, &rank);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Comm_size (mpicomm, &num_procs);
  SC_CHECK_MPI (mpiret);
}

/* EOF sc_sort.c */
