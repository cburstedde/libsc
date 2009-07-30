/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2008,2009 Carsten Burstedde, Lucas Wilcox.

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

#include <sc.h>

static void
log_normal (void)
{
  SC_TRACE ("Trace normal\n");
  SC_LDEBUG ("Debug normal\n");
  SC_VERBOSE ("Verbose normal\n");
  SC_INFO ("Info normal\n");
  SC_STATISTICS ("Statistics normal\n");
  SC_PRODUCTION ("Production normal\n");
}

static void
log_global (void)
{
  SC_GLOBAL_TRACE ("Trace global\n");
  SC_GLOBAL_LDEBUG ("Debug global\n");
  SC_GLOBAL_VERBOSE ("Verbose global\n");
  SC_GLOBAL_INFO ("Info global\n");
  SC_GLOBAL_STATISTICS ("Statistics global\n");
  SC_GLOBAL_PRODUCTION ("Production global\n");
}

int
main (int argc, char **argv)
{
  int                 mpiret;

  mpiret = MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  log_normal ();
  log_global ();

  sc_set_log_defaults (stdout, NULL, SC_LP_VERBOSE);
  log_normal ();

  sc_init (MPI_COMM_WORLD, true, true, NULL, SC_LP_DEFAULT);

  sc_package_print_summary (SC_LP_PRODUCTION);

  sc_set_log_defaults (stderr, NULL, SC_LP_STATISTICS);
  log_normal ();
  log_global ();

  sc_set_log_defaults (stdout, NULL, SC_LP_TRACE);
  log_normal ();

  sc_set_log_defaults (NULL, NULL, SC_LP_TRACE);
  log_global ();

  sc_set_log_defaults (stderr, NULL, SC_LP_SILENT);
  log_normal ();

  sc_finalize ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
