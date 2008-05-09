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
#include <sc_options.h>

int
main (int argc, char **argv)
{
  int                 mpiret;
  int                 rank;
  int                 first_arg;
  int                 w;
  int                 i1, i2;
  double              d;
  const char         *s1, *s2;
  sc_options_t       *opt;

  mpiret = MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  mpiret = MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  SC_CHECK_MPI (mpiret);

  sc_log_init (stdout, rank);

  opt = sc_options_new (argv[0]);
  sc_options_add_switch (opt, 'w', "switch", &w, "Switch");
  sc_options_add_int (opt, 'i', "integer1", &i1, 0, "Integer 1");
  sc_options_add_double (opt, 'd', "double", &d, 0., "Double");
  sc_options_add_string (opt, 's', "string", &s1, NULL, "String 1");
  sc_options_add_string (opt, 't', NULL, &s2, NULL, "String 2");
  sc_options_add_inifile (opt, 'f', "inifile", ".ini file");
  sc_options_add_int (opt, '\0', "integer2", &i2, 7, "Integer 2");

  /* this is just to show off the load function */
  if (!sc_options_load (opt, "preload.ini", NULL)) {
    SC_GLOBAL_INFO ("Preload successful\n");
  }
  else {
    SC_GLOBAL_INFO ("Preload not found or failed\n");
  }

  first_arg = sc_options_parse (opt, argc, argv, sc_root_stderr);
  if (rank == 0) {
    if (first_arg < 0) {
      sc_options_print_help (opt, 1, stdout);
    }
    else {
      sc_options_print_summary (opt, stdout);
      sc_options_print_arguments (opt, first_arg, argc, argv, stdout);
    }
  }

  sc_options_destroy (opt);

  sc_memory_check ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}

/* EOF options.c */
