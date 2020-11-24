/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2020 individual authors

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

#include <sc.h>

#define SC_TEST_TOOLONG 123456789012345678901234567890123456789
#define SC_TEST_LONG    1234567890123456789
#define SC_TEST_INT     123456789

#define TH(s,l,a,b) (test_helpers (SC_TOSTRING (s), l, a, b))

static int
test_helpers (const char *str, const char *label, int tint, int tlong)
{
  int                 nft = 0;
  const int           nint = sc_atoi (str);
  const long          nlong = sc_atol (str);

  if (tint) {
    /* we expect the int test to go through */
    if (nint == INT_MIN || nint == INT_MAX) {
      SC_GLOBAL_LERRORF ("Unexpected Xflow in sc_atoi for %s\n", label);
      ++nft;
    }
  }
  else {
    /* we expect an overflow */
    if (nint != INT_MAX) {
      SC_GLOBAL_LERRORF ("Undetected Xflow in sc_atoi for %s\n", label);
      ++nft;
    }
  }

  if (tlong) {
    /* we expect the long test to go through */
    if (nlong == LONG_MIN || nlong == LONG_MAX) {
      SC_GLOBAL_LERRORF ("Unexpected Xflow in sc_atol for %s\n", label);
      ++nft;
    }
  }
  else {
    /* we expect an overflow */
    if (nlong != LONG_MAX) {
      SC_GLOBAL_LERRORF ("Undetected Xflow in sc_atol for %s\n", label);
      ++nft;
    }
  }

  return nft;
}

int
main (int argc, char **argv)
{
  int                 mpiret;
  sc_MPI_Comm         mpicomm;
  int                 num_failed_tests;

  /* standard initialization */
  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpicomm = sc_MPI_COMM_WORLD;

  sc_init (mpicomm, 1, 1, NULL, SC_LP_DEFAULT);

  /* test integer conversion functions */
  num_failed_tests = 0;
  num_failed_tests += TH (SC_TEST_TOOLONG, "too long", 0, 0);
  num_failed_tests += TH (SC_TEST_LONG, "long", 0, 1);
  num_failed_tests += TH (SC_TEST_INT, "int", 1, 1);

  /* clean up and exit */
  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return num_failed_tests ? EXIT_FAILURE : EXIT_SUCCESS;
}
