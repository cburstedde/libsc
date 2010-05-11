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

#include <sc.h>

static void
log_normal (void)
{
  SC_TRACEF ("Trace normal %d\n", SC_LP_TRACE);
  SC_LDEBUG ("Debug normal\n");
  SC_VERBOSEF ("Verbose normal %d\n", SC_LP_VERBOSE);
  SC_INFO ("Info normal\n");
  SC_STATISTICSF ("Statistics normal %d\n", SC_LP_STATISTICS);
  SC_PRODUCTION ("Production normal\n");
  SC_ESSENTIAL ("Essential normal\n");
  SC_LERRORF ("Error normal %d\n", SC_LP_ERROR);
}

static void
log_global (void)
{
  SC_GLOBAL_TRACE ("Trace global\n");
  SC_GLOBAL_LDEBUGF ("Debug global %d\n", SC_LP_DEBUG);
  SC_GLOBAL_VERBOSE ("Verbose global\n");
  SC_GLOBAL_INFOF ("Info global %d\n", SC_LP_INFO);
  SC_GLOBAL_STATISTICS ("Statistics global\n");
  SC_GLOBAL_PRODUCTIONF ("Production global %d\n", SC_LP_PRODUCTION);
  SC_GLOBAL_ESSENTIALF ("Essential global %d\n", SC_LP_ESSENTIAL);
  SC_GLOBAL_LERROR ("Error global\n");
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

  sc_init (MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

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
