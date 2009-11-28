/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2009 Carsten Burstedde, Lucas Wilcox.

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

#include <sc_warp.h>

int
main (void)
{
  double              round1[3];
  sc_warp_interval_t *root;

  sc_init (MPI_COMM_NULL, 1, 1, NULL, SC_LP_DEFAULT);

  root = sc_warp_new (0., 1.);

  round1[0] = .3;
  round1[1] = .58;
  round1[2] = .86;
  sc_warp_update (root, 3, round1, 0.15, 7);
  sc_warp_update (root, 3, round1, 0.20, 7);

  sc_warp_destroy (root);

  sc_finalize ();

  return 0;
}
