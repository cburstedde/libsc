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

#include <sc_io.h>

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

static int
single_inplace_test (sc_array_t *src, int itest)
{
  const size_t        inz[3] = { 0, 1324, 139422 };
  int                 i;
  int                 retval;
  int                 num_failed_tests = 0;
  size_t              sz, mz;
  size_t              original_size;
  sc_array_t          inp, view, targ;

  SC_ASSERT (src != NULL);
  sc_array_init (&inp, 1);
  sc_array_init (&view, 1);
  sc_array_init (&targ, 1);

  for (i = 0; i < 3; ++i) {
    /* original size */
    sz = src->elem_size * src->elem_count;

    /* encode input in place */
    sc_array_init_count (&inp, 1, sz);
    memcpy (inp.array, src->array, sz);
    sc_io_encode (&inp, NULL);

    /* decode in place with array */
    retval = sc_io_decode (&inp, NULL, 0);
    if (retval) {
      SC_LERRORF ("decode array in place error %d %d\n", itest, i);
      ++num_failed_tests;
      goto error_inplace_test;
    }
    if (inp.elem_count != sz) {
      SC_LERRORF ("decode array in place size %d %d\n", itest, i);
      ++num_failed_tests;
      goto error_inplace_test;
    }
    sc_array_reset (&inp);

    if (!sc_io_have_zlib () || itest != -1) {
      /* for the examples we call, test -1 has a large enough data
         size that the encoded data is shorter than the plaintext. */

      /* encode input in place */
      sc_array_init_count (&inp, 1, sz);
      memcpy (inp.array, src->array, sz);
      sc_io_encode (&inp, NULL);

      /* decode in place with view */
      sc_array_init_view (&view, &inp, 0, inp.elem_count);
      retval = sc_io_decode (&view, NULL, 0);
      if (retval) {
        SC_LERRORF ("decode view in place error %d %d\n", itest, i);
        ++num_failed_tests;
        goto error_inplace_test;
      }

      if (view.elem_count != sz) {
        SC_LERRORF ("decode view in place size %d %d\n", itest, i);
        ++num_failed_tests;
        goto error_inplace_test;
      }
      sc_array_reset (&view);
      sc_array_reset (&inp);
    }

    /* encode input in place */
    sc_array_init_count (&inp, 1, sz);
    memcpy (inp.array, src->array, sz);
    sc_io_encode (&inp, NULL);

    /* decode and verify original data size */
    if (sc_io_decode_length (&inp, &original_size)) {
      SC_LERRORF ("decode length error on test %d %d\n", itest, i);
      ++num_failed_tests;
      goto error_inplace_test;
    }
    if (original_size != sz) {
      SC_LERRORF ("decode length mismatch on test %d %d\n", itest, i);
      ++num_failed_tests;
      goto error_inplace_test;
    }

    /* decode into view of fitting size */
    mz = SC_MAX (sz, inz[i]);
    sc_array_init_count (&targ, 1, mz);
    sc_array_init_view (&view, &targ, 0, mz);
    retval = sc_io_decode (&inp, &view, 0);
    if (retval) {
      SC_LERRORF ("decode view error %d %d\n", itest, i);
      ++num_failed_tests;
      goto error_inplace_test;
    }
    if (view.elem_count != sz) {
      SC_LERRORF ("decode view size %d %d\n", itest, i);
      ++num_failed_tests;
      goto error_inplace_test;
    }
    sc_array_reset (&inp);
    sc_array_reset (&view);
    sc_array_reset (&targ);
  }

error_inplace_test:
  sc_array_reset (&inp);
  sc_array_reset (&view);
  sc_array_reset (&targ);
  return num_failed_tests;
}

static int
single_code_test (sc_array_t *src, int itest)
{
  int                 num_failed_tests = 0;
  int                 retval;
  size_t              original_size;
  sc_array_t          dest;
  sc_array_t          comp;

  if (itest < 3) {
    num_failed_tests += single_inplace_test (src, itest);
  }

  /* encode */
  SC_ASSERT (src != NULL);
  sc_array_init (&comp, src->elem_size);
  sc_array_init (&dest, 1);
  sc_io_encode (src, &dest);

  /* decode and verify original data size */
  if (sc_io_decode_length (&dest, &original_size)) {
    SC_LERRORF ("decode length error on test %d\n", itest);
    ++num_failed_tests;
    goto error_code_test;
  }
  if (original_size != src->elem_count * src->elem_size) {
    SC_LERRORF ("decode length mismatch on test %d\n", itest);
    ++num_failed_tests;
    goto error_code_test;
  }

  /* decode */
  retval = sc_io_decode (&dest, &comp, 0);
  if (retval) {
    SC_LERRORF ("test %d: sc_io_decode internal error\n", itest);
    ++num_failed_tests;
    goto error_code_test;
  }
  if (src->elem_count != comp.elem_count) {
    SC_LERRORF ("test %d: sc_io_decode length mismatch\n", itest);
    ++num_failed_tests;
    goto error_code_test;
  }

  /* compare input and output data */
  if (memcmp (src->array, comp.array, src->elem_size * src->elem_count)) {
    SC_LERRORF ("test %d: encode/decode data mismatch\n", itest);
    ++num_failed_tests;
    goto error_code_test;
  }

error_code_test:
  sc_array_reset (&comp);
  sc_array_reset (&dest);
  sc_array_reset (src);
  return num_failed_tests;
}

static int
test_encode_decode (void)
{
  int                 num_failed_tests = 0;
  int                 i, j;
  size_t              slen;
  const char         *str1 = "Hello world.  This is a short text.";
  const char         *str2 =
    "This is a much longer text.  We just paste stuff.\n"
    "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE\
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR\
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF\
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN\
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE\
 POSSIBILITY OF SUCH DAMAGE.";
  sc_array_t          src;

  sc_array_init_data (&src, (void *) str1, 1, strlen (str1) + 1);
  single_code_test (&src, -2);

  sc_array_init_data (&src, (void *) str2, 1, strlen (str2) + 1);
  single_code_test (&src, -1);

  for (i = 0; i <= 2000; ++i) {
    if (i % 500 == 0) {
      SC_LDEBUGF ("Code iteration %d\n", i);
    }
    slen = i <= 1800 ? i : 8 * i;
    sc_array_init_count (&src, sizeof (int), slen);
    for (j = 0; j < (int) slen; ++j) {
      *(int *) sc_array_index_int (&src, j) = 3 * i + 4 * j + 5;
    }
    num_failed_tests += single_code_test (&src, i);
    if (num_failed_tests >= 50) {
      break;
    }
  }

  return num_failed_tests;
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

  /* test encode and decode functions */
  num_failed_tests += test_encode_decode ();

  /* clean up and exit */
  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return num_failed_tests ? EXIT_FAILURE : EXIT_SUCCESS;
}
