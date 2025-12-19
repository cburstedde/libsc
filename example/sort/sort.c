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

/*
 * Demo program to exercise various sorting algorithms
 */

#include <sc_options.h>
#include <sc_random.h>

typedef struct sc_sort
{
  int                 mpirank;
  int                 mpisize;
  int                 n;
}
sc_sort_t;

static void
run_sort (sc_sort_t *s)
{
}

int
main (int argc, char **argv)
{
  sc_sort_t           sort, *s = &sort;
  int                 fail = 0;
  int                 mpiret;
  int                 help;
  int                 first_arg;
  sc_options_t       *opt;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

  mpiret = sc_MPI_Comm_size (sc_MPI_COMM_WORLD, &s->mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (sc_MPI_COMM_WORLD, &s->mpirank);
  SC_CHECK_MPI (mpiret);

  opt = sc_options_new (argv[0]);
  sc_options_add_int (opt, 'n', NULL, &s->n, 0, "Total number of items");
  sc_options_add_switch (opt, 'h', "help", &help, "Print help information");

  /* process command line options */
  first_arg = sc_options_parse (sc_package_id, SC_LP_INFO, opt, argc, argv);
  if (!fail && first_arg < 0) {
    SC_GLOBAL_LERROR ("Error in option parsing\n");
    fail = 1;
  }
  if (!fail && first_arg < argc) {
    SC_GLOBAL_LERROR ("This program takes no arguments, just options\n");
    fail = 1;
  }
  if (!fail && s->n < 0) {
    SC_GLOBAL_LERROR ("Parameter n must be non-negative\n");
    fail = 1;
  }

  /* execute main program action */
  if (fail) {
    sc_options_print_usage (sc_package_id, SC_LP_ERROR, opt, NULL);
  }
  else {
    if (help) {
      sc_options_print_usage (sc_package_id, SC_LP_PRODUCTION, opt, NULL);
    }
    else {
      sc_options_print_summary (sc_package_id, SC_LP_PRODUCTION, opt);
      run_sort (s);
    }
  }

  sc_options_destroy (opt);
  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
