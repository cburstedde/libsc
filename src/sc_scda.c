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
#define SC_SCDA_VENDOR_STRING_BYTES 20 /**< maximal number of vendor string bytes */
#define SC_SCDA_USER_STRING_FIELD 62   /**< byte count for user string entry
                                            including the padding */
#define SC_SCDA_PADDING_MOD 32  /**< divisor for variable length padding */

/** get a random double in the range [A,B) */
#define SC_SCDA_RAND_RANGE(A, B, state) ((A) + sc_rand (state) * ((B) - (A)))

/** get a random int in the range [A,B) */
#define SC_SCDA_RAND_RANGE_INT(A, B, state) ((int) SC_SCDA_RAND_RANGE (A, B,  \
                                                                       state))

/** Examine scda file return value and print an error message in case of error.
 * The parameter msg is prepended to the file, line and error information.
 * This is a general macro that gets the log handler as a parameter.
 */
#define SC_SCDA_CHECK_VERBOSE_GEN(errcode, msg, sc_scda_log) do {             \
                                    char sc_scda_msg[sc_MPI_MAX_ERROR_STRING];\
                                    int sc_scda_len, sc_scda_retval;          \
                                    if (!sc_scda_ferror_is_success (errcode)) {\
                                    sc_scda_retval =                          \
                                    sc_scda_ferror_string (errcode, sc_scda_msg,\
                                                           &sc_scda_len);     \
                                    SC_ASSERT(sc_scda_retval !=               \
                                              SC_SCDA_FERR_ARG);              \
                                    if (sc_scda_retval == SC_SCDA_FERR_SUCCESS){\
                                    sc_scda_log ("%s at %s:%d: %*.*s\n",      \
                                                 msg,  __FILE__, __LINE__,    \
                                                 sc_scda_len, sc_scda_len,    \
                                                 sc_scda_msg);}               \
                                    else {                                    \
                                    sc_scda_log ("%s at %s:%d: %s\n",         \
                                                 msg, __FILE__, __LINE__,     \
                                                 "An error occurred but "     \
                                                 "ferror_string failed");     \
                                    }}} while (0)

/** For collective calls. Cf. \ref SC_SCDA_CHECK_VERBOSE_GEN.
 */
#define SC_SCDA_CHECK_VERBOSE_COLL(errcode, msg) SC_SCDA_CHECK_VERBOSE_GEN ( \
                                    errcode, msg, SC_GLOBAL_LERRORF)

/** For non-collective calls. Cf. \ref SC_SCDA_CHECK_VERBOSE_GEN.
 */
#define SC_SCDA_CHECK_VERBOSE_NONCOLL(errcode, msg) SC_SCDA_CHECK_VERBOSE_GEN (\
                                    errcode, msg, SC_LERRORF)

/** Collectivly check a given errorcode.
 * This macro assumes that errcode is a collective
 * variable and that the macro is called collectivly.
 * The calling function must return NULL in case of an error.
 */
#define SC_SCDA_CHECK_COLL_ERR(errcode, fc, user_msg) do {                   \
                                    SC_SCDA_CHECK_VERBOSE_COLL (*errcode,    \
                                                                user_msg);   \
                                    if (!sc_scda_ferror_is_success (*errcode)) {\
                                    sc_scda_file_error_cleanup (&fc->file);  \
                                    SC_FREE (fc);                            \
                                    return NULL;}} while (0)

/* This macro is suitable to be called after a non-collective operation.
 * For a correct error handling it is required to skip the rest
 * of the non-collective code and then broadcast the error flag.
 * The macro can be used multiple times in a function but will always jump to
 * the end of the calling function that must be a void function.
 */
#define SC_SCDA_CHECK_NONCOLL_ERR(errcode, user_msg) do {                    \
                                    SC_SCDA_CHECK_VERBOSE_NONCOLL (*errcode, \
                                                                   user_msg);\
                                    if (!sc_scda_ferror_is_success (*errcode)) {\
                                    return;}} while (0)

/** Handle a non-collective error.
 * Use this macro after \ref SC_SCDA_CHECK_NONCOLL_ERR *directly* after the end
 * of non-collective statements.
 * Can be only used once in a function.
 */
#define SC_SCDA_HANDLE_NONCOLL_ERR(errcode, fc) do{                            \
                                    SC_CHECK_MPI(sc_MPI_Bcast(&errcode->scdaret,\
                                                  1, sc_MPI_INT, 0,           \
                                                  fc->mpicomm));               \
                                    SC_CHECK_MPI(sc_MPI_Bcast(&errcode->mpiret,\
                                                  1, sc_MPI_INT, 0,            \
                                                  fc->mpicomm));               \
                                    if (!sc_scda_ferror_is_success (*errcode)) {\
                                    sc_scda_file_error_cleanup (&fc->file);    \
                                    SC_FREE (fc);                              \
                                    return NULL;}} while (0)

/** Check for a count error of a serial (rank 0) I/O operation.
 * This macro is only valid to use after checking that there was not I/O error.
 * For a correct error handling it is required to skip the rest
 * of the non-collective code and then broadcast the count  error flag.
 * The macro can be used multiple times in a function but will always jump to
 * the end of the calling function that must be a void function.
 * \b cerror must be a pointer to an int that is passed to the subsequent call
 * \ref SC_SCDA_HANDLE_NONCOLL_COUNT_ERR.
 */
#define SC_SCDA_CHECK_NONCOLL_COUNT_ERR(icount, ocount, cerror) do {                   \
                                    *cerror = ((int) icount) != ocount; \
                                    if (*cerror) {                      \
                                    SC_LERRORF ("Count error on rank 0 at "    \
                                                "%s:%d.\n", __FILE__, __LINE__);\
                                    return;}} while (0)

/** Handle a non-collective count error.
 * Use this macro after \ref SC_SCDA_CHECK_NONCOLL_COUNT_ERR *directly* after
 * the end of the non-collective statements but after \ref
 * SC_SCDA_HANDLE_NONCOLL_ERR, which must be executed first.
 * Can be used only once in a function.
 * On rank 0 \b cerror must point to the int that was set by \ref
 * SC_SCDA_CHECK_NONCOLL_COUNT_ERR. On all other ranks \b cerror is set by this
 * macro.
 */
#define SC_SCDA_HANDLE_NONCOLL_COUNT_ERR(errorcode, cerror, fc) do{            \
                                    SC_CHECK_MPI (sc_MPI_Bcast (cerror,        \
                                                  1, sc_MPI_INT, 0,            \
                                                  fc->mpicomm));               \
                                    sc_scda_scdaret_to_errcode (               \
                                        *cerror ? SC_SCDA_FERR_COUNT :         \
                                                         SC_SCDA_FERR_SUCCESS, \
                                        errorcode, fc);                        \
                                    SC_SCDA_CHECK_VERBOSE_NONCOLL (*errorcode, \
                                                    "Read/write count check"); \
                                    if (*cerror) {                             \
                                    sc_scda_file_error_cleanup (&fc->file);    \
                                    SC_FREE (fc);                              \
                                    return NULL;}} while (0)

/** The opaque file context for for scda files. */
struct sc_scda_fcontext
{
  /* *INDENT-OFF* */
  sc_MPI_Comm         mpicomm; /**< associated MPI communicator */
  int                 mpisize; /**< number of MPI ranks */
  int                 mpirank; /**< MPI rank */
  sc_MPI_File         file;    /**< file object */
  unsigned            fuzzy_everyn; /**< In average every n-th possible error
                                       origin returns a fuzzy error. There may
                                       be multiple possible error origins in
                                       one top-level scda function.
                                       We return for each possible error origin
                                       a random error with the empirical
                                       probability of 1 / fuzzy_everyn but only
                                       if the respective possible error origin
                                       did not already cause an error without
                                       the fuzzy error return. In such a case,
                                       the actual error is returned. 0 means
                                       that there are no fuzzy error returns. */
  sc_rand_state_t     fuzzy_seed; /**< The seed for the fuzzy error return.
                                       This value is ignored if
                                       fuzzy_everyn == 0. It is important to
                                       notice that fuzzy_seed is initialized by
                                       the user-defined seed but is adjusted
                                       to the state after each call of
                                       \ref sc_rand. */
  /* *INDENT-ON* */
};

/** Copy \b src to \b dest.
 * \b dest must have at least \b n bytes.
 */
static void
sc_scda_copy_bytes (char *dest, const char *src, size_t n)
{
  SC_ASSERT (dest != NULL);
  SC_ASSERT (n == 0 || src != NULL);

  if (n == 0) {
    return;
  }

  (void) memcpy (dest, src, n);
}

/** Set \b n bytes in \b dest to \b c.
 * \b dest must have at least \b n bytes.
 */
static void
sc_scda_set_bytes (char *dest, int c, size_t n)
{
  SC_ASSERT (dest != NULL);

  if (n == 0) {
    return;
  }

  (void) memset (dest, c, n);
}

/** A convenience function that calls \ref sc_scda_set_bytes with c = '\0'.
 */
static void
sc_scda_init_nul (char *dest, size_t len)
{
  SC_ASSERT (dest != NULL);

  sc_scda_set_bytes (dest, '\0', len);
}

/** Pad the input data to a fixed length.
 *
 * \param [in]  input_data    The data that is padded. \b input_len many bytes.
 * \param [in]  input_len     The length of \b input_len in number of bytes.
 *                            \b input_len must be less or equal to
 *                            \b pad_len - 4.
 * \param [out] output_data   On output the padded data. \b pad_len many bytes.
 * \param [in]  pad_len       The target padding length and hence the length of
 *                            \b output_data in number of bytes.
 */
static void
sc_scda_pad_to_fix_len (const char *input_data, size_t input_len,
                        char *output_data, size_t pad_len)
{
#if 0
  uint8_t            *byte_arr;
#endif

  SC_ASSERT (input_data != NULL);
  SC_ASSERT (output_data != NULL);
  SC_ASSERT (input_len <= pad_len - 4);

  /* We assume that output_data has at least pad_len allocated bytes. */

  /* copy input data into output_data */
  sc_scda_copy_bytes (output_data, input_data, input_len);

  /* append padding */
#if 0
  byte_arr = (uint8_t *) padded_data;
#endif
  output_data[input_len] = ' ';
  sc_scda_set_bytes (&output_data[input_len + 1], '-',
                     pad_len - input_len - 2);
  output_data[pad_len - 1] = '\n';
}

/** This function checks if \b padded_data is actually padded to \b pad_len.
 * Moreover, the raw data is extracted.
 *
 * \param [in]  padded_data   The padded data.
 * \param [in]  pad_len       The length of \b padded_data in number of bytes.
 * \param [out] raw_data      On output the raw data extracted from
 *                            \b padded_data. The byte count of \b raw_data
 *                            is \b raw_len. \b raw_data must be at least
 *                            \b pad_len - 4. Undefined data if the function
 *                            returns true.
 * \param [out] raw_len       The length of \b raw_data in number of bytes.
 *                            Undefined if the function returns true.
 * \return                    True if \b padded_data does not satisfy the
 *                            scda padding convention for fixed-length paddding.
 *                            False, otherwise.
 */
static int
sc_scda_get_pad_to_fix_len (char *padded_data, size_t pad_len, char *raw_data,
                            size_t *raw_len)
{
  size_t              si;

  SC_ASSERT (padded_data != NULL);
  SC_ASSERT (raw_data != NULL);
  SC_ASSERT (raw_len != NULL);

  if (pad_len < 4) {
    /* data too short to satisfy padding */
    return -1;
  }

  if (padded_data[pad_len - 1] != '\n') {
    /* wrong termination */
    return -1;
  }

  for (si = pad_len - 2; si != 0; --si) {
    if (padded_data[si] != '-') {
      break;
    }
  }
  if (padded_data[si] != ' ') {
    return -1;
  }

  /* the padding was valid and the remaining data is the actual data */
  *raw_len = si;
  sc_scda_copy_bytes (raw_data, padded_data, *raw_len);

  return 0;
}

/** Get the number of padding bytes with respect to the padding of \ref
 *  sc_scda_pad_to_mod.
 *
 */
static size_t
sc_scda_pad_to_mod_len (size_t input_len)
{
  size_t              num_pad_bytes;

  /* compute the number of padding bytes */
  num_pad_bytes =
    (SC_SCDA_PADDING_MOD -
     (input_len % SC_SCDA_PADDING_MOD)) % SC_SCDA_PADDING_MOD;

  if (num_pad_bytes < 7) {
    /* not sufficient number of padding bytes for the padding format */
    num_pad_bytes += SC_SCDA_PADDING_MOD;
  }

  return num_pad_bytes;
}

/** Pad data to a length that is congurent to 0 modulo \ref SC_SCDA_PADDING_MOD.
 *
 * \param [in]  input_data  The input data. At least \b input_len bytes.
 * \param [in]  input_len   The length of \b input_data in number of bytes.
 * \param [out] output_data On output the padded input data. Must be at least
 *                          \ref sc_scda_pad_to_mod_len (\b input_len) bytes.
 */
static void
sc_scda_pad_to_mod (const char *input_data, size_t input_len,
                    char *output_data)
{
  size_t              num_pad_bytes;

  SC_ASSERT (input_len == 0 || input_data != NULL);
  SC_ASSERT (output_data != NULL);

  /* compute the number of padding bytes */
  num_pad_bytes = sc_scda_pad_to_mod_len (input_len);

  SC_ASSERT (num_pad_bytes >= 6);
  SC_ASSERT (num_pad_bytes <= SC_SCDA_PADDING_MOD + 6);

  sc_scda_copy_bytes (output_data, input_data, input_len);

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
  sc_scda_set_bytes (&output_data[input_len + 2], '=', num_pad_bytes - 4);
  output_data[input_len + num_pad_bytes - 2] = '\n';
  output_data[input_len + num_pad_bytes - 1] = '\n';
}

/** Checks if \b padded_data is actually padded with respect to
 *  \ref SC_SCDA_PADDING_MOD.
 *
 * Given the raw data length, this function checks if the padding format is
 * correct and extracts the raw data.
 *
 * \param [in]  padded_data   The padded data with byte count \b padded_len.
 * \param [in]  padded_len    The length of \b padded_data in number of bytes.
 *                            Must be at least 7.
 * \param [in]  raw_len       The length of the raw data in \b padded_data
 *                            in number of bytes. This value must be known by
 *                            the user.
 * \param [out] raw_data      On output the raw data from \b padded_data if
 *                            the function returns true and undefined otherwise.
 * \return                    True if \b padded_data does not satisfy the scda
 *                            padding convention for padding to a modulo
 *                            condition. False, otherwise.
 */
static int
sc_scda_get_pad_to_mod (char *padded_data, size_t padded_len, size_t raw_len,
                        char *raw_data)
{
  size_t              si;
  size_t              num_pad_bytes;

  SC_ASSERT (padded_data != NULL);
  SC_ASSERT (raw_len == 0 || raw_data != NULL);

  num_pad_bytes = sc_scda_pad_to_mod_len (raw_len);

  /* check if padding data length conforms to the padding format */
  if (num_pad_bytes + raw_len != padded_len) {
    /* raw_len and padded_len are not consistent */
    return -1;
  }
  SC_ASSERT (padded_len >= 7);

  /* check the content of the padding bytes */
  if (padded_data[padded_len - 1] != '\n' ||
      padded_data[padded_len - 2] != '\n') {
    /* terminating line breaks are missing */
    return -1;
  }

  for (si = padded_len - 3; si != padded_len - num_pad_bytes; --si) {
    if (padded_data[si] != '=') {
      /* wrong padding character */
      return -1;
    }
  }
  SC_ASSERT (si == raw_len);

  if ((!((padded_data[si] == '=' && raw_len != 0 &&
          padded_data[si - 1] == '\n') || padded_data[si] == '\n'))) {
    /* wrong padding start */
    return -1;
  }

  if (raw_len != 0) {
    /* get the raw data if we required raw_data != NULL */
    sc_scda_copy_bytes (raw_data, padded_data, raw_len);
  }

  return 0;
}

/**
 * This function is for creating and reading a file.
 * The passed \b fc must have filled MPI information (cf. \ref
 * sc_scda_fill_mpi_data).
 * The function returns \ref SC_SCDA_FERR_ARG if everyn and/or seed in opt are
 * not collective. Otherwise, the function returns \ref SC_SCDA_FERR_SUCCESS.
 */
static sc_scda_ret_t
sc_scda_examine_options (sc_scda_fopen_options_t * opt, sc_scda_fcontext_t *fc,
                         sc_MPI_Info *info)
{
  SC_ASSERT (fc != NULL);

  if (opt != NULL) {
    int                 mpiret;
    int                 local_fuzzy_params_cmp, collective_fuzzy_params;
    /* byte buffer since it is not clear which data type is larger */
    /* we use a char array as a byte buffer, cf. \ref sc_array_index */
    char                buf[sizeof (unsigned) + sizeof (sc_rand_state_t)];
    unsigned            bcast_everyn;
    sc_rand_state_t     bcast_seed;

    /* check if fuzzy_everyn and fuzzy_seed are collective */

    /* copy fuzzy parameters to byte buffer */
    sc_scda_copy_bytes (buf, (char *) &opt->fuzzy_everyn, sizeof (unsigned));
    sc_scda_copy_bytes (&buf[sizeof (unsigned)], (char *) &opt->fuzzy_seed,
                        sizeof (sc_rand_state_t));

    /* For the sake of simplicity, we use a Bcast followed by an Allreduce
     * instead of one Allreduce call with a custom reduction function.
     */
    mpiret = sc_MPI_Bcast (buf, sizeof (unsigned) + sizeof (sc_rand_state_t),
                           sc_MPI_BYTE, 0, fc->mpicomm);
    SC_CHECK_MPI (mpiret);

    /* get actual data from the byte buffer */
    bcast_everyn = *((unsigned *) buf);
    bcast_seed = *((sc_rand_state_t *) & buf[sizeof (unsigned)]);

    /* compare fuzzy parameters */
    local_fuzzy_params_cmp = bcast_everyn != opt->fuzzy_everyn
      || bcast_seed != opt->fuzzy_seed;

    /* synchronize comparison results */
    mpiret = sc_MPI_Allreduce (&local_fuzzy_params_cmp,
                               &collective_fuzzy_params, 1, sc_MPI_INT,
                               sc_MPI_LOR, fc->mpicomm);
    SC_CHECK_MPI (mpiret);

    if (collective_fuzzy_params) {
      /* non-collective fuzzy parameters */
      /* no fuzzy error testing in case of an error */
      fc->fuzzy_everyn = 0;
      fc->fuzzy_seed = 0;
      return SC_SCDA_FERR_ARG;
    }

    *info = opt->info;
    fc->fuzzy_everyn = opt->fuzzy_everyn;
    fc->fuzzy_seed = opt->fuzzy_seed;
  }
  else {
    *info = sc_MPI_INFO_NULL;
    /* no fuzzy error return by default */
    fc->fuzzy_everyn = 0;
    fc->fuzzy_seed = 0;
  }

  return SC_SCDA_FERR_SUCCESS;
}

static void
sc_scda_fill_mpi_data (sc_scda_fcontext_t * fc, sc_MPI_Comm mpicomm)
{
  int                 mpiret;

  SC_ASSERT (fc != NULL);

  mpiret = sc_MPI_Comm_size (mpicomm, &fc->mpisize);
  SC_CHECK_MPI (mpiret);

  mpiret = sc_MPI_Comm_rank (mpicomm, &fc->mpirank);
  SC_CHECK_MPI (mpiret);

  fc->mpicomm = mpicomm;
}

/** Get the user string length for writing.
 *
 * \param [in]  user_string  A user string that can be a C string or a binary
 *                           user string.
 * \param [in]  in_len       Must be NULL for \b user_string being a C string
 *                           and must point to the number of bytes of
 *                           \b user_string excluding the terminating nul for a
 *                           binary user string. If \b len is greater than
 *                           \b SC_SCDA_USER_STRING_BYTES, this function returns
 *                           true.
 * \param [out] out_len      On output the length of \b user_string in number
 *                           of bytes excluding the terminating nul if the
 *                           function returns false. Otherwise \b out_len is
 *                           undefined.
 * \return                   True if \b user_string is not compliant with the
 *                           scda file format, i.e. too long or missing nul
 *                           termination. False, otherwise.
 */
static int
sc_scda_get_user_string_len (const char *user_string,
                             const size_t *in_len, size_t *out_len)
{
  SC_ASSERT (user_string != NULL);
  SC_ASSERT (out_len != NULL);

  if (in_len != NULL) {
    /* binary user string */
    if (*in_len > SC_SCDA_USER_STRING_BYTES) {
      /* binary user string is too long */
      return -1;
    }

    if (user_string[*in_len] != '\0') {
      /* missing nul-termination */
      return -1;
    }

    *out_len = *in_len;
    return 0;
  }
  else {
    /* we expect a nul-terminated C string */
    char                user_string_copy[SC_SCDA_USER_STRING_BYTES + 1];
    size_t              len;

    sc_strcopy (user_string_copy, SC_SCDA_USER_STRING_BYTES + 1, user_string);

    /* user_string_copy is guaranteed to be nul-terminated */
    len = strlen (user_string_copy);
    if (len < SC_SCDA_USER_STRING_BYTES) {
      /* user string is nul-terminated */
      *out_len = len;
      return 0;
    }

    SC_ASSERT (len == SC_SCDA_USER_STRING_BYTES);

    /* check for nul at last byte position of user string */
    if (user_string[SC_SCDA_USER_STRING_BYTES - 1] == '\0') {
      *out_len = len;
      return 0;
    }

    /* user string is not nul-terminated */
    return -1;
  }

  /* unreachable */
  SC_ABORT_NOT_REACHED ();
}

/** Draw a sample for the proability 1/ \b everyn.
 */
static int
sc_scda_sample_everyn (unsigned everyn, sc_rand_state_t *state)
{
  SC_ASSERT (state != NULL);

  return sc_rand (state) < 1. / (double) everyn;
}

/** Create a random but consistent scdaret.
 */
static sc_scda_ret_t
sc_scda_get_fuzzy_scdaret (unsigned everyn, sc_rand_state_t *state)
{
  sc_scda_ret_t       sample;

  SC_ASSERT (everyn != 0);

  /* draw an error with the empirical probalilty of 1 / freq */
  if (sc_scda_sample_everyn (everyn, state)) {
    /* draw an error  */
    sample = (sc_scda_ret_t)
      SC_SCDA_RAND_RANGE_INT (SC_SCDA_FERR_FORMAT, SC_SCDA_FERR_LASTCODE,
                              state);
  }
  else {
    sample = SC_SCDA_FERR_SUCCESS;
  }

  return sample;
}

static int
sc_scda_get_fuzzy_mpiret (unsigned everyn, sc_rand_state_t *state)
{
  int                 index_sample, sample;

  SC_ASSERT (everyn != 0);
  SC_ASSERT (state != NULL);

  /* draw an error with the empirical probalilty of 1 / everyn */
  if (sc_scda_sample_everyn (everyn, state)) {
    /* draw an MPI 2.0 I/O error */
    /* The MPI standard does not guarantee that the MPI error codes
     * are contiguous. Hence, take the minimal available error code set and
     * draw an index into this set.
     */
    index_sample = SC_SCDA_RAND_RANGE_INT (0, 16, state);

    /* map int 0...15 to an MPI 2.0 I/O error code */
    switch (index_sample) {
    case 0:
      sample = sc_MPI_ERR_FILE;
      break;
    case 1:
      sample = sc_MPI_ERR_NOT_SAME;
      break;
    case 2:
      sample = sc_MPI_ERR_AMODE;
      break;
    case 3:
      sample = sc_MPI_ERR_UNSUPPORTED_DATAREP;
      break;
    case 4:
      sample = sc_MPI_ERR_UNSUPPORTED_OPERATION;
      break;
    case 5:
      sample = sc_MPI_ERR_NO_SUCH_FILE;
      break;
    case 6:
      sample = sc_MPI_ERR_FILE_EXISTS;
      break;
    case 7:
      sample = sc_MPI_ERR_BAD_FILE;
      break;
    case 8:
      sample = sc_MPI_ERR_ACCESS;
      break;
    case 9:
      sample = sc_MPI_ERR_NO_SPACE;
      break;
    case 10:
      sample = sc_MPI_ERR_QUOTA;
      break;
    case 11:
      sample = sc_MPI_ERR_READ_ONLY;
      break;
    case 12:
      sample = sc_MPI_ERR_FILE_IN_USE;
      break;
    case 13:
      sample = sc_MPI_ERR_DUP_DATAREP;
      break;
    case 14:
      sample = sc_MPI_ERR_CONVERSION;
      break;
    case 15:
      sample = sc_MPI_ERR_IO;
      break;
    default:
      SC_ABORT_NOT_REACHED ();
      break;
    }
  }
  else {
    sample = sc_MPI_SUCCESS;
  }

  return sample;
}

/** Converts a scdaret error code into a sc_scda_ferror_t code.
 *
 * This function must be used in the case of success as well to ensure full
 * coverage for the fuzzy error testing.
 */
static void
sc_scda_scdaret_to_errcode (sc_scda_ret_t scda_ret,
                            sc_scda_ferror_t * scda_errorcode,
                            sc_scda_fcontext_t * fc)
{
  sc_scda_ret_t       scda_ret_internal;
  int                 mpiret_internal;

  SC_ASSERT (SC_SCDA_FERR_SUCCESS <= scda_ret &&
             scda_ret < SC_SCDA_FERR_LASTCODE);
  SC_ASSERT (scda_errorcode != NULL);

  /* if we have an MPI error; we need \ref sc_scda_mpiret_to_errcode */
  SC_ASSERT (scda_ret != SC_SCDA_FERR_MPI);

  if (scda_ret != SC_SCDA_FERR_SUCCESS) {
    /* an error happened and we do not return fuzzy in any case */
    scda_ret_internal = scda_ret;
    mpiret_internal = sc_MPI_SUCCESS;
  }
  else {
    /* no error occurred, we may return fuzzy */
    scda_ret_internal =
      (fc->fuzzy_everyn == 0) ? scda_ret :
      sc_scda_get_fuzzy_scdaret (fc->fuzzy_everyn, &fc->fuzzy_seed);
    if (scda_ret_internal == SC_SCDA_FERR_MPI) {
      SC_ASSERT (fc->fuzzy_everyn > 0);
      /* we must draw an MPI error */
      /* frequency 1 since we need mpiret != sc_MPI_SUCCESS */
      mpiret_internal = sc_scda_get_fuzzy_mpiret (1, &fc->fuzzy_seed);
    }
    else {
      mpiret_internal = sc_MPI_SUCCESS;
    }
  }

  scda_errorcode->scdaret = scda_ret_internal;
  scda_errorcode->mpiret = mpiret_internal;
}

/** Converts an MPI or libsc error code into a sc_scda_ferror_t code.
 *
 * This function must be used in the case of success as well to ensure full
 * coverage for the fuzzy error testing.
 */
static void
sc_scda_mpiret_to_errcode (int mpiret, sc_scda_ferror_t * scda_errorcode,
                           sc_scda_fcontext_t * fc)
{
  sc_scda_ret_t       scda_ret_internal;
  int                 mpiret_internal;

  SC_ASSERT ((sc_MPI_SUCCESS <= mpiret && mpiret < sc_MPI_ERR_LASTCODE));
  SC_ASSERT (scda_errorcode != NULL);

  if (fc->fuzzy_everyn == 0) {
    /* no fuzzy errors */
    scda_ret_internal =
      (mpiret == sc_MPI_SUCCESS) ? SC_SCDA_FERR_SUCCESS : SC_SCDA_FERR_MPI;
    mpiret_internal = mpiret;
  }
  else {
    /* fuzzy error testing but only if the actual call was successful */
    mpiret_internal =
      (mpiret ==
       sc_MPI_SUCCESS) ? sc_scda_get_fuzzy_mpiret (fc->fuzzy_everyn,
                                                   &fc->fuzzy_seed) : mpiret;
    scda_ret_internal =
      (mpiret_internal ==
       sc_MPI_SUCCESS) ? SC_SCDA_FERR_SUCCESS : SC_SCDA_FERR_MPI;
  }

  scda_errorcode->scdaret = scda_ret_internal;
  scda_errorcode->mpiret = mpiret_internal;
}

/** Check if an error code is valid. */
static int
sc_scda_errcode_is_valid (sc_scda_ferror_t errcode)
{
  /* check the value ranges */
  if (!(SC_SCDA_FERR_SUCCESS <= errcode.scdaret
        && errcode.scdaret < SC_SCDA_FERR_LASTCODE)) {
    return 0;
  }

  if (!(sc_MPI_SUCCESS <= errcode.mpiret
        && errcode.mpiret < sc_MPI_ERR_LASTCODE)) {
    return 0;
  }

  /* check case of no MPI/MPI-replacement error */
  if (!(errcode.scdaret == SC_SCDA_FERR_MPI
        || errcode.mpiret == sc_MPI_SUCCESS)) {
    return 0;
  }

  /* check case of an MPI/MPI-replacement error */
  if (!(errcode.scdaret != SC_SCDA_FERR_MPI
        || errcode.mpiret != sc_MPI_SUCCESS)) {
    return 0;
  }

  /* check case of no scdaret error */
  if (!(errcode.scdaret != SC_SCDA_FERR_SUCCESS
        || errcode.mpiret == sc_MPI_SUCCESS)) {
    return 0;
  }

  return 1;
}

int
sc_scda_ferror_is_success (sc_scda_ferror_t errorcode)
{
  SC_ASSERT (sc_scda_errcode_is_valid (errorcode));

  return !errorcode.scdaret && !errorcode.mpiret;
}

/** Close an MPI file or its libsc-internal replacement in case of an error.
 * \param [in,out]  file    A sc_MPI_file
 * \return                  Always -1 since this function is only called
 *                          if an error already occurred.
 */
static int
sc_scda_file_error_cleanup (sc_MPI_File * file)
{
  /* no error checking since we are called under an error condition */
  SC_ASSERT (file != NULL);
  if (*file != sc_MPI_FILE_NULL) {
    /* We do not use here the libsc closing function since we do not perform
     * error checking in this function that is only called if we had already
     * an error.
     */
#ifdef SC_ENABLE_MPIIO
    MPI_File_close (file);
#else
    fclose ((*file)->file);
    (*file)->file = NULL;
#endif
  }
#ifndef SC_ENABLE_MPIIO
  SC_FREE (*file);
#endif
  return -1;
}

/** This function peforms the start up for both scda fopen functions.
 *
 * \param [in]   opt        The sc_scda_fopen_options_t structure that is
 *                          passed to the scda fopen function. \b opt may be
 *                          NULL.
 * \param [in]   mpicomm    The MPI communicator of the scda fopen function.
 * \param [out]  info       On output the MPI info object as defined by \b opt.
 * \param [out]  errcode    An errcode that can be interpreted by \ref
 *                          sc_scda_ferror_string or mapped to an error class.
 *                          by \ref sc_scda_ferror_class.
 * \return                  A pointer to a file context containing the fuzzy
 *                          error parameters as encoded in \b opt and the
 *                          MPI rank and size according to \b mpicomm.
 *                          In case of an error NULL, see also \b errcode.
 */
static sc_scda_fcontext_t*
sc_scda_fopen_start_up (sc_scda_fopen_options_t *opt, sc_MPI_Comm mpicomm,
                        sc_MPI_Info *info, sc_scda_ferror_t *errcode)
{
  sc_scda_fcontext_t *fc;
  sc_scda_ret_t       scdaret;

  SC_ASSERT (info != NULL);

  /* allocate the file context */
  fc = SC_ALLOC (sc_scda_fcontext_t, 1);

  /* fill convenience MPI information */
  sc_scda_fill_mpi_data (fc, mpicomm);

  /* examine options */
  scdaret = sc_scda_examine_options (opt, fc, info);
  /* It is guaranteed by sc_scda_examine_options that fuzzy error testing is
   * deactivated in fc if scdaret does not encode success.
   */
  sc_scda_scdaret_to_errcode (scdaret, errcode, fc);

  if (!sc_scda_ferror_is_success (*errcode)) {
    /* an error occurred and the file was not yet open */
    SC_FREE (fc);
    return NULL;
  }

  return fc;
}

/** Internal function to run the serial code part in \ref sc_scda_fopen_write.
 *
 * \param [in] fc           The file context as in \ref sc_scda_fopen_write
 *                          before running the serial code part.
 * \param [in] user_string  As in the documentation of \ref sc_scda_fopen_write.
 * \param [in] len          As in the documentation of \ref sc_scda_fopen_write.
 * \param [out] count_err   A Boolean indicating if a count error occurred.
 * \param [out] errcode     An errcode that can be interpreted by \ref
 *                          sc_scda_ferror_string or mapped to an error class
 *                          by \ref sc_scda_ferror_class.
 */
static void
sc_scda_fopen_write_serial_internal (sc_scda_fcontext_t *fc,
                                     const char *user_string, size_t *len,
                                     int *count_err, sc_scda_ferror_t *errcode)
{
  int                 mpiret;
  int                 count;
  int                 current_len;
  int                 invalid_user_string;
  char                file_header_data[SC_SCDA_HEADER_BYTES];
  size_t              user_string_len;

  /* get scda file header section */
  /* magic */
  sc_scda_copy_bytes (file_header_data, SC_SCDA_MAGIC, SC_SCDA_MAGIC_BYTES);
  current_len = SC_SCDA_MAGIC_BYTES;

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
  /* check the user string */
  /* According to 'A.2 Parameter conventions' in the scda specification
   * it is an unchecked runtime error if the user string is not collective,
   * and it leads to undefined behavior.
   * Therefore, we just check the user string on rank 0.
   */
  invalid_user_string =
    sc_scda_get_user_string_len (user_string, len, &user_string_len);
  /* We always translate the error code to have full coverage for the fuzzy
   * error testing.
   */
  sc_scda_scdaret_to_errcode (invalid_user_string ? SC_SCDA_FERR_ARG :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  /* We call the macro to check for a non-collectiv error.
   * The macro can only handle non-collective errors that occur on rank 0.
   * If errcode encodes success, the macro has no effect. Otherwise,
   * the macro prints the error using \ref SC_LERRORF and returns from this
   * function, i.e. jump to \ref SC_SCDA_HANDLE_NONCOLL_ERR. Hence, this macro
   * is only valid in pair with a call of \ref SC_SCDA_HANDLE_NONCOLL_ERR
   * directly after the non-collective code part.
   */
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid user string");

  sc_scda_pad_to_fix_len (user_string, user_string_len,
                          &file_header_data[current_len],
                          SC_SCDA_USER_STRING_FIELD);
  current_len += SC_SCDA_USER_STRING_FIELD;

  /* pad the file header section */
  sc_scda_pad_to_mod (NULL, 0, &file_header_data[current_len]);
  current_len += SC_SCDA_PADDING_MOD;

  SC_ASSERT (current_len == SC_SCDA_HEADER_BYTES);

  /* write scda file header section */
  mpiret =
    sc_io_write_at (fc->file, 0, file_header_data, SC_SCDA_HEADER_BYTES,
                    sc_MPI_BYTE, &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  /* See the first appearance of this macro in this function for more
   * information. They both use the same associated \ref
   * SC_SCDA_HANDLE_NONCOLL_ERR call.
   */
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Writing the file header section");
  /* The macro to check non-collective for count errors.
   * If SC_SCDA_HEADER_BYTES == count, i.e. no count error occurred, the macro
   * has no effect. This macro is only valid after calling \ref
   * SC_SCDA_CHECK_NONCOLL_ERR. If a count error occurred, the macro returns
   * from this function, i.e. jump to \ref SC_SCDA_HANDLE_NONCOLL_ERR, which
   * must be followed by a call of SC_SCDA_HANDLE_NONCOLL_COUNT_ERR.
   * The macro argument count_err must point to the count error Boolean that is
   * an output parameter of this function.
   */
  SC_SCDA_CHECK_NONCOLL_COUNT_ERR (SC_SCDA_HEADER_BYTES, count, count_err);
}

sc_scda_fcontext_t *
sc_scda_fopen_write (sc_MPI_Comm mpicomm,
                     const char *filename,
                     const char *user_string, size_t *len,
                     sc_scda_fopen_options_t * opt,
                     sc_scda_ferror_t * errcode)
{
  int                 mpiret;
  int                 count_err;
  sc_MPI_Info         info;
  sc_scda_fcontext_t *fc;

  SC_ASSERT (filename != NULL);
  SC_ASSERT (user_string != NULL);
  SC_ASSERT (errcode != NULL);

  /* We assume the filename to be nul-terminated. */

  /* get MPI info and parse opt */
  fc = sc_scda_fopen_start_up (opt, mpicomm, &info, errcode);
  if (fc == NULL) {
    /* start up failed; see errcode */
    /* This is a special case of an error in a scda top-level function before
     * the file is opened. Hence, we do not use the standard error macros but
     * just print the error by the following macro.
     */
    SC_SCDA_CHECK_VERBOSE_COLL (*errcode, "Parse options");
    return NULL;
  }

  /* open the file for writing */
  mpiret =
    sc_io_open (mpicomm, filename, SC_IO_WRITE_CREATE, info, &fc->file);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  /* We call a macro for checking an error that occurs collectivly.
   * In case of an error the macro prints an error message using \ref
   * SC_GLOBAL_LERRORF, closes the file and frees fc.
   */
  SC_SCDA_CHECK_COLL_ERR (errcode, fc, "File open write");

  if (fc->mpirank == 0) {
    sc_scda_fopen_write_serial_internal (fc, user_string, len, &count_err,
                                         errcode);
  }
  /* This macro must be the first expression after the non-collective code part
   * since it must be the point where \ref SC_SCDA_CHECK_NONCOLL_ERR and \ref
   * SC_SCDA_CHECK_NONCOLL_COUNT_ERR can jump to, which are called in \ref
   * sc_scda_fopen_write_serial_internal. The macro handles the non-collective
   * error, i.e. it broadcasts the errcode, which may encode success, from
   * rank 0 to all other ranks and in case of an error it closes the file,
   * frees the file context and returns NULL. Hence, it is valid that errcode
   * is only initialized on rank 0 before calling this macro. This macro is only
   * valid to be called once in a function and this macro is only valid to be
   * called directly after a non-collective code part that contains at least one
   * call \ref SC_SCDA_CHECK_NONCOLL_ERR.
   */
  SC_SCDA_HANDLE_NONCOLL_ERR (errcode, fc);
  /* The macro to check potential non-collective count errors. It is only valid
   * to be called directly after \ref SC_SCDA_HANDLE_NONCOLL_ERR and only
   * if the preceding non-collective code block contains at least one call of
   * \ref SC_SCDA_CHECK_NONCOLL_COUNT_ERR. The macro is only valid to be called
   * once per function. The count error status is broadcasted and the macro
   * prints an error message using \ref SC_LERRORF. This means in particular
   * that it is valid that errcode is only initialized on rank 0 before calling
   * this macro. The macro argument count_err must point to the count error
   * Boolean that was set on rank 0 by \ref sc_scda_fopen_write_serial_internal.
   */
  SC_SCDA_HANDLE_NONCOLL_COUNT_ERR (errcode, &count_err, fc);

  return fc;
}

/**
 */
static int
sc_scda_check_file_header (char *file_header_data, char *user_string,
                           size_t *len)
{
  int                 current_pos;
  char                vendor_string[SC_SCDA_VENDOR_STRING_BYTES];
  size_t              vendor_len;

  SC_ASSERT (file_header_data != NULL);
  SC_ASSERT (user_string != NULL);
  SC_ASSERT (len != NULL);

  /* TODO: Add errcode as output parameter if we want to add more detailed errors */

  /* check structure that is not padding */
  /* *INDENT-OFF* */
  if (!(file_header_data[SC_SCDA_MAGIC_BYTES] == ' ' &&
        file_header_data[SC_SCDA_MAGIC_BYTES + 1 +
                         SC_SCDA_VENDOR_STRING_FIELD] == 'F' &&
        file_header_data[SC_SCDA_MAGIC_BYTES + 1 +
                         SC_SCDA_VENDOR_STRING_FIELD + 1] == ' ')) {
    /* wrong file header structure */
    return -1;
  }
  /* *INDENT-ON* */

  /* check the entries of the file header */

  /* check magic */
  if (memcmp (SC_SCDA_MAGIC, file_header_data, SC_SCDA_MAGIC_BYTES)) {
    /* wrong magic */
    return -1;
  }
  current_pos = SC_SCDA_MAGIC_BYTES + 1;

  /* check the padding of the vendor string */
  if (sc_scda_get_pad_to_fix_len (&file_header_data[current_pos],
                                  SC_SCDA_VENDOR_STRING_FIELD, vendor_string,
                                  &vendor_len)) {
    /* wrong padding format */
    return -1;
  }
  /* vendor string content is not checked */

  current_pos += SC_SCDA_VENDOR_STRING_FIELD + 2;
  /* check the user string */
  if (sc_scda_get_pad_to_fix_len
      (&file_header_data
       [current_pos], SC_SCDA_USER_STRING_FIELD, user_string, len)) {
    /* wrong padding format */
    return -1;
  }
  /* the user string content is not checked */
  user_string[*len] = '\0';

  current_pos += SC_SCDA_USER_STRING_FIELD;
  /* check the padding of zero data bytes */
  if (sc_scda_get_pad_to_mod
      (&file_header_data[current_pos], SC_SCDA_PADDING_MOD, 0, NULL)) {
    /* wrong padding format */
    return -1;
  }

  return 0;
}

/** Internal function to run the serial code part in \ref sc_scda_fopen_read.
 *
 * \param [in] fc           The file context as in \ref sc_scda_fopen_read
 *                          before running the serial code part.
 * \param [out] user_string As in the documentation of \ref sc_scda_fopen_read.
 * \param [out] len         As in the documentation of \ref sc_scda_fopen_read.
 * \param [out] count_err   A Boolean indicating if a count error occurred.
 * \param [out] errcode     An errcode that can be interpreted by \ref
 *                          sc_scda_ferror_string or mapped to an error class
 *                          by \ref sc_scda_ferror_class.
 */
static void
sc_scda_fopen_read_serial_internal (sc_scda_fcontext_t * fc,
                                    char *user_string, size_t *len,
                                    int *count_err, sc_scda_ferror_t *errcode)
{
  int                 mpiret;
  int                 count;
  int                 invalid_file_header;
  char                file_header_data[SC_SCDA_HEADER_BYTES];

  mpiret =
    sc_io_read_at (fc->file, 0, file_header_data, SC_SCDA_HEADER_BYTES,
                   sc_MPI_BYTE, &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  /* The macro to check errcode after a non-collective function call.
   * More information can be found in the comments in \ref sc_scda_fopen_write
   * and in the documentation of the \ref SC_SCDA_CHECK_NONCOLL_ERR.
   */
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Read the file header section");
  /* The macro to check for a count error after a non-collective function call.
   * More information can be found in the comments in \ref sc_scda_fopen_write
   * and in the documentation of the \ref SC_SCDA_CHECK_NONCOLL_COUNT_ERR.
   */
  SC_SCDA_CHECK_NONCOLL_COUNT_ERR (SC_SCDA_HEADER_BYTES, count, count_err);

  /* initialize user_string */
  sc_scda_init_nul (user_string, SC_SCDA_USER_STRING_BYTES + 1);

  invalid_file_header =
    sc_scda_check_file_header (file_header_data, user_string, len);
  sc_scda_scdaret_to_errcode (invalid_file_header ? SC_SCDA_FERR_FORMAT :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  /* cf. the comment on the first call of this macro in this function */
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid file header");
}

sc_scda_fcontext_t *
sc_scda_fopen_read (sc_MPI_Comm mpicomm,
                    const char *filename,
                    char *user_string, size_t *len,
                    sc_scda_fopen_options_t * opt, sc_scda_ferror_t * errcode)
{
  int                 mpiret;
  int                 count_err;
  sc_MPI_Info         info;
  sc_scda_fcontext_t *fc;

  SC_ASSERT (filename != NULL);
  SC_ASSERT (user_string != NULL);
  SC_ASSERT (len != NULL);
  SC_ASSERT (errcode != NULL);

  /* We assume the filename to be nul-terminated. */

  /* get MPI info and parse opt */
  fc = sc_scda_fopen_start_up (opt, mpicomm, &info, errcode);
  if (fc == NULL) {
    /* start up failed; see errcode */
    /* This is a special case of an error in a scda top-level function before
     * the file is opened. Hence, we do not use the standard error macros but
     * just print the error by the following macro.
     */
    SC_SCDA_CHECK_VERBOSE_COLL (*errcode, "Parse options");
    return NULL;
  }

  /* open the file in reading mode */
  mpiret = sc_io_open (mpicomm, filename, SC_IO_READ, info, &fc->file);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  /* The macro to check errcode after a collective function call.
   * More information can be found in the comments in \ref sc_scda_fopen_write
   * and in the documentation of the \ref SC_SCDA_CHECK_COLL_ERR.
   */
  SC_SCDA_CHECK_COLL_ERR (errcode, fc, "File open read");

  /* read file header section on rank 0 */
  if (fc->mpirank == 0) {
    sc_scda_fopen_read_serial_internal (fc, user_string, len, &count_err,
                                        errcode);
  }
  /* The macro to handle a non-collective error that is associated to a
   * preceding call of \ref SC_SCDA_CHECK_NONCOLL_ERR.
   * More information can be found in the comments in \ref sc_scda_fopen_write
   * and in the documentation of the \ref SC_SCDA_HANDLE_NONCOLL_ERR.
   */
  SC_SCDA_HANDLE_NONCOLL_ERR (errcode, fc);
  /* The macro to handle a non-collective count error that is associated to a
   * preceding call of \ref SC_SCDA_CHECK_NONCOLL_COUNT_ERR.
   * More information can be found in the comments in \ref sc_scda_fopen_write
   * and in the documentation of the \ref SC_SCDA_HANDLE_NONCOLL_COUNT_ERR.
   */
  SC_SCDA_HANDLE_NONCOLL_COUNT_ERR (errcode, &count_err, fc);
  /* Bcast the user string */
  mpiret = sc_MPI_Bcast (user_string, SC_SCDA_USER_STRING_BYTES + 1,
                         sc_MPI_BYTE, 0, mpicomm);
  SC_CHECK_MPI (mpiret);

  return fc;
}

int
sc_scda_fclose (sc_scda_fcontext_t * fc, sc_scda_ferror_t * errcode)
{
  int                 mpiret;

  SC_ASSERT (fc != NULL);
  SC_ASSERT (errcode != NULL);

  mpiret = sc_io_close (&fc->file);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  /* Since this function does not return NULL in case of an error, closes the
   * file and frees file context in any case, we can not use one of our
   * standard error macros but we call \ref SC_SCDA_CHECK_VERBOSE_COLL to print
   * an error message in case of an error.
   */
  SC_SCDA_CHECK_VERBOSE_COLL (*errcode, "File close");

  SC_FREE (fc);

  return sc_scda_ferror_is_success (*errcode) ? 0 : -1;
}

int
sc_scda_ferror_string (sc_scda_ferror_t errcode, char *str, int *len)
{
  int                 retval;
  const char         *tstr = NULL;

  SC_ASSERT (str != NULL);
  SC_ASSERT (len != NULL);
  SC_ASSERT (sc_scda_errcode_is_valid (errcode));

  if (str == NULL || len == NULL || !sc_scda_errcode_is_valid (errcode)) {
    return SC_SCDA_FERR_ARG;
  }

  /* check if an MPI error occurred */
  if (errcode.scdaret == SC_SCDA_FERR_MPI) {
    /* use mpiret for the error string */
    return sc_MPI_Error_string (errcode.mpiret, str, len);
  }

  /* no MPI error occurred; use scdaret for the error string */
  switch (errcode.scdaret) {
  case SC_SCDA_FERR_SUCCESS:
    tstr = "Success";
    break;
  case SC_SCDA_FERR_FORMAT:
    tstr = "Wrong file format";
    break;
  case SC_SCDA_FERR_USAGE:
    tstr = "Incorrect workflow for scda reading function";
    break;
  case SC_SCDA_FERR_DECODE:
    tstr = "Not conforming to scda encoding convention";
    break;
  case SC_SCDA_FERR_ARG:
    tstr = "Invalid argument to scda file function";
    break;
  case SC_SCDA_FERR_COUNT:
    tstr =
      "Read or write count error that is not classified as an other error";
    break;

  default:
    /* not a valid scdaret value or SC_SCDA_FERR_MPI */
    SC_ABORT_NOT_REACHED ();
  }
  SC_ASSERT (tstr != NULL);

  /* print into the output string */
  if ((retval = snprintf (str, sc_MPI_MAX_ERROR_STRING, "%s", tstr)) < 0) {
    /* unless something goes against the current standard of snprintf */
    return sc_MPI_ERR_NO_MEM;
  }
  if (retval >= sc_MPI_MAX_ERROR_STRING) {
    retval = sc_MPI_MAX_ERROR_STRING - 1;
  }
  *len = retval;

  /* we have successfully placed a string in the output variables */
  return SC_SCDA_FERR_SUCCESS;
}
