/*
  This file is part of the SC Library, version 3.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include <sc3_mpienv.h>
#include <sc3_refcount.h>

struct sc3_mpienv
{
  sc3_refcount_t      rc;
  sc3_allocator_t    *mator;
  int                 setup;

  /* parameters fixed after setup call */
  sc3_MPI_Comm_t      mpicomm;
  int                 commdup;

  /* member variables initialized in setup call */
};

int
sc3_mpienv_is_valid (const sc3_mpienv_t * m, char *reason)
{
  SC3E_TEST (m != NULL, reason);
  SC3E_IS (sc3_refcount_is_valid, &m->rc, reason);
  SC3E_IS (sc3_allocator_is_setup, m->mator, reason);

  /* specific checks here */
  SC3E_TEST (!m->setup || m->mpicomm != SC3_MPI_COMM_NULL, reason);

  SC3E_YES (reason);
}

int
sc3_mpienv_is_new (const sc3_mpienv_t * m, char *reason)
{
  SC3E_IS (sc3_mpienv_is_valid, m, reason);
  SC3E_TEST (!m->setup, reason);
  SC3E_YES (reason);
}

int
sc3_mpienv_is_setup (const sc3_mpienv_t * m, char *reason)
{
  SC3E_IS (sc3_mpienv_is_valid, m, reason);
  SC3E_TEST (m->setup, reason);
  SC3E_YES (reason);
}

sc3_error_t        *
sc3_mpienv_new (sc3_allocator_t * mator, sc3_mpienv_t ** mp)
{
  sc3_mpienv_t       *m;

  SC3E_RETVAL (mp, NULL);
  SC3A_IS (sc3_allocator_is_setup, mator);

  SC3E (sc3_allocator_ref (mator));
  SC3E (sc3_allocator_calloc_one (mator, sizeof (sc3_mpienv_t), &m));
  SC3E (sc3_refcount_init (&m->rc));

  /* set defaults here whenever not zero/null */
  m->mpicomm = SC3_MPI_COMM_WORLD;

  SC3A_IS (sc3_mpienv_is_new, m);
  *mp = m;
  return NULL;
}

sc3_error_t        *
sc3_mpienv_set_comm (sc3_mpienv_t * m, sc3_MPI_Comm_t comm, int dup)
{
  SC3A_IS (sc3_mpienv_is_new, m);
  SC3A_CHECK (comm != SC3_MPI_COMM_NULL);

  /* remove previous communicator */
  if (m->commdup) {
    SC3E (sc3_MPI_Comm_free (&m->mpicomm));
  }

  /* register new communicator */
  if (dup) {
    SC3E (sc3_MPI_Comm_dup (comm, &m->mpicomm));
    SC3E (sc3_MPI_Comm_set_errhandler (m->mpicomm, SC3_MPI_ERRORS_RETURN));
  }
  else {
    m->mpicomm = comm;
  }
  m->commdup = dup;
  return NULL;
}

sc3_error_t        *
sc3_mpienv_setup (sc3_mpienv_t * m)
{
  SC3A_IS (sc3_mpienv_is_new, m);

  /* set mpienv to setup state */
  m->setup = 1;
  SC3A_IS (sc3_mpienv_is_setup, m);
  return NULL;
}

sc3_error_t        *
sc3_mpienv_ref (sc3_mpienv_t * m)
{
  SC3E (sc3_refcount_ref (&m->rc));
  return NULL;
}

sc3_error_t        *
sc3_mpienv_unref (sc3_mpienv_t ** mp)
{
  int                 waslast;
  sc3_allocator_t    *mator;
  sc3_mpienv_t       *m;
  sc3_error_t        *leak = NULL;

  SC3E_INOUTP (mp, m);
  SC3A_IS (sc3_mpienv_is_valid, m);
  SC3E (sc3_refcount_unref (&m->rc, &waslast));
  if (waslast) {
    *mp = NULL;

    mator = m->mator;
    if (m->setup) {
      /* deallocate data created on setup here */
    }

    /* deallocated data knonw on setup here */
    if (m->commdup) {
      SC3E (sc3_MPI_Comm_free (&m->mpicomm));
    }

    SC3E (sc3_allocator_free (mator, m));
    SC3L (&leak, sc3_allocator_unref (&mator));
  }
  return leak;
}

sc3_error_t        *
sc3_mpienv_destroy (sc3_mpienv_t ** mp)
{
  sc3_error_t        *leak = NULL;
  sc3_mpienv_t       *m;

  SC3E_INULLP (mp, m);
  SC3L_DEMAND (&leak, sc3_refcount_is_last (&m->rc, NULL));
  SC3L (&leak, sc3_mpienv_unref (&m));

  SC3A_CHECK (m == NULL || leak != NULL);
  return leak;
}
