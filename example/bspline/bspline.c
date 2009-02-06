/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2009 Carsten Burstedde, Lucas Wilcox.

  The SC Library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with the SC Library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sc_bspline.h>

static void
create_plot (const char *name, sc_bspline_t * bs)
{
  int                 retval;
  int                 i, nevals;
  char                filename[BUFSIZ];
  double             *result;
  const double       *knotse;
  FILE               *gf;

  SC_INFOF ("Creating plot %s\n", name);

  result = SC_ALLOC (double, SC_MAX (bs->d, 2));
  knotse = bs->knots->e[0];

  snprintf (filename, BUFSIZ, "%s.gnuplot", name);
  gf = fopen (filename, "wb");
  SC_CHECK_ABORT (gf != NULL, "Plot file open");

  fprintf (gf, "set key left\n"
           "set size ratio -1\n"
           "set output \"%s.eps\"\n"
           "set terminal postscript color solid\n", name);
  fprintf (gf, "plot '-' title \"points\" with linespoints, "
           "'-' title \"spline\" with lines, "
           "'-' title \"knot values\", " "'-' title \"uniform values\"\n");

  /* plot control points */
  for (i = 0; i <= bs->p; ++i) {
    fprintf (gf, "%g %g\n", bs->points->e[i][0], bs->points->e[i][1]);
  }
  fprintf (gf, "e\n");

  /* plot spline curve */
  nevals = 150;
  for (i = 0; i < nevals; ++i) {
    sc_bspline_evaluate (bs, i / (double) (nevals - 1), result);
    fprintf (gf, "%g %g\n", result[0], result[1]);
  }
  fprintf (gf, "e\n");

  /* plot spline points at knot values */
  for (i = 0; i <= bs->l; ++i) {
    sc_bspline_evaluate (bs, knotse[bs->n + i], result);
    fprintf (gf, "%g %g\n", result[0], result[1]);
  }
  fprintf (gf, "e\n");

  /* plot spline points at equidistant values */
  for (i = 0; i <= bs->l; ++i) {
    sc_bspline_evaluate (bs, i / (double) bs->l, result);
    fprintf (gf, "%g %g\n", result[0], result[1]);
  }
  fprintf (gf, "e\n");

  retval = fclose (gf);
  SC_CHECK_ABORT (retval == 0, "Plot file close");

  SC_FREE (result);
}

int
main (int argc, char **argv)
{
  int                 retval, nargs;
  int                 minpoints;
  int                 d, p, n;
  double              x, y;
  sc_dmatrix_t       *points, *knots, *works;
  sc_bspline_t       *bs;

  sc_init (MPI_COMM_NULL, true, true, NULL, SC_LP_DEFAULT);

  nargs = 2;
  if (argc != nargs) {
    SC_PRODUCTIONF ("Usage: %s <degree>\n", argv[0]);
    SC_CHECK_ABORT (false, "Usage error");
  }
  n = atoi (argv[1]);
  SC_CHECK_ABORT (n >= 0, "Degree must be non-negative");

  minpoints = sc_bspline_min_number_points (n);
  SC_INFOF ("Degree %d will require at least %d points\n", n, minpoints);

  d = 2;
  points = sc_dmatrix_new (0, d);

  p = -1;
  for (;;) {
    retval = scanf ("%lg %lg", &x, &y);
    if (retval == d) {
      ++p;
      sc_dmatrix_resize (points, p + 1, d);
      points->e[p][0] = x;
      points->e[p][1] = y;
    }
    else
      break;
  }
  SC_CHECK_ABORT (p + 1 >= minpoints, "Not enough points");
  SC_INFOF ("Points read %d\n", p + 1);

  works = sc_bspline_workspace_new (n, d);

  knots = sc_bspline_knots_new (n, points);
  bs = sc_bspline_new (n, points, knots, works);
  create_plot ("uniform", bs);
  sc_bspline_destroy (bs);
  sc_dmatrix_destroy (knots);

  knots = sc_bspline_knots_new_length (n, points);
  bs = sc_bspline_new (n, points, knots, works);
  create_plot ("length", bs);
  sc_bspline_destroy (bs);
  sc_dmatrix_destroy (knots);

  sc_dmatrix_destroy (works);
  sc_dmatrix_destroy (points);

  sc_finalize ();

  return 0;
}
