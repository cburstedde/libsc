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

#include <sc_scda.h>
#include <sc_options.h>

#define SC_SCDA_FILE_EXT "scd"
#define SC_SCDA_TEST_FILE "sc_test_scda." SC_SCDA_FILE_EXT

#define SC_SCDA_GLOBAL_ARRAY_COUNT 12
#define SC_SCDA_ARRAY_SIZE 3

static void
test_scda_write_fixed_size_array (sc_scda_fcontext_t *fc, int mpirank,
                                  int mpisize)
{
  const int           indirect = 0;
  int                 i;
  char               *data_ptr;
  size_t              si;
  const size_t        elem_size = SC_SCDA_ARRAY_SIZE;
  size_t              local_elem_count;
  const sc_scda_ulong global_elem_count = SC_SCDA_GLOBAL_ARRAY_COUNT;
  sc_scda_ulong       per_proc_count, remainder_count;
  sc_array_t          elem_counts, data;
  sc_scda_ferror_t    errcode;

  sc_array_init_count (&elem_counts, sizeof (sc_scda_ulong),
                       (size_t) mpisize);

  /* get the counts per process */
  per_proc_count = global_elem_count / (sc_scda_ulong) mpisize;
  remainder_count = global_elem_count % (sc_scda_ulong) mpisize;

  /* set elem_counts */
  for (i = 0; i < mpisize; ++i) {
    *((sc_scda_ulong *) sc_array_index_int (&elem_counts, i)) =
      per_proc_count;
  }
  *((sc_scda_ulong *) sc_array_index_int (&elem_counts, mpisize - 1)) +=
    remainder_count;

  /* create local data */
  local_elem_count =
    (size_t) *((sc_scda_ulong *) sc_array_index_int (&elem_counts, mpirank));
  sc_array_init_size (&data, elem_size, local_elem_count);
  for (si = 0; si < local_elem_count; ++si) {
    data_ptr = (char *) sc_array_index (&data, si);
    data_ptr[0] = 'a';
    data_ptr[1] = 'b';
    data_ptr[2] = 'c';
  }

  fc = sc_scda_fwrite_array (fc, "A fixed-length array section", NULL, &data,
                             &elem_counts, elem_size, indirect, 0, &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "sc_scda_fwrite_array failed");

  /* write an empty array */

  /* set elem_counts */
  for (i = 0; i < mpisize; ++i) {
    *((sc_scda_ulong *) sc_array_index_int (&elem_counts, i)) = 0;
  }

  sc_array_resize (&data, 0);

  fc = sc_scda_fwrite_array (fc, "An empty array", NULL, &data,
                             &elem_counts, elem_size, indirect, 0, &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "sc_scda_fwrite_array empty array failed");

  sc_array_reset (&elem_counts);
  sc_array_reset (&data);
}

static void
test_scda_read_fixed_size_array (sc_scda_fcontext_t *fc, int mpirank,
                                 int mpisize)
{
  const int           indirect = 0;
  int                 i;
  int                 decode;
  char                read_user_string[SC_SCDA_USER_STRING_BYTES + 1];
  char                section_type;
  char               *data_ptr;
  size_t              len;
  size_t              elem_count, elem_size, si;
  size_t              num_local_elements;
  sc_scda_ferror_t    errcode;
  sc_array_t          array_data, elem_counts;
  sc_scda_ulong       per_proc_count, remainder_count;

  fc =
    sc_scda_fread_section_header (fc, read_user_string, &len, &section_type,
                                  &elem_count, &elem_size, &decode, &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "sc_scda_fread_section_header failed");
  SC_CHECK_ABORT (section_type == 'A'
                  && elem_count == SC_SCDA_GLOBAL_ARRAY_COUNT
                  && elem_size == SC_SCDA_ARRAY_SIZE,
                  "Identifying section type");

  /* read array data */
  sc_array_init (&array_data, elem_size);
  sc_array_init_size (&elem_counts, sizeof (sc_scda_ulong), (size_t) mpisize);

  /* get the counts per process */
  per_proc_count = elem_count / (sc_scda_ulong) mpisize;
  remainder_count = elem_count % (sc_scda_ulong) mpisize;

  /* set elem_counts, i.e. the reading partition */
  for (i = 0; i < mpisize; ++i) {
    *((sc_scda_ulong *) sc_array_index_int (&elem_counts, i)) =
      per_proc_count;
  }
  *((sc_scda_ulong *) sc_array_index_int (&elem_counts, mpisize - 1)) +=
    remainder_count;

  /* allocate space for data that will be read */
  num_local_elements =
    (size_t) *((sc_scda_ulong *) sc_array_index_int (&elem_counts, mpirank));
  sc_array_resize (&array_data, num_local_elements);

  /* read the array data */
  fc = sc_scda_fread_array_data (fc, &array_data, &elem_counts, elem_size,
                                 indirect, &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "sc_scda_fread_array_data failed");

  /* check read data */
  for (si = 0; si < num_local_elements; ++si) {
    data_ptr = (char *) sc_array_index (&array_data, si);
    SC_CHECK_ABORT (data_ptr[0] == 'a' && data_ptr[1] == 'b' &&
                    data_ptr[2] == 'c', "sc_scda_fread_array_data data "
                    "mismatch");
  }

  sc_array_reset (&array_data);
  sc_array_reset (&elem_counts);

  /* read the empty array */
  fc =
    sc_scda_fread_section_header (fc, read_user_string, &len, &section_type,
                                  &elem_count, &elem_size, &decode, &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "sc_scda_fread_section_header failed");
  SC_CHECK_ABORT (section_type == 'A' && elem_count == 0
                  && elem_size == SC_SCDA_ARRAY_SIZE,
                  "Identifying section type");

  /* define trivial partition; a partition is always required */
  sc_array_init_count (&elem_counts, sizeof (sc_scda_ulong), (size_t) mpisize);
  for (i = 0; i < mpisize; ++i) {
    *((sc_scda_ulong *) sc_array_index_int (&elem_counts, i)) = 0;
  }

  fc = sc_scda_fread_array_data (fc, NULL, &elem_counts, elem_size, indirect,
                                 &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "sc_scda_fread_array_data skip empty array failed");

  sc_array_reset (&elem_counts);
}

int
main (int argc, char **argv)
{
  sc_MPI_Comm         mpicomm = sc_MPI_COMM_WORLD;
  int                 mpiret, mpirank, mpisize;
  int                 first_argc;
  int                 int_everyn, int_seed;
  int                 decode;
  const char         *filename = SC_SCDA_TEST_FILE;
  const char         *file_user_string = "This is a test file";
  char                read_user_string[SC_SCDA_USER_STRING_BYTES + 1];
  char                section_type;
  sc_scda_fcontext_t *fc;
  sc_scda_fopen_options_t scda_opt, scda_opt_err;
  sc_scda_ferror_t    errcode;
  size_t              len;
  size_t              elem_count, elem_size;
  size_t              block_size;
  sc_options_t       *opt;
  sc_array_t          data;
  const char         *inline_data = "Test inline data               \n";
  const char         *block_data = "Test block data";
  char                read_data[SC_SCDA_INLINE_FIELD];

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  sc_init (mpicomm, 1, 1, NULL, SC_LP_INFO);

  /* parse command line options */
  opt = sc_options_new (argv[0]);

  sc_options_add_int (opt, 'N', "fuzzy-everyn", &int_everyn, 0,
                      "average fuzzy error return; 0 means no fuzzy returns "
                      "and must be >= 0");
  sc_options_add_int (opt, 'S', "fuzzy-seed", &int_seed, -1,
                      "seed "
                      "for fuzzy error return of scda functions; ignored for "
                      "fuzzy-everyn == 0");

  first_argc =
    sc_options_parse (sc_package_id, SC_LP_DEFAULT, opt, argc, argv);

  if (first_argc < 0 || first_argc != argc) {
    sc_options_print_usage (sc_package_id, SC_LP_ERROR, opt, NULL);
  }

  sc_options_print_summary (sc_package_id, SC_LP_PRODUCTION, opt);

  if (int_everyn < 0) {
    SC_GLOBAL_LERROR ("Usage error: fuzzy-everyn must be >= 0\n");
    sc_options_print_usage (sc_package_id, SC_LP_ERROR, opt, NULL);
    return 1;
  }

  /* Test checking of non-collective fuzzy parameters. */
  mpiret = sc_MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  scda_opt_err.info = sc_MPI_INFO_NULL;
  if (mpirank == 0) {
    scda_opt_err.fuzzy_everyn = 0;
    scda_opt_err.fuzzy_seed = 0;
  }
  else {
    scda_opt_err.fuzzy_everyn = 1;
    scda_opt_err.fuzzy_seed = 0;
  }
  if (mpisize > 1) {
    SC_GLOBAL_ESSENTIAL
      ("We expect two invalid scda function parameter errors."
       " This is just for testing purposes and does not imply"
       " erroneous code behavior.\n");
  }
  /* fopen_write with non-collective fuzzy error parameters */
  fc = sc_scda_fopen_write (mpicomm, filename, file_user_string, NULL,
                            &scda_opt_err, &errcode);
  if (mpisize > 1) {
    SC_CHECK_ABORT (fc == NULL && errcode.scdaret == SC_SCDA_FERR_ARG,
                    "Test fuzzy error parameters check");
  }
  else {
    /* we can not provoke non-collective parameter error in serial */
    SC_CHECK_ABORT (fc != NULL && sc_scda_ferror_is_success (errcode),
                    "Test fuzzy error parameters check in serial");
    sc_scda_fclose (fc, &errcode);
    SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                    "scda_fclose after read failed");
  }
  /* fopen_read with non-collective fuzzy error parameters */
  fc =
    sc_scda_fopen_read (mpicomm, filename, read_user_string, &len,
                        &scda_opt_err, &errcode);
  if (mpisize > 1) {
    SC_CHECK_ABORT (fc == NULL && errcode.scdaret == SC_SCDA_FERR_ARG,
                    "Test fuzzy error parameters check");
  }
  else {
    /* we can not provoke non-collective parameter error in serial */
    SC_CHECK_ABORT (fc != NULL && sc_scda_ferror_is_success (errcode),
                    "Test fuzzy error parameters check in serial");
    sc_scda_fclose (fc, &errcode);
    SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                    "scda_fclose after read failed");
  }

  /* Create valid scda options structure. */
  /* set the options to actiavate fuzzy error testing */
  /* WARNING: Fuzzy error testing means that the code randomly produces
   * errors. Random errors mean in particular that error codes may arise from
   * code places, which can not produce such particular error codes without
   * fuzzy error testing. Nonetheless, our implementation is designed to be
   * able to handle this situations properly.
   */
  scda_opt.fuzzy_everyn = (unsigned) int_everyn;
  if (scda_opt.fuzzy_everyn > 0 && int_seed < 0) {
    scda_opt.fuzzy_seed = (sc_rand_state_t) sc_MPI_Wtime ();
    mpiret =
      sc_MPI_Bcast (&scda_opt.fuzzy_seed, 1, sc_MPI_UNSIGNED, 0, mpicomm);
    SC_CHECK_MPI (mpiret);
    SC_GLOBAL_INFOF ("Fuzzy error return with time-dependent seed activated. "
                     "The seed is %lld.\n", (long long) scda_opt.fuzzy_seed);
  }
  else {
    scda_opt.fuzzy_seed = (sc_rand_state_t) int_seed;
  }
  scda_opt.info = sc_MPI_INFO_NULL;

  fc = sc_scda_fopen_write (mpicomm, filename, file_user_string, NULL,
                            &scda_opt, &errcode);
  /* TODO: check errcode */
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "scda_fopen_write failed");

  /* write a block section to the file */
  block_size = strlen (block_data);
  sc_array_init_data (&data, (void *) block_data, block_size, 1);
  fc = sc_scda_fwrite_block (fc, "Block section test", NULL, &data, block_size,
                             mpisize - 1, 0, &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "scda_fwrite_block failed");

  /* write an inline section to the file */
  sc_array_init_data (&data, (void *) inline_data, SC_SCDA_INLINE_FIELD, 1);
  fc = sc_scda_fwrite_inline (fc, "Inline section test without user-defined "
                              "padding", NULL, &data, mpisize - 1, &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "scda_fwrite_inline failed");

  /* write an inline section with an empty user string */
  fc = sc_scda_fwrite_inline (fc, "", NULL, &data, 0, &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "scda_fwrite_inline with empty user string failed");

  /* write a block section */
  fc = sc_scda_fwrite_block (fc, "A block section with the inline data", NULL,
                             &data, 32, mpisize - 1, 0, &errcode);
  /* TODO: check errcode */
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "scda_fwrite_block failed");

  /* write a fixed-size array section */
  test_scda_write_fixed_size_array (fc, mpirank, mpisize);

  /* intentionally try to write with non-collective block size */
  if (mpisize > 1) {
    SC_GLOBAL_ESSENTIAL
      ("We expect an invalid scda function parameter error."
       " This is just for testing purposes and do not imply"
       " erroneous code behavior.\n");
    fc = sc_scda_fwrite_block (fc, "A block section", NULL, &data,
                              (mpirank == 0) ? 32 : 33, mpisize - 1, 0,
                              &errcode);
    SC_CHECK_ABORT (!sc_scda_ferror_is_success (errcode) &&
                    errcode.scdaret == SC_SCDA_FERR_ARG, "scda_fwrite_block "
                    "check catch non-collective block size");
  }

  if (mpisize == 1) {
    sc_scda_fclose (fc, &errcode);
    /* TODO: check errcode and return value */
    SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                    "scda_fclose after write failed");
  }
  else {
    /* fc was closed due to an intentionally triggered error */
    SC_CHECK_ABORT (fc == NULL, "fc closed after error in fwrite_block");
  }

  fc =
    sc_scda_fopen_read (mpicomm, filename, read_user_string, &len, &scda_opt,
                        &errcode);
  /* TODO: check errcode */
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "scda_fopen_read failed");

  SC_INFOF ("File header user string: %s\n", read_user_string);

  /* read first section header */
  fc = sc_scda_fread_section_header (fc, read_user_string, &len, &section_type,
                                     &elem_count, &elem_size, &decode,
                                     &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "sc_scda_fread_section_header failed");
  SC_CHECK_ABORT (section_type == 'B' && elem_count == 0 &&
                  elem_size == block_size, "Identifying section type");

  /* read block data */
  sc_array_init_data (&data, read_data, block_size, 1);
  fc = sc_scda_fread_block_data (fc, &data, block_size, 0, &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "sc_scda_fread_block_data failed");
  SC_CHECK_ABORT (mpirank != 0
                  || !strncmp (read_data, block_data, block_size),
                  "inline data mismatch");

  SC_INFOF ("Read file section header of type %c with user string: %s\n",
            section_type, read_user_string);

  /* read second section header */
  fc = sc_scda_fread_section_header (fc, read_user_string, &len, &section_type,
                                     &elem_count, &elem_size, &decode,
                                     &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "sc_scda_fread_section_header failed");
  SC_CHECK_ABORT (section_type == 'I' && elem_count == 0 && elem_size == 0,
                  "Identifying section type");

  /* read inline data */
  sc_array_init_data (&data, read_data, SC_SCDA_INLINE_FIELD, 1);
  fc = sc_scda_fread_inline_data (fc, &data, 0, &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "sc_scda_fread_inline_data failed");
  SC_CHECK_ABORT (mpirank != 0
                  || !strncmp (read_data, inline_data,
                               SC_SCDA_INLINE_FIELD), "inline data mismatch");

  SC_INFOF ("Read file section header of type %c with user string: %s\n",
            section_type, read_user_string);

  /* skip the next inline section */
  /* reading the section header can not be skipped */
  fc = sc_scda_fread_section_header (fc, read_user_string, &len, &section_type,
                                     &elem_count, &elem_size, &decode,
                                     &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "sc_scda_fread_section_header for inline failed");
  SC_CHECK_ABORT (section_type == 'I' && elem_count == 0 && elem_size == 0,
                  "Identifying section type");
  fc = sc_scda_fread_inline_data (fc, NULL, mpisize - 1, &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "sc_scda_fread_inline_data skip failed");

  /* read the block section header */
  fc = sc_scda_fread_section_header (fc, read_user_string, &len, &section_type,
                                     &elem_count, &elem_size, &decode,
                                     &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "sc_scda_fread_section_header for block failed");
  SC_CHECK_ABORT (section_type == 'B' && elem_count == 0 && elem_size == 32,
                  "Identifying section type");

  /* read the block data */
  (void) memset (read_data, '\0', SC_SCDA_INLINE_FIELD);
  fc = sc_scda_fread_block_data (fc, &data, SC_SCDA_INLINE_FIELD, mpisize - 1,
                                 &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "sc_scda_fread_block_data failed");
  SC_CHECK_ABORT (mpirank != mpisize - 1
                  || !strncmp (read_data, inline_data,
                               SC_SCDA_INLINE_FIELD), "block data mismatch");

  test_scda_read_fixed_size_array (fc, mpirank, mpisize);

  sc_scda_fclose (fc, &errcode);
  /* TODO: check errcode and return value */
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "scda_fclose after read failed");

  fc =
    sc_scda_fopen_read (mpicomm, filename, read_user_string, &len, &scda_opt,
                        &errcode);
  SC_CHECK_ABORT (sc_scda_ferror_is_success (errcode),
                  "scda_fopen_read failed");

  /* provoke error for invalid scda workflow */
  SC_GLOBAL_ESSENTIAL ("We expect an error for incorrect workflow for scda"
                       " reading function, which is triggered on purpose to"
                       " test the error checking.\n");
  fc = sc_scda_fread_inline_data (fc, &data, 0, &errcode);
  SC_CHECK_ABORT (!sc_scda_ferror_is_success (errcode) &&
                  errcode.scdaret == SC_SCDA_FERR_USAGE && fc == NULL,
                  "sc_scda_fread_section_header error detection failed");
  /* fc is closed and deallocated due to the occurred error  */

  sc_options_destroy (opt);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
