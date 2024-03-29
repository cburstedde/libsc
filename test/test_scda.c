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

#include <sc_scda.h>

/* WARNING: this file contains obsolete references to be removed */

#if 0
#define SC_SCDA_FILE_EXT "scd"
#define SC_SCDA_TEST_FILE "sc_test_scda." SC_SCDA_FILE_EXT
#endif

int
main (int argc, char **argv)
{
  int                 mpiret;
#if 0
  int                 errcode;
  const char         *filename = SC_SCDA_TEST_FILE;
  char                read_user_string[SC_SCDA_USER_STRING_BYTES + 1];
  sc_scda_fcontext_t *fc;
#endif

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

#if 0
  fc =
    sc_scda_fopen_write (filename, sc_MPI_COMM_WORLD, "This is a test file",
                        &errcode);
  /* TODO: check errcode */

  sc_scda_fclose (fc, &errcode);

  fc =
    sc_scda_fopen_read (sc_MPI_COMM_WORLD, filename, read_user_string,
                       &errcode);

  sc_scda_fclose (fc, &errcode);
#endif

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
