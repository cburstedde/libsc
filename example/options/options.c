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

#include <sc_options.h>

/* quick and dirty -- please use callback's data argument instead */
static int          w;

static int
callback (sc_options_t * opt, const char *theoptarg, void *data)
{
  if (theoptarg == NULL) {
    SC_GLOBAL_INFOF ("%s without argument\n", (const char *) data);
  }
  else {
    SC_GLOBAL_INFOF ("%s with %s\n", (const char *) data, theoptarg);
  }
  ++w;

  return 0;
}

int
main (int argc, char **argv)
{
  int                 mpiret, retval;
  int                 rank;
  int                 first_arg;
  int                 i1, i2, si1;
  int                 kvint;
  size_t              z;
  double              d, sd;
  const char         *s1, *s2, *ss1, *ss2;
  const char         *cd = "Callback example";
  sc_keyvalue_t      *keyvalue;
  sc_options_t       *opt, *subopt;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  mpiret = sc_MPI_Comm_rank (sc_MPI_COMM_WORLD, &rank);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

  keyvalue = sc_keyvalue_new ();
  sc_keyvalue_set_int (keyvalue, "one", 1);
  sc_keyvalue_set_int (keyvalue, "two", 2);

  opt = sc_options_new (argv[0]);
  sc_options_add_switch (opt, 'w', "switch", &w, "Switch");
  sc_options_add_int (opt, 'i', "integer1", &i1, 0, "Integer 1");
  sc_options_add_double (opt, 'd', "double", &d, 0., "Double");
  sc_options_add_string (opt, 's', "string", &s1, NULL, "String 1");
  sc_options_add_callback (opt, 'c', "call1", 1, callback, (void *) cd,
                           "Callback 1");
  sc_options_add_callback (opt, 'C', "call2", 0, callback, (void *) cd,
                           "Callback 2");
  sc_options_add_string (opt, 't', NULL, &s2, NULL, "String 2");
  sc_options_add_inifile (opt, 'f', "inifile", ".ini file");
  sc_options_add_int (opt, '\0', "integer2", &i2, 7, "Integer 2");
  sc_options_add_size_t (opt, 'z', "sizet", &z, (size_t) 7000000000ULL,
                         "Size_t");

  subopt = sc_options_new (argv[0]);
  sc_options_add_int (subopt, 'i', "integer", &si1, 0, "Subset integer");
  sc_options_add_double (subopt, 'd', "double", &sd, 0., "Subset double");
  sc_options_add_string (subopt, 's', NULL, &ss1, NULL, "Subset string 1");
  sc_options_add_string (subopt, '\0', "string2", &ss2, NULL,
                         "Subset string 1");
  sc_options_add_keyvalue (subopt, 'n', "number", &kvint, "one",
                           keyvalue, "Subset keyvalue number");

  sc_options_add_suboptions (opt, subopt, "Subset");

  /* this is just to show off the load function */
  if (!sc_options_load (sc_package_id, SC_LP_INFO, opt,
                        "sc_options_preload.ini")) {
    SC_GLOBAL_INFO ("Preload successful\n");
  }
  else {
    SC_GLOBAL_INFO ("Preload not found or failed\n");
  }

  first_arg = sc_options_parse (sc_package_id, SC_LP_INFO, opt, argc, argv);
  if (first_arg < 0) {
    sc_options_print_usage (sc_package_id, SC_LP_INFO, opt,
                            "Usage for arg 1\nand for arg 2");
    SC_GLOBAL_INFO ("Option parsing failed\n");
  }
  else {
    SC_GLOBAL_INFO ("Option parsing successful\n");
    sc_options_print_summary (sc_package_id, SC_LP_INFO, opt);
    SC_GLOBAL_INFOF ("Keyvalue number is now %d\n", kvint);

    if (rank == 0) {
      retval = sc_options_save (sc_package_id, SC_LP_INFO, opt, "output.ini");
      if (retval) {
        SC_GLOBAL_INFO ("Option file output failed\n");
      }
      else {
        retval = sc_options_load_args (sc_package_id, SC_LP_INFO, opt,
                                       "output.ini");
        if (retval) {
          SC_GLOBAL_INFO ("Argument file input failed\n");
        }
        else {
          sc_options_print_summary (sc_package_id, SC_LP_INFO, opt);
          SC_GLOBAL_INFO ("Argument save load successful\n");
        }
      }
    }
  }

  sc_options_destroy (opt);
  sc_options_destroy (subopt);
  sc_keyvalue_destroy (keyvalue);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
