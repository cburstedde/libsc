/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2015 The University of Texas System

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

#include <sc.h>
#include <sc_mpi.h>
#include <sc_options.h>

int
main (int argc, char **argv)
{
  sc_options_t *opt;
  int           mpiret, node_size = 1;
  int           first;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  sc_init (MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

  opt = sc_options_new (argv[0]);
  sc_options_add_int (opt, 'n', "node-size", &node_size,
                      node_size, "simulated node size for intranode communicators");

  first = sc_options_parse (sc_package_id, SC_LP_INFO, opt, argc, argv);
  if (first < 0) {
    sc_options_print_usage (sc_package_id, SC_LP_INFO, opt, NULL);
    sc_abort_collective ("Usage error");
  }
  sc_options_destroy (opt);

  sc_mpi_comm_attach_node_comms (MPI_COMM_WORLD,node_size);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);
  return 0;
}
