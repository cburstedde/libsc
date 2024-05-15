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
#include <sc_options.h>

#define SC_SCDA_FILE_EXT "scd"
#define SC_SCDA_TEST_FILE "sc_test_scda." SC_SCDA_FILE_EXT

int
main (int argc, char **argv)
{
  sc_MPI_Comm         mpicomm = sc_MPI_COMM_WORLD;
  int                 mpiret;
  int                 first_argc;
  const char         *filename = SC_SCDA_TEST_FILE;
  const char         *file_user_string = "This is a test file";
  char                read_user_string[SC_SCDA_USER_STRING_BYTES + 1];
  sc_scda_fcontext_t *fc;
  sc_scda_fopen_options_t scda_opt;
  sc_scda_ferror_t    errcode;
  size_t              len;
  sc_options_t       *opt;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  sc_init (mpicomm, 1, 1, NULL, SC_LP_INFO);

  /* parse command line options */
  opt = sc_options_new (argv[0]);

  sc_options_add_bool (opt, 'F', "fuzzy-error", &scda_opt.fuzzy_errors, 0,
                       "Boolean "
                       "switch for fuzzy error return of scda functions");
  sc_options_add_int (opt, 'S', "fuzzy-seed", &scda_opt.fuzzy_seed, -1,
                      "seed "
                      "for fuzzy error return of scda functions; ignored for "
                      "fuzzy-error false");
  sc_options_add_int (opt, 'E', "fuzzy-frequency", &scda_opt.fuzzy_freq, 3,
                      "fuzzy-frequency; ignored for fuzzy-error false");

  first_argc =
    sc_options_parse (sc_package_id, SC_LP_DEFAULT, opt, argc, argv);

  if (first_argc < 0 || first_argc != argc) {
    sc_options_print_usage (sc_package_id, SC_LP_ERROR, opt, NULL);
  }

  sc_options_print_summary (sc_package_id, SC_LP_PRODUCTION, opt);

  fc = sc_scda_fopen_write (mpicomm, filename, file_user_string, NULL,
                            NULL, &errcode);
  /* TODO: check errcode */
  SC_CHECK_ABORT (sc_scda_is_success (&errcode), "fopen_write failed");

  sc_scda_fclose (fc, &errcode);
  /* TODO: check errcode and return value */
  SC_CHECK_ABORT (sc_scda_is_success (&errcode), "fclose after write failed");

  /* set the options to actiavate fuzzy error testing */
  /* WARNING: Fuzzy error testing means that the code randomly produces
   * errors.
   */
  scda_opt.info = sc_MPI_INFO_NULL;

  fc =
    sc_scda_fopen_read (mpicomm, filename, read_user_string, &len, &scda_opt,
                        &errcode);
  /* TODO: check errcode */
  SC_CHECK_ABORT (sc_scda_is_success (&errcode), "fopen_read failed");

  SC_INFOF ("File header user string: %s\n", read_user_string);

  sc_scda_fclose (fc, &errcode);
  /* TODO: check errcode and return value */
  SC_CHECK_ABORT (sc_scda_is_success (&errcode), "fclose after read failed");

  sc_options_destroy (opt);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
