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

#include <sc.h>
#include <stdio.h>
#include <string.h>

int
main (int argc, char **argv)
{
  int                 mpiret;
  sc_MPI_Comm         mpicomm;
  int                 num_failed_tests;
  int                 version_major, version_minor, version_point;
  const char         *version;
  char                version_tmp[32];

  /* standard initialization */
  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpicomm = sc_MPI_COMM_WORLD;

  sc_init (mpicomm, 1, 1, NULL, SC_LP_DEFAULT);

  /* check all functions related to version numbers of libsc */
  num_failed_tests = 0;
  version = sc_version ();
  SC_GLOBAL_LDEBUGF ("Full SC version: %s\n", version);

  version_major = sc_version_major ();
  SC_GLOBAL_LDEBUGF ("Major SC version: %d\n", version_major);
  snprintf (version_tmp, 32, "%d", version_major);
  if (strncmp (version, version_tmp, strlen (version_tmp))) {
    SC_VERBOSE ("Test failure for major version of SC\n");
    num_failed_tests++;
  }

  version_minor = sc_version_minor ();
  SC_GLOBAL_LDEBUGF ("Minor SC version: %d\n", version_minor);
  snprintf (version_tmp, 32, "%d.%d", version_major, version_minor);
  if (strncmp (version, version_tmp, strlen (version_tmp))) {
    SC_VERBOSE ("Test failure for minor version of SC\n");
    num_failed_tests++;
  }

  version_point = sc_version_point ();
  SC_GLOBAL_LDEBUGF ("Point SC version: %d\n", version_point);
  snprintf (version_tmp, 32, "%d.%d.%d", version_major, version_minor, version_point);
  if (strncmp (version, version_tmp, strlen (version_tmp))) {
    SC_VERBOSE ("Test failure for point version of SC\n");
    num_failed_tests++;
  }

  /* clean up and exit */
  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return num_failed_tests ? 1 : 0;
}
