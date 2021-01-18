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

static sc3_error_t *
test_mpienv (sc3_allocator_t * alloc, sc3_MPI_Comm_t mpicomm, int shared)
{
  return NULL;
}

static sc3_error_t *
init_alloc (sc3_allocator_t * mainalloc, sc3_allocator_t ** alloc)
{
  SC3E (sc3_allocator_new (mainalloc, alloc));
  SC3E (sc3_allocator_set_align (*alloc, /* just to be unusual */ 64));
  SC3E (sc3_allocator_setup (*alloc));
  return NULL;
}

static sc3_error_t *
reset_alloc (sc3_allocator_t * mainalloc, sc3_allocator_t ** alloc)
{
  SC3E (sc3_allocator_destroy (alloc));
  SC3E_DEMIS (sc3_allocator_is_free, mainalloc);
  return NULL;
}

static int
check_error (sc3_error_t * e, const char *msg, int size, int rank)
{
  char buffer[SC3_BUFSIZE];
  if (sc3_error_check (&e, buffer, SC3_BUFSIZE)) {
    fprintf (stderr, "Error on rank %d/%d by %s:\n%s\n",
             rank, size, msg, buffer);
    return 1;
  }
  return 0;
}

#define CHECK_ERROR(f) (check_error (f, #f, s, r))

int
main (int argc, char **argv)
{
  int                 num_failed_tests = 0;
  int                 s = -1, r = -1;
  sc3_allocator_t    *mainalloc = sc3_allocator_nothread ();
  sc3_allocator_t    *alloc;

  /* Primitive error checking */
  SC3X (sc3_MPI_Init (&argc, &argv));
  SC3X (sc3_MPI_Comm_size (SC3_MPI_COMM_WORLD, &s));
  SC3X (sc3_MPI_Comm_rank (SC3_MPI_COMM_WORLD, &r));

  /* Sophisticated error checking */
  num_failed_tests += CHECK_ERROR (init_alloc (mainalloc, &alloc));
  num_failed_tests += CHECK_ERROR (test_mpienv (alloc, SC3_MPI_COMM_SELF, 0));
  num_failed_tests += CHECK_ERROR (test_mpienv (alloc, SC3_MPI_COMM_WORLD, 0));
  num_failed_tests += CHECK_ERROR (test_mpienv (alloc, SC3_MPI_COMM_WORLD, 1));
  num_failed_tests += CHECK_ERROR (reset_alloc (mainalloc, &alloc));

  /* Primitive error checking */
  SC3X (sc3_MPI_Finalize ());
  return num_failed_tests ? EXIT_FAILURE : EXIT_SUCCESS;
}
