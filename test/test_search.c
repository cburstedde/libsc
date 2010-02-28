/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2010 Carsten Burstedde, Lucas Wilcox.

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

#include <sc_search.h>

int
main (int argc, char ** argv)
{
  int                 mpiret;
  int                 mpirank, mpisize;
  int                 maxlevel, level, target;
  int                 i, position;
  MPI_Comm            mpicomm;

  mpiret = MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  mpicomm = MPI_COMM_WORLD;
  mpiret = MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  if (mpirank == 0) {
    maxlevel = 3;
    target = 3;
    for (level = maxlevel; level >= 0; --level) {
      SC_LDEBUGF ("Level %d %d\n", maxlevel, level);

      for (i = 0; i < 1 << level; ++i) {

	position = sc_search_bias (maxlevel, level, i, target);
	SC_LDEBUGF ("Levels %d %d index %d target %d position %d\n",
		    maxlevel, level, i, target, position);
      }
    }
  }

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
