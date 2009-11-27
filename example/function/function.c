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

#include <sc_functions.h>

static double
func_sqr (double x, void *data)
{
  return x * x;
}

static double
func_sqrt (double x, void *data)
{
  return sqrt (x);
}

static double
func_sin (double x, void *data)
{
  return sin (2. * M_PI * x);
}

int
main (int argc, char **argv)
{
  double              x1, x2, x3;
  double              y1, y2, y3;

  y1 = .56;
  x1 = sc_function1_invert (func_sqr, NULL, 0., 1., y1, 1e-3);
  SC_STATISTICSF ("Inverted sqr (%g) = %g\n", x1, y1);

  y2 = .56;
  x2 = sc_function1_invert (func_sqrt, NULL, 0., 1., y2, 1e-3);
  SC_STATISTICSF ("Inverted sqrt (%g) = %g\n", x2, y2);

  y3 = .56;
  x3 = sc_function1_invert (func_sin, NULL, 0.25, 0.75, y3, 1e-3);
  SC_STATISTICSF ("Inverted sin (2 pi %g) = %g\n", x3, y3);

  return 0;
}
