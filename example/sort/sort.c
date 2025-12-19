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

/*
 * Demo program to exercise various sorting algorithms
 */

#include <sc_options.h>
#include <sc_random.h>
#include <sc_sort.h>

typedef struct sc_sort
{
  sc_rand_state_t     rstate;
  int                 mpirank;
  int                 mpisize;
  int                 n;
  int                 myself, mynext, myn;
  int                 roundn;
  sc_array_t         *input;
  sc_array_t         *tosort;
}
sc_sort_t;

static void
print_small (sc_sort_t *s, sc_array_t *a, const char *prefi)
{
  int                 i;

  SC_ASSERT (s != NULL);
  SC_ASSERT (a != NULL);
  SC_ASSERT (a->elem_size == sizeof (double));
  SC_ASSERT (a->elem_count == (size_t) s->myn);

  if (s->myn <= 10 && s->mpirank == 0) {
    for (i = 0; i < s->myn; ++i) {
      SC_INFOF ("%8s %d is %8.6f\n", prefi, i,
                *(double *) sc_array_index_int (a, i));
    }
  }
}

static void
run_sort (sc_sort_t *s)
{
  int                 i;

  /* compute offset and count of elements assigned to rank */
  s->myself = (int) ((s->mpirank * (long) s->n) / s->mpisize);
  s->mynext = (int) (((s->mpirank + 1) * (long) s->n) / s->mpisize);
  s->myn = s->mynext - s->myself;

  /* initialize storage for items to sort */
  s->input = sc_array_new_count (sizeof (double), s->myn);
  for (i = 0; i < s->myn; ++i) {
    *(double *) sc_array_index_int (s->input, i) =
      floor (s->roundn * sc_rand (&s->rstate)) / s->roundn;
  }

  /* print for small output ranges */
  print_small (s, s->input, "Input");

  /* initialize output array */
  s->tosort = sc_array_new (sizeof (double));

  /* run quicksort for comparison */
  sc_array_copy (s->tosort, s->input);
  sc_array_sort (s->tosort, sc_double_compare);
  print_small (s, s->tosort, "Qsort");

  /* run parallel sort for comparison */
  sc_array_copy (s->tosort, s->input);
  sc_psort (sc_MPI_COMM_SELF,
            s->tosort->array, &s->tosort->elem_count, s->tosort->elem_size,
            sc_double_compare);
  print_small (s, s->tosort, "Psort");

  /* release memory */
  sc_array_destroy (s->tosort);
  sc_array_destroy (s->input);
}

int
main (int argc, char **argv)
{
  sc_sort_t           sort, *s = &sort;
  int                 fail = 0;
  int                 mpiret;
  int                 help;
  int                 first_arg;
  size_t              seed;
  sc_options_t       *opt;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

  mpiret = sc_MPI_Comm_size (sc_MPI_COMM_WORLD, &s->mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (sc_MPI_COMM_WORLD, &s->mpirank);
  SC_CHECK_MPI (mpiret);

  opt = sc_options_new (argv[0]);
  sc_options_add_int (opt, 'n', NULL, &s->n, 10, "Total number of items");
  sc_options_add_int (opt, 'r', NULL, &s->roundn, 1000, "Random rounded");
  sc_options_add_size_t (opt, 's', "seed", &seed, 0, "Random number seed");
  sc_options_add_switch (opt, 'h', "help", &help, "Show help information");

  /* process command line options */
  first_arg = sc_options_parse (sc_package_id, SC_LP_INFO, opt, argc, argv);
  if (!fail && first_arg < 0) {
    SC_GLOBAL_LERROR ("Error in option parsing\n");
    fail = 1;
  }
  if (!fail && first_arg < argc) {
    SC_GLOBAL_LERROR ("This program takes no arguments, just options\n");
    fail = 1;
  }
  if (!fail && s->n < 0) {
    SC_GLOBAL_LERROR ("Parameter n must be non-negative\n");
    fail = 1;
  }
  if (!fail && s->roundn <= 0) {
    SC_GLOBAL_LERROR ("Parameter r must be positive\n");
    fail = 1;
  }
  s->rstate = (sc_rand_state_t) (seed ^ (size_t) s->mpirank);

  /* execute main program action */
  if (fail) {
    sc_options_print_usage (sc_package_id, SC_LP_ERROR, opt, NULL);
  }
  else {
    if (help) {
      sc_options_print_usage (sc_package_id, SC_LP_PRODUCTION, opt, NULL);
    }
    else {
      sc_options_print_summary (sc_package_id, SC_LP_PRODUCTION, opt);
      run_sort (s);
    }
  }

  sc_options_destroy (opt);
  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
