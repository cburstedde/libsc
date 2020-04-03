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

#include <sc_dmatrix.h>

int
main (int argc, char **argv)
{
#ifdef SC_WITH_BLAS
  int                 mpiret;
  sc_dmatrix_pool_t  *p13, *p92;
  sc_dmatrix_t       *m1, *m2, *m3, *m4;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

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

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);
#endif /* !SC_WITH_BLAS */

  return 0;
}
