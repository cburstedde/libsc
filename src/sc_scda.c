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
#include <sc_io.h>

/* file section header data */
#define SC_SCDA_MAGIC "scdata0" /**< magic encoding format identifier and version */
#define SC_SCDA_MAGIC_BYTES 7   /**< number of magic bytes */
#define SC_SCDA_VENDOR_STRING "libsc" /**< implementation defined data */
#define SC_SCDA_VENDOR_STRING_FIELD 24 /**< byte count for vendor string entry
                                            including the padding */
#define SC_SCDA_USER_STRING_FILED 62   /**< byte count for user string entry
                                            including the padding */
#define SC_SCDA_PADDING_MOD 32  /**< divisor for variable lenght padding */

/** The opaque file context for for scda files. */
struct sc_scda_fcontext
{
  /* *INDENT-OFF* */
  sc_MPI_Comm         mpicomm; /**< associated MPI communicator */
  int                 mpisize; /**< number of MPI ranks */
  int                 mpirank; /**< MPI rank */
  sc_MPI_File         file;    /**< file object */
  /* *INDENT-ON* */
};

/** Pad the input data to a fixed length.
 *
 * The result is written to \b output_data, which must be allocated.
 *
 */
static void
sc_scda_pad_to_fix_len (const char *input_data, size_t input_len,
                        char *output_data, size_t pad_len)
{
  SC_ASSERT (input_data != NULL);
  SC_ASSERT (input_len <= pad_len - 4);

  void               *pointer;
#if 0
  uint8_t            *byte_arr;
#endif

  /* We assume that output_data has at least pad_len allocated bytes. */

  /* copy input data into output_data */
  pointer = memcpy (output_data, input_data, input_len);
  SC_EXECUTE_ASSERT_TRUE (pointer == (void *) output_data);

  /* append padding */
#if 0
  byte_arr = (uint8_t *) padded_data;
#endif
  output_data[input_len] = ' ';
  memset (&output_data[input_len + 1], '-', pad_len - input_len - 2);
  output_data[pad_len - 1] = '\n';
}

static void
sc_scda_pad_to_mod (const char *input_data, size_t input_len,
                    char *output_data)
{
  SC_ASSERT (input_len == 0 || input_data != NULL);
  SC_ASSERT (output_data != NULL);

  int                 num_pad_bytes;
  void               *pointer;

  /* compute the number of padding bytes */
  num_pad_bytes =
    (SC_SCDA_PADDING_MOD -
     ((int) input_len % SC_SCDA_PADDING_MOD)) % SC_SCDA_PADDING_MOD;

  if (num_pad_bytes < 7) {
    /* not sufficient number of padding bytes for the padding format */
    num_pad_bytes += SC_SCDA_PADDING_MOD;
  }

  SC_ASSERT (num_pad_bytes >= 6);
  SC_ASSERT (num_pad_bytes <= SC_SCDA_PADDING_MOD + 6);

  pointer = memcpy (output_data, input_data, input_len);
  SC_EXECUTE_ASSERT_TRUE (pointer == (void *) output_data);

  /* check for last byte to decide on padding format */
  if (input_len > 0 && input_data[input_len - 1] == '\n') {
    /* input data ends with a line break */
    output_data[input_len] = '=';
  }
  else {
    /* add a line break add the beginning of the padding */
    output_data[input_len] = '\n';
  }
  output_data[input_len + 1] = '=';

  /* append the remaining padding bytes */
  memset (&output_data[input_len + 2], '=', num_pad_bytes - 4);
  output_data[input_len + num_pad_bytes - 2] = '\n';
  output_data[input_len + num_pad_bytes - 1] = '\n';
}

sc_scda_fcontext_t *
sc_scda_fopen_write (sc_MPI_Comm mpicomm,
                     const char *filename,
                     const char *user_string, size_t *len,
                     sc_scda_fopen_options_t * opt,
                     sc_scda_ferror_t * errcode)
{
  SC_ASSERT (filename != NULL);
  SC_ASSERT (user_string != NULL);
  SC_ASSERT (len != NULL);
  SC_ASSERT (errcode != NULL);

  int                 mpiret;
  sc_MPI_Info         info;
  sc_scda_fcontext_t *fc;

  /* TODO: check length of the filename */
  /* We assume the filename to be nul-terminated. */

  /* TODO: check the user string; implement a helper function for this */

  /* TODO: Check options if opt is valid? */

  /* allocate the file context */
  fc = SC_ALLOC (sc_scda_fcontext_t, 1);

  /* examine options */
  if (opt != NULL) {
    info = opt->info;
  }
  else {
    info = sc_MPI_INFO_NULL;
  }

  /* fill convenience MPI information */
  mpiret = sc_MPI_Comm_size (mpicomm, &fc->mpisize);
  SC_CHECK_MPI (mpiret);

  mpiret = sc_MPI_Comm_rank (mpicomm, &fc->mpirank);
  SC_CHECK_MPI (mpiret);

  /* open the file for writing */
  mpiret =
    sc_io_open (mpicomm, filename, SC_IO_WRITE_CREATE, info, &fc->file);
  /* TODO: check return value */

  if (fc->mpirank == 0) {
    int                 count;
    int                 current_len;
    char                file_header_data[SC_SCDA_HEADER_BYTES];

    /* get scda file header section */
    /* magic */
    memcpy (file_header_data, SC_SCDA_MAGIC, SC_SCDA_MAGIC_BYTES);
    current_len = SC_SCDA_MAGIC_BYTES;
    /* TODO: check return value */

    file_header_data[current_len++] = ' ';

    /* vendor string */
    sc_scda_pad_to_fix_len (SC_SCDA_VENDOR_STRING,
                            strlen (SC_SCDA_VENDOR_STRING),
                            &file_header_data[current_len],
                            SC_SCDA_VENDOR_STRING_FIELD);
    current_len += SC_SCDA_VENDOR_STRING_FIELD;

    /* file section specifying character */
    file_header_data[current_len++] = 'F';
    file_header_data[current_len++] = ' ';

    /* user string */
    sc_scda_pad_to_fix_len (user_string, *len,
                            &file_header_data[current_len],
                            SC_SCDA_USER_STRING_FILED);

    /* pad the file header section */

    /* write scda file header section */
    mpiret =
      sc_io_write_at (fc->file, 0, file_header_data, SC_SCDA_HEADER_BYTES,
                      sc_MPI_BYTE, &count);
    /* TODO: check return value and count */
  }

  return fc;
}

int
sc_scda_fclose (sc_scda_fcontext_t * fc, sc_scda_ferror_t * errcode)
{
  SC_ASSERT (fc != NULL);
  SC_ASSERT (errcode != NULL);

  int                 retval;

  /* TODO: further checks before calling sc_io_close? */

  retval = sc_io_close (&fc->file);
  /* TODO: handle return value */

  SC_FREE (fc);

  return 0;
}
