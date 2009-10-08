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

#include <sc_complex.h>

int
main (int argc, char **argv)
{
  int                 num_errors = 0;
  double              x;
  sc_double_complex_t a, b, c;

  sc_init (MPI_COMM_NULL, 1, 1, NULL, SC_LP_DEFAULT);

  a = sc_double_complex_t (1.2, 0.0);
  b = sc_double_complex_t (0.0, 3.4);
  c = a + b;

  if (fabs (real (c) - 1.2) > SC_EPS)
    ++num_errors;

  if (fabs (imag (c) - 3.4) > SC_EPS)
    ++num_errors;

  x = 3.56;
  if (3.56 - fabs (x) > SC_EPS)
    ++num_errors;

  sc_finalize ();

  return num_errors > 0;
}
