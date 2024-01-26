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

/* Save a file to disk in one call, read it and compare.
 * The first read is replicated on all processes.
 * The second read is on one rank and broadcast.
 * We round robin this over all available ranks. */

#include <sc_io.h>
#include <sc_options.h>

/** Create a dynamic array initialized by copying a const string.
 * The array contains the string bytes without the terminating NUL.
 * We would rather do this using a view to maintain an inofficial NUL
 * at the buffer's end, but the \ref sc_array_t does not work with const.
 * \param [in] cstr     String constant must not be NULL.
 * \param [out] plen    If not NULL, on output assigned length of \a str.
 * \return              Byte array with length and contents of \a string
 *                      not including the input's terminating NUL.
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

/** Return and free loose ends is a collective function. */
static int
test_return (int retval, sc_array_t * buffer)
{
  if (retval) {
    SC_GLOBAL_LERROR ("Error testing save/load file\n");
  }
  if (buffer != NULL) {
    sc_array_destroy (buffer);
  }
  return retval;
}

/** Broadcast in-argument retval from rank 0 to all.
 * \return input \a retval collectively on all ranks.
 */
static int
bcast_retval (sc_MPI_Comm mpicomm, int *retval)
{
  int                 mpiret;

  SC_ASSERT (retval != NULL);

  mpiret = sc_MPI_Bcast (retval, 1, sc_MPI_INT, 0, mpicomm);
  SC_CHECK_MPI (mpiret);

  return *retval;
}

/** Logical OR the in-argument over all ranks.
 * \return result collectively on all ranks.
 */
static int
bclor_retval (sc_MPI_Comm mpicomm, int *retval)
{
  int                 mpiret;
  int                 retres;

  SC_ASSERT (retval != NULL);

  mpiret = sc_MPI_Allreduce (retval, &retres, 1, sc_MPI_INT,
                             sc_MPI_LOR, mpicomm);
  SC_CHECK_MPI (mpiret);

  return *retval = retres;
}

/** Non-collective function to verify buffer against a given string */
static int
verify_contents (const char *filename, const char *verbing,
                 sc_array_t *buffer, const char *string, size_t length)
{
  SC_ASSERT (buffer != NULL && buffer->elem_size == 1);
  SC_ASSERT (string != NULL);

  if (buffer->elem_count != length) {
    SC_LERRORF ("Length %ld/%ld error %s file %s\n",
                (long int) buffer->elem_count, (long int) length,
                verbing, filename);
    return -1;
  }
  if (strncmp (sc_array_index (buffer, 0), string, length)) {
    SC_LERRORF ("Content error %s file %s\n", verbing, filename);
    return -1;
  }
  return 0;
}

/** This function has a collective return over the communicator. */
int
test_file (const char *filename, sc_MPI_Comm mpicomm)
{
  int                 mpiret;
  int                 retval;
  int                 size, rank, root;
  size_t              length;
  const char         *string = "This is a test string for sc_test_io_file.\n";

  /* the buffer is freed before returning from this function */
  sc_array_t         *buffer = NULL;

  /* access properties of communicator */
  mpiret = sc_MPI_Comm_size (mpicomm, &size);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &rank);
  SC_CHECK_MPI (mpiret);

  /* save the string to a file and remember length on all ranks */
  retval = -1;
  buffer = array_new_string (string, &length);
  if (rank == 0 && (retval = sc_io_file_save (filename, buffer))) {
    SC_LERRORF ("Error saving file %s\n", filename);
  }
  if (bcast_retval (mpicomm, &retval)) {

    /* this return is collective */
    return test_return (-1, buffer);
  }
  sc_array_destroy_null (&buffer);
  SC_ASSERT (!retval);

  /* we are synced in time: load file contents replicated */
  buffer = sc_array_new (1);
  if ((retval = sc_io_file_load (filename, buffer, -1))) {
    SC_LERRORF ("Error loading file %s\n", filename);
  }
  if (bclor_retval (mpicomm, &retval)) {

    /* this return is collective */
    return test_return (-1, buffer);
  }
  SC_ASSERT (!retval);

  /* verify length and content found in file */
  retval = verify_contents (filename, "loading", buffer, string, length);
  if (bclor_retval (mpicomm, &retval)) {

    /* this return is collective */
    return test_return (-1, buffer);
  }
  sc_array_destroy_null (&buffer);

  /* round robin single read and broadcast over all ranks */
  for (root = 0; root < size; ++root) {
    /* load and broadcast file contents */
    buffer = sc_array_new (1);
    if (sc_io_file_bcast (filename, buffer, -1, root, mpicomm)) {

      /* this return is collective */
      SC_GLOBAL_LERRORF ("Error bcasting file %s\n", filename);
      return test_return (-1, buffer);
    }

    /* verify length and content found in file */
    retval = verify_contents (filename, "bcasting", buffer, string, length);
    if (bclor_retval (mpicomm, &retval)) {

      /* this return is collective */
      return test_return (-1, buffer);
    }
    sc_array_destroy_null (&buffer);
  }

  /* clean up and return using the same convention as above */
  return test_return (0, buffer);
}

int
main (int argc, char **argv)
{
  int                 mpiret;
  int                 first;
  int                 iserr;
  const char         *filepref;
  char                filename[BUFSIZ];
  sc_MPI_Comm         mpicomm;
  sc_options_t       *opt;

  /* define communicator for logging and general operation */
  mpicomm = sc_MPI_COMM_WORLD;
  mpiret = sc_MPI_Init (&argc, &argv);
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

    /* run test function */
    snprintf (filename, BUFSIZ, "%s.dat", filepref);
    if (test_file (filename, mpicomm)) {

      /* this branch is collective */
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
