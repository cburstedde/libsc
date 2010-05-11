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

#include <sc_warp.h>

int
main (void)
{
  double              round1[3];
  double              round2[3];
  sc_warp_interval_t *root;

  sc_init (MPI_COMM_NULL, 1, 1, NULL, SC_LP_DEFAULT);

  root = sc_warp_new (0., 1.);

  round1[0] = .3;
  round1[1] = .58;
  round1[2] = .86;
  sc_warp_update (root, 3, round1, 0.10, 7);
  sc_warp_write (root, stdout);

  round2[0] = .3;
  round2[1] = .86;
  round2[2] = .92;
  sc_warp_update (root, 3, round2, 0.15, 7);
  sc_warp_write (root, stdout);

  sc_warp_destroy (root);

  sc_finalize ();

  return 0;
}
