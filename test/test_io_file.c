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

/* Save a file to disk in one call, read it and compare. */

#include <sc_io.h>
#include <sc_options.h>

/** Create a dynamic array initialized by copying a const string.
 * \param [in] cstr     String constant must not be NULL.
 * \param [out] plen    If not NULL, on output assigned length of \a str.
 * \return              Byte array with length and contents of \a string.
 */
static sc_array_t  *
array_new_string (const char *cstr, size_t *plen)
{
  size_t              len;
  sc_array_t         *arr;

  /* calculate length of input string */
  SC_ASSERT (cstr != NULL);
  len = strlen (cstr);

  /* create an array of matching length and populate */
  arr = sc_array_new_count (1, len);
  memcpy (sc_array_index (arr, 0), cstr, len);

  /* optionally return string length to caller */
  if (plen != NULL) {
    *plen = len;
  }
  return arr;
}

static int
test_return (int retval, sc_array_t * buffer)
{
  if (retval) {
    SC_LERROR ("Error testing save/load file\n");
  }
  if (buffer != NULL) {
    sc_array_destroy (buffer);
  }
  return retval;
}

int
test_file (const char *filename)
{
  size_t              length;
  const char         *string = "This is a test string for sc_test_io_file.\n";

  /* the buffer is freed before returning from this function */
  sc_array_t         *buffer = NULL;

  /* save a given string to a file */
  buffer = array_new_string (string, &length);

  /* save the string to a file */
  if (sc_io_file_save (filename, buffer)) {
    SC_LERRORF ("Error saving file %s\n", filename);
    return test_return (-1, buffer);
  }
  sc_array_destroy_null (&buffer);

  /* load file contents */
  buffer = sc_array_new (1);
  if (sc_io_file_load (filename, buffer)) {
    SC_LERRORF ("Error loading file %s\n", filename);
    return test_return (-1, buffer);
  }

  /* verify contents */
  if (buffer->elem_count != length) {
    SC_LERRORF ("Length %ld error loading file %s\n",
                (long int) buffer->elem_count, filename);
    return test_return (-1, buffer);
  }
  if (strncmp (buffer->array, string, length)) {
    SC_LERRORF ("Content error loading file %s\n", filename);
    return test_return (-1, buffer);
  }
  sc_array_destroy_null (&buffer);

  /* clean up and return using the same convention as above */
  return test_return (0, buffer);
}

int
main (int argc, char **argv)
{
  int                 mpiret;
  int                 mpirank;
  int                 first;
  int                 iserr;
  int                 retval;
  const char         *filepref;
  char                filename[BUFSIZ];
  sc_MPI_Comm         mpicomm;
  sc_options_t       *opt;

  /* define communicator for logging and general operation */
  mpicomm = sc_MPI_COMM_WORLD;
  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  /* setup logging and stack trace */
  sc_init (mpicomm, 1, 1, NULL, SC_LP_DEFAULT);
  iserr = 0;

  /* process command line options */
  opt = sc_options_new (argv[0]);
  sc_options_add_string (opt, 'f', "filepref", &filepref, "sc_test_io_file",
                         "File to write");
  first = sc_options_parse (sc_package_id, SC_LP_ERROR, opt, argc, argv);
  if (first < argc) {
    sc_options_print_usage (sc_package_id, SC_LP_PRODUCTION, opt, NULL);
    iserr = 1;
  }

  /* execute test for real */
  if (!iserr) {
    int                 retloc;

    /* run test function */
    snprintf (filename, BUFSIZ, "%s.%06d", filepref, mpirank);
    retloc = test_file (filename);

    /* the test function is not collective; synchronize error value */
    mpiret = sc_MPI_Allreduce (&retloc, &retval, 1, sc_MPI_INT,
                               sc_MPI_LOR, mpicomm);
    SC_CHECK_MPI (mpiret);
    if (retval) {
      SC_GLOBAL_LERRORF ("Operational error in %s\n", argv[0]);
      iserr = 1;
    }
  }

  /* clean up program context */
  sc_options_destroy (opt);
  sc_finalize ();

  /* terminate MPI environment */
  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  /* return error status to caller */
  return iserr ? EXIT_FAILURE : EXIT_SUCCESS;
}
