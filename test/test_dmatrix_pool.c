/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2008,2009 Carsten Burstedde, Lucas Wilcox.

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

#include <sc_dmatrix.h>

int
main (int argc, char **argv)
{
#ifdef SC_BLAS
  int                 mpiret;
  sc_dmatrix_pool_t  *p13, *p92;
  sc_dmatrix_t       *m1, *m2, *m3, *m4;

  mpiret = MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (MPI_COMM_WORLD, true, true, NULL, SC_LP_DEFAULT);

  p13 = sc_dmatrix_pool_new (1, 3);
  p92 = sc_dmatrix_pool_new (9, 2);

  m1 = sc_dmatrix_pool_alloc (p13);
  m2 = sc_dmatrix_pool_alloc (p92);
  m3 = sc_dmatrix_pool_alloc (p13);

  sc_dmatrix_pool_free (p13, m1);
  m1 = sc_dmatrix_pool_alloc (p13);

  m4 = sc_dmatrix_pool_alloc (p13);
  sc_dmatrix_pool_free (p13, m1);

  sc_dmatrix_pool_free (p13, m4);
  m4 = sc_dmatrix_pool_alloc (p13);
  m1 = sc_dmatrix_pool_alloc (p13);

  sc_dmatrix_pool_free (p13, m1);
  sc_dmatrix_pool_free (p92, m2);
  sc_dmatrix_pool_free (p13, m3);
  sc_dmatrix_pool_free (p13, m4);

  sc_dmatrix_pool_destroy (p13);
  sc_dmatrix_pool_destroy (p92);

  sc_finalize ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);
#endif

  return 0;
}
