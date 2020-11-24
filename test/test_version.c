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
#include <stdio.h>
#include <string.h>

int
main (int argc, char **argv)
{
  int                 mpiret;
  sc_MPI_Comm         mpicomm;
  int                 num_failed_tests;
  int                 version_major, version_minor;
  const char         *version;
  char                version_tmp[32];

  /* standard initialization */
  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpicomm = sc_MPI_COMM_WORLD;

  sc_init (mpicomm, 1, 1, NULL, SC_LP_DEFAULT);

  /* check all functions related to version numbers of libsc */
  num_failed_tests = 0;
  version = sc_version ();
  SC_GLOBAL_LDEBUGF ("Full SC version: %s\n", version);

  version_major = sc_version_major ();
  SC_GLOBAL_LDEBUGF ("Major SC version: %d\n", version_major);
  snprintf (version_tmp, 32, "%d", version_major);
  if (strncmp (version, version_tmp, strlen (version_tmp))) {
    SC_VERBOSE ("Test failure for major version of SC\n");
    num_failed_tests++;
  }

  version_minor = sc_version_minor ();
  SC_GLOBAL_LDEBUGF ("Minor SC version: %d\n", version_minor);
  snprintf (version_tmp, 32, "%d.%d", version_major, version_minor);
  if (strncmp (version, version_tmp, strlen (version_tmp))) {
    SC_VERBOSE ("Test failure for minor version of SC\n");
    num_failed_tests++;
  }

  /* clean up and exit */
  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return num_failed_tests ? 1 : 0;
}
