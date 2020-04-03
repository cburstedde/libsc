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

static void
check_derivatives (sc_bspline_t * bs)
{
  int                 i, k;
  int                 nevals;
  double              t, h, diff, diff2;
  double              result1[2], result2[2], result3[2], result4[2];

  /* compare derivatives and finite difference approximation */
  nevals = 150;
  for (i = 0; i < nevals; ++i) {
    t = i / (double) (nevals - 1);
    sc_bspline_derivative (bs, t, result1);
    sc_bspline_derivative2 (bs, t, result2);

    diff = 0.;
    for (k = 0; k < 2; ++k) {
      diff += (result1[k] - result2[k]) * (result1[k] - result2[k]);
    }
    SC_CHECK_ABORT (diff < 1e-12, "Derivative mismatch");

    if (i > 0 && i < nevals - 1) {
      h = 1e-8;
      sc_bspline_evaluate (bs, t - h, result2);
      sc_bspline_evaluate (bs, t + h, result3);
      sc_bspline_derivative_n (bs, 0, t + h, result4);

      diff = diff2 = 0.;
      for (k = 0; k < 2; ++k) {
        result2[k] = (result3[k] - result2[k]) / (2. * h);
        diff += (result1[k] - result2[k]) * (result1[k] - result2[k]);
        diff2 += (result3[k] - result4[k]) * (result3[k] - result4[k]);
      }
      SC_CHECK_ABORT (diff < 1e-6, "Difference mismatch");
      SC_CHECK_ABORT (diff2 < 1e-12, "Evaluation mismatch");
    }

#if 0
    sc_bspline_derivative_n (bs, 2, t, result1);
    SC_LDEBUGF ("Second derivative %d %g %g\n", i, result1[0], result1[1]);
#endif
  }
}

int
main (int argc, char **argv)
{
  sc_MPI_Comm         mpicomm;
  int                 mpiret, mpisize;
  int                 retval, nargs;
  int                 minpoints;
  int                 d, p, n;
  double              x, y;
  sc_dmatrix_t       *points, *knots, *works;
  sc_bspline_t       *bs;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  mpicomm = sc_MPI_COMM_WORLD;
  sc_init (mpicomm, 1, 1, NULL, SC_LP_DEFAULT);

  mpiret = sc_MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  if (mpisize != 1)
    sc_abort_collective ("This program runs in serial only");

  nargs = 2;
  if (argc != nargs) {
    SC_LERRORF ("Usage: %s <degree>\n", argv[0]);
    SC_ABORT ("Usage error");
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

  if (n > 0) {
    knots = sc_bspline_knots_new_length (n, points);
    bs = sc_bspline_new (n, points, knots, works);
    create_plot ("length", bs);
    check_derivatives (bs);
    sc_bspline_destroy (bs);
    sc_dmatrix_destroy (knots);
  }

  sc_dmatrix_destroy (works);
  sc_dmatrix_destroy (points);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
