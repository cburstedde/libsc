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
#include <sc_io.h>

/** The opaque file context for for scda files. */
struct sc_scda_fcontext
{
  sc_MPI_Comm         mpicomm;
                           /**< associated MPI communicator */
  int                 mpisize;
                           /**< number of MPI ranks */
  int                 mpirank;
                           /**< MPI rank */
  sc_MPI_File         file;/**< file object */
};

sc_scda_fcontext_t *
sc_scda_fopen_write (sc_MPI_Comm mpicomm,
                     const char *filename,
                     const char *user_string, size_t *len,
                     sc_scda_fopen_options_t * opt,
                     sc_scda_ferror_t * errcode)
{
  SC_ASSERT (filename != NULL);
  SC_ASSERT (user_string != NULL);
  SC_ASSERT (len != NULL);
  SC_ASSERT (errcode != NULL);

  int                 mpiret;
  sc_MPI_Info         info;
  sc_scda_fcontext_t *fc;

  /* TODO: check length of the filename */
  /* We assume the filename to be nul-terminated. */

  /* TODO: check the user string; implement a helper function for this */

  /* TODO: Check options if opt is valid? */

  /* allocate the file context */
  fc = SC_ALLOC (sc_scda_fcontext_t, 1);

  /* examine options */
  if (opt != NULL) {
    info = opt->info;
  }
  else {
    info = sc_MPI_INFO_NULL;
  }

  /* fill convience MPI information */
  mpiret = sc_MPI_Comm_size (mpicomm, &fc->mpisize);
  SC_CHECK_MPI (mpiret);

  mpiret = sc_MPI_Comm_rank (mpicomm, &fc->mpirank);
  SC_CHECK_MPI (mpiret);

  /* open the file for writing */
  mpiret =
    sc_io_open (mpicomm, filename, SC_IO_WRITE_CREATE, info, &fc->file);
  /* TODO: check return value */

  if (fc->mpirank == 0) {
    /* scda file header section */

  }

  return fc;
}

int
sc_scda_fclose (sc_scda_fcontext_t * fc, sc_scda_ferror_t * errcode)
{
  SC_ASSERT (fc != NULL);
  SC_ASSERT (errcode != NULL);

  int                 retval;

  /* TODO: further checks before calling sc_io_close? */

  retval = sc_io_close (&fc->file);
  /* TODO: handle return value */

  SC_FREE (fc);

  return 0;
}
