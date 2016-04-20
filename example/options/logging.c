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

#include <sc_options.h>

static void
run_program (void)
{
  SC_LDEBUG ("Debug\n");
  SC_INFO ("Info\n");
  SC_GLOBAL_INFO ("Info\n");
  SC_GLOBAL_PRODUCTION ("Production\n");
  SC_GLOBAL_ESSENTIAL ("Essential\n");
}

int
main (int argc, char **argv)
{
  int                 mpiret;
  int                 first_arg;
  int                 verbosity;
  sc_keyvalue_t      *priorities;
  sc_options_t       *opt;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

  /* initialize key-value structure for option parsing */
  priorities = sc_keyvalue_new ();
  sc_keyvalue_set_int (priorities, "default", SC_LP_DEFAULT);
  sc_keyvalue_set_int (priorities, "debug", SC_LP_DEBUG);
  sc_keyvalue_set_int (priorities, "informative", SC_LP_INFO);
  sc_keyvalue_set_int (priorities, "production", SC_LP_PRODUCTION);
  sc_keyvalue_set_int (priorities, "essential", SC_LP_ESSENTIAL);
  sc_keyvalue_set_int (priorities, "silent", SC_LP_SILENT);

  /* initialize option structure */
  opt = sc_options_new (argv[0]);
  sc_options_add_keyvalue (opt, 'V', "verbosity", &verbosity, "default",
                           priorities, "Choose the log level");

  /* parse command line options */
  first_arg = sc_options_parse (sc_package_id, SC_LP_ERROR, opt, argc, argv);
  if (first_arg < 0) {
    SC_GLOBAL_LERROR ("Option parsing failed\n");
  }
  else {
    SC_GLOBAL_INFO ("Option parsing successful\n");
    sc_options_print_summary (sc_package_id, SC_LP_PRODUCTION, opt);

    /* set verbosity level */
    sc_package_set_verbosity (sc_package_id, verbosity);

    /* go to work */
    run_program ();
  }

  /* cleanup */
  sc_options_destroy (opt);
  sc_keyvalue_destroy (priorities);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
