/*
  This file is part of the SC Library, version 3.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

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

#include <sc3_log.h>
#include <sc3_options.h>

typedef struct options_global
{
  sc3_allocator_t    *alloc;
  int                 stop;
  int                 help;
  int                 f1;
  int                 i1;
  int                 i2;
  double              dd;
  const char         *s1;
  const char         *s2;
}
options_global_t;

static sc3_error_t *
parse_options (options_global_t * g, int argc, char **argv)
{
  int                 pos;
  int                 res;
  sc3_options_t      *opt;

  /* construct options object */
  SC3E (sc3_options_new (g->alloc, &opt));
  SC3E (sc3_options_set_spacing (opt, 20));
  SC3E (sc3_options_set_stop (opt, &g->stop));
  SC3E (sc3_options_add_switch (opt, '?', "help", "Please help",
                                &g->help));
  SC3E (sc3_options_add_switch (opt, 'f', "flag", "Some flag",
                                &g->f1));
  SC3E (sc3_options_add_int (opt, 'i', "i-one", "First integer",
                             &g->i1, 6));
  SC3E (sc3_options_add_int (opt, 'j', NULL, "Second integer", &g->i2, 7));
  SC3E (sc3_options_add_double (opt, 'd', "number", "Real value", &g->dd, 9.18));
  SC3E (sc3_options_add_string (opt, 's', "string", "A string option",
                                &g->s1, NULL));
  SC3E (sc3_options_add_string (opt, '\0', "string2", NULL,
                                &g->s2, "String 2 default value"));
  SC3E (sc3_options_setup (opt));

  /* parse command line options */
  res = 0;
  for (pos = 1; pos < argc; ) {
    SC3E (sc3_options_parse (opt, argc, argv, &pos, &res));
    if (res < 0 || g->stop) {
      break;
    }
    if (res == 0) {
      /* in this example we allow arguments and options to mix */
      SC3_GLOBAL_PRODUCTIONF ("Argument at position %d: %s", pos, argv[pos]);
      ++pos;
    }
  }

  /* display summary and/or help */
  if (res < 0) {
    SC3_GLOBAL_ERRORF ("Option error at position %d", pos);
  }
  if (res < 0 || g->help) {
    SC3E (sc3_options_log_help (opt, NULL, SC3_LOG_ESSENTIAL));
  }
  else {
    for (; pos < argc; ++pos) {
      SC3_GLOBAL_PRODUCTIONF ("Argument at position %d: %s", pos, argv[pos]);
    }
    SC3E (sc3_options_log_summary (opt, NULL, SC3_LOG_ESSENTIAL));
  }

  SC3E (sc3_options_destroy (&opt));
  return NULL;
}

static sc3_error_t *
run_main (options_global_t * g, int argc, char **argv)
{
  SC3E (sc3_allocator_new (NULL, &g->alloc));
  SC3E (sc3_allocator_set_align (g->alloc, 0));
  SC3E (sc3_allocator_setup (g->alloc));

  SC3E (parse_options (g, argc, argv));

  SC3E (sc3_allocator_destroy (&g->alloc));
  return NULL;
}

int
main (int argc, char **argv)
{
  options_global_t global, *g = &global;
  SC3X (sc3_MPI_Init (&argc, &argv));
  SC3X (run_main (g, argc, argv));
  SC3X (sc3_MPI_Finalize ());
  return EXIT_SUCCESS;
}
