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

#include <sc.h>

#ifdef SC_HAVE_DERIVED_DATATYPES
static void
sc_test_wrap_non_blocking (sc_MPI_Comm mpicomm)
{
  char               *data, *recv_buf;
  size_t              si;
  size_t              count = (size_t) INT_MAX + 1;
  int                 mpiret, mpisize, mpirank;
  sc_MPI_Request      req_send, req_recv;

  mpiret = sc_MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);

  if (mpisize == 1) {
    /* no non-trivial communication possible */
    return;
  }

  mpiret = sc_MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  /* create dummy data that exceeds INT_MAX as byte count */
  data = SC_ALLOC (char, count);
  recv_buf = NULL;
  if (mpirank == 1) {
    recv_buf = SC_ALLOC (char, count);
  }

  /* set data */
  /* we can not use memset since it has an int parameter for the count */
  for (si = 0; si < count; ++si) {
    data[si] = 'a';
  }

  /* use wrapper to ship the data */
  if (mpirank == 0) {
    mpiret =
      sc_wrap_Isend (data, count, sc_MPI_BYTE, 1, 0, mpicomm, &req_send);
    SC_CHECK_MPI (mpiret);
    mpiret = sc_MPI_Wait (&req_send, sc_MPI_STATUS_IGNORE);
    SC_CHECK_MPI (mpiret);
  }

  if (mpirank == 1) {
    mpiret =
      sc_wrap_Irecv (recv_buf, count, sc_MPI_BYTE, 0, 0, mpicomm, &req_recv);
    SC_CHECK_MPI (mpiret);
    mpiret = sc_MPI_Wait (&req_recv, sc_MPI_STATUS_IGNORE);
    SC_CHECK_MPI (mpiret);
  }

  if (mpirank == 1) {
    /* check the received data */
    SC_CHECK_ABORT (!memcmp (data, recv_buf, count), "Data mismatch");
  }

  SC_FREE (data);
  if (mpirank == 1) {
    SC_FREE (recv_buf);
  }
}
#endif

int
main (int argc, char **argv)
{
  sc_MPI_Comm         mpicomm = sc_MPI_COMM_WORLD;
  int                 mpiret;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  sc_init (mpicomm, 1, 1, NULL, SC_LP_INFO);

#ifdef SC_HAVE_DERIVED_DATATYPES
  sc_test_wrap_non_blocking (mpicomm);
#endif

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
