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

#include <sc_v4l2.h>

static void
v4l2_run (const char *devname)
{
  int                 retval;
  sc_v4l2_device_t   *vd;

  vd = sc_v4l2_device_open (devname);
  SC_CHECK_ABORTF (vd != NULL, "Failed to open device %s", devname);

  fprintf (stderr, "%s\n", sc_v4l2_device_devstring (vd));
  fprintf (stderr, "%s\n", sc_v4l2_device_capstring (vd));

  retval = sc_v4l2_device_close (vd);
  SC_CHECK_ABORTF (!retval, "Failed to close device %s", devname);
}

int
main (int argc, char **argv)
{
  int                 mpiret;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

  if (argc >= 2) {
    v4l2_run (argv[1]);
  }

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
