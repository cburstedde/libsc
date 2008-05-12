/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2007,2008 Carsten Burstedde, Lucas Wilcox.

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

#ifndef SC_FUNCTIONS_H
#define SC_FUNCTIONS_H

typedef double      (*sc_function3_t) (double, double, double, void *);

/*
 * this structure is used as data element for the meta functions.
 * for _sum and _product:
 * f1 needs to be a valid function.
 * f2 can be a function, then it is used,
 *    or NULL, in which case parameter2 is used.
 * for _tensor: f1, f2, f3 need to be valid functions.
 */
typedef struct sc_function3_meta
{
  sc_function3_t      f1;
  sc_function3_t      f2;
  double              parameter2;
  sc_function3_t      f3;
  void               *data;
}
sc_function3_meta_t;

double              sc_zero (double x, double y, double z, void *data);
double              sc_one (double x, double y, double z, void *data);
double              sc_two (double x, double y, double z, void *data);
double              sc_ten (double x, double y, double z, void *data);

/**
 * \param data   needs to be *double with the value of the constant.
 */
double              sc_constant (double x, double y, double z, void *data);

double              sc_x (double x, double y, double z, void *data);
double              sc_y (double x, double y, double z, void *data);
double              sc_z (double x, double y, double z, void *data);

double              sc_sum (double x, double y, double z, void *data);
double              sc_product (double x, double y, double z, void *data);
double              sc_tensor (double x, double y, double z, void *data);

#endif /* !SC_FUNCTIONS_H */
