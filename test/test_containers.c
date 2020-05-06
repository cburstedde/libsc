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

#include <sc.h>
#include <sc3_memstamp.h>

static sc3_error_t *
test_mstamp (void)
{
  const size_t        munit[3] = { 0, 17, 5138 };
  const size_t        msize[3] = { 20, 37, 537 };
  int                 i, j;
  size_t              isize;
  void               *pc;
  sc3_mstamp_t       *mst;

  for (j = 0; j < 3; ++j) {
    /* zero data size */
    SC3E (sc3_mstamp_init (sc3_allocator_nocount (), munit[j], 0, &mst));
    for (i = 0; i < 8 + 3 * j; ++i) {
      SC3E (sc3_mstamp_alloc (mst, &pc));
      SC_CHECK_ABORT (pc == NULL, "Mstamp alloc NULL");
    }
    SC3E (sc3_mstamp_destroy (&mst));
    /* proper data size */
    isize = msize[j];
    SC3E (sc3_mstamp_init (sc3_allocator_nocount (), munit[j], isize, &mst));
    for (i = 0; i < 7829; ++i) {
      SC3E (sc3_mstamp_alloc (mst, &pc));
      memset (pc, -1, isize);
    }
    SC3E (sc3_mstamp_destroy (&mst));
    SC3E (sc3_mstamp_init (sc3_allocator_nocount (), munit[j], isize, &mst));
    for (i = 0; i < 3124; ++i) {
      SC3E (sc3_mstamp_alloc (mst, &pc));
      memset (pc, -1, isize);
    }
    for (i = 0; i < 3124; ++i) {
      SC3E (sc3_mstamp_free (mst, pc));
    }
    SC3E (sc3_mstamp_destroy (&mst));
  }
  return NULL;
}

static void
report_errors (sc3_error_t ** pe)
{
  char                eflat[SC3_BUFSIZE];

  if (pe != NULL && *pe != NULL) {
    sc3_error_destroy_noerr (pe, eflat);
    fprintf (stderr, "Error: %s\n", eflat);
  }
}

int
main (int argc, char **argv)
{
  sc3_error_t        *e;
  SC3E_SET (e, test_mstamp ());
  report_errors (&e);
  SC_CHECK_ABORT (e == NULL, "Memory stamp's tests failed\n");
  return 0;
}
