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
  int                 i1;
  int                 i2;
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
  SC3E (sc3_options_set_stop (opt, &g->stop));
  SC3E (sc3_options_add_int (opt, 'i', "--i-one", "First integer",
                             &g->i1, 6));
  SC3E (sc3_options_add_int (opt, 'j', NULL, "Second integer", &g->i2, 7));
  SC3E (sc3_options_setup (opt));

  /* parse command line options */
  res = 0;
  for (pos = 1; pos < argc; ++pos) {
    SC3E (sc3_options_parse (opt, argc, argv, &pos, &res));
    if (res <= 0 || g->stop) {
      break;
    }
  }
  SC3_GLOBAL_PRODUCTIONF ("Parsed with last result %d", res);

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
