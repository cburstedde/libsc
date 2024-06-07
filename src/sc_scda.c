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
#include <time.h>

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
#define SC_SCDA_FUZZY_FREQUENCY 3 /**< default frequency for fuzzy error return */

/** get a random int in the range [A,B] */
#define SC_SCDA_RAND_RANGE(A,B) ((A) + rand() / (RAND_MAX / ((B) - (A) + 1) + 1))

/** Examine scda file return value and print an error message in case of error.
 * The parameter msg is prepended to the file, line and error information.
 */
#define SC_SCDA_CHECK_VERBOSE(errcode, msg) do {                              \
                                    char sc_scda_msg[sc_MPI_MAX_ERROR_STRING];\
                                    int sc_scda_len, sc_scda_retval;          \
                                    if (!sc_scda_is_success (errcode)) {      \
                                    sc_scda_retval =                          \
                                    sc_scda_ferror_string (errcode, sc_scda_msg,\
                                                           &sc_scda_len);     \
                                    SC_ASSERT(sc_scda_retval !=               \
                                              SC_SCDA_FERR_ARG);\
                                    if (sc_scda_retval == SC_SCDA_FERR_SUCCESS){\
                                    SC_GLOBAL_LERRORF ("%s at %s:%d: %*.*s\n",\
                                                       msg,  __FILE__, __LINE__,\
                                                       sc_scda_len, sc_scda_len,\
                                                       sc_scda_msg);}         \
                                    else {                                    \
                                    SC_GLOBAL_LERRORF ("%s at %s:%d: %s\n",\
                                                       msg, __FILE__, __LINE__,\
                                                       "An error occurred but "\
                                                       "ferror_string failed");\
                                    }}} while (0)

/** Collectivly check a given errorcode.
 * This macro assumes that errcode is a collective
 * variable and that the macro is called collectivly.
 * The calling function must return NULL in case of an error.
 */
#define SC_SCDA_CHECK_COLL_ERR(errcode, fc, user_msg) do {                   \
                                    SC_SCDA_CHECK_VERBOSE (*errcode, user_msg);\
                                    if (!sc_scda_is_success (*errcode)) {    \
                                    sc_scda_file_error_cleanup (&fc->file);  \
                                    SC_FREE (fc);                            \
                                    return NULL;}} while (0)

/* This macro is suitable to be called after a non-collective operation.
 * It is only valid to be called once per function.
 * For a correct error handling it is required to skip the rest
 * of the non-collective code and then broadcast the error flag.
 * The macro can be used multiple times in a function but will always jump to
 * the same label. This leads to the intended error handling.
 */
#define SC_SCDA_CHECK_NONCOLL_ERR(errcode, user_msg) do {                    \
                                    SC_SCDA_CHECK_VERBOSE (*errcode, user_msg);\
                                    if (!sc_scda_is_success (*errcode)) {    \
                                    goto scda_err_lbl;}} while (0)

/** Handle a non-collective error.
 * Use this macro after \ref SC_SCDA_CHECK_NONCOLL_ERR *directly* after the end
 * of non-collective statements.
 * Can be only used once in a function.
 */
#define SC_SCDA_HANDLE_NONCOLL_ERR(errcode, fc) do{                            \
                                    scda_err_lbl: ;                            \
                                    SC_CHECK_MPI(sc_MPI_Bcast(&errcode->scdaret,\
                                                  1, sc_MPI_INT, 0,           \
                                                  fc->mpicomm));               \
                                    SC_CHECK_MPI(sc_MPI_Bcast(&errcode->mpiret,\
                                                  1, sc_MPI_INT, 0,            \
                                                  fc->mpicomm));               \
                                    if (!sc_scda_is_success (*errcode)) {      \
                                    sc_scda_file_error_cleanup (&fc->file);    \
                                    SC_FREE (fc);                              \
                                    return NULL;}} while (0)

/** Declare variable for the count error synchronization.
 * Since we must synchronize the count error, we declare a variable for the
 * count error that is true if a count occurred.
 * We use this macro to have a fixed name of the variable that then can be used
 * in the macros \ref SC_SCDA_CHECK_NONCOLL_COUNT_ERR and \ref
 * SC_SCDA_HANDLE_NONCOLL_COUNT_ERR without explicitly passing the error count
 * variable.
 */
#define SC_SCDA_DECLARE_COUNT_VAR int scda_count_err

/** Check for a count error of a serial (rank 0) I/O operation.
 * This macro is only valid to use after checking that there was not I/O error.
 * It is only valid to be called once per function.
 * For a correct error handling it is required to skip the rest
 * of the non-collective code and then broadcast the count  error flag.
 * The macro can be used multiple times in a function but will always jump to
 * the same label. This leads to the intended error handling.
 */
#define SC_SCDA_CHECK_NONCOLL_COUNT_ERR(icount, ocount) do {                   \
                                    scda_count_err = ((int) icount) != ocount; \
                                    if (scda_count_err) {                      \
                                    SC_LERRORF ("Count error on rank 0 at "    \
                                                "%s:%d.\n", __FILE__, __LINE__);\
                                    goto scda_err_lbl;}} while (0)

/** Handle a non-collective count error.
 * Use this macro after \ref SC_SCDA_CHECK_NONCOLL_COUNT_ERR *directly* after
 * the end of the non-collective statements.
 * Can be used only once in a function.
 */
#define SC_SCDA_HANDLE_NONCOLL_COUNT_ERR(errorcode, fc) do{                    \
                                    SC_CHECK_MPI (sc_MPI_Bcast (&scda_count_err,\
                                                  1, sc_MPI_INT, 0,            \
                                                  fc->mpicomm));               \
                                    sc_scda_scdaret_to_errcode (               \
                                        scda_count_err ? SC_SCDA_FERR_COUNT :  \
                                                         SC_SCDA_FERR_SUCCESS, \
                                        errorcode, fc);                        \
                                    SC_SCDA_CHECK_VERBOSE (*errorcode,         \
                                                    "Read/write count check"); \
                                    if (scda_count_err) {                      \
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
  /* Fuzzy error return variables. They are only relevant if fuzzy_errors is true */
  int                 fuzzy_errors; /**< boolean for fuzzy error return */
  unsigned            fuzzy_seed; /**< seed for fuzzy error return */
  int                 fuzzy_freq; /**< Frequency of the fuzzy error return,
                                       i.e. for each possible error origin
                                       we return a random error with the
                                       empirical probability of 1 / fuzzy_freq
                                       but only if the respective possible error
                                       origin did not already caused an error
                                       without the fuzzy error return. In such a
                                       case, the actual error is returned. */
  /* *INDENT-ON* */
};

static void
sc_scda_copy_bytes (char *dest, const char *src, size_t n)
{
  SC_ASSERT (dest != NULL);
  SC_ASSERT (n == 0 || src != NULL);

  void               *pointer;

  if (n == 0) {
    return;
  }

  pointer = memcpy (dest, src, n);
  SC_EXECUTE_ASSERT_TRUE (pointer == (void *) dest);
}

static void
sc_scda_set_bytes (char *dest, int c, size_t n)
{
  SC_ASSERT (dest != NULL);

  memset (dest, c, n);
}

static void
sc_scda_init_nul (char *dest, size_t len)
{
  SC_ASSERT (dest != NULL);

  sc_scda_set_bytes (dest, '\0', len);
}

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
  SC_ASSERT (output_data != NULL);
  SC_ASSERT (input_len <= pad_len - 4);

#if 0
  uint8_t            *byte_arr;
#endif

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
 *
 */
static int
sc_scda_get_pad_to_fix_len (char *padded_data, size_t pad_len, char *raw_data,
                            size_t *raw_len)
{
  SC_ASSERT (padded_data != NULL);
  SC_ASSERT (raw_data != NULL);
  SC_ASSERT (raw_len != NULL);

  size_t              si;

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

static void
sc_scda_pad_to_mod (const char *input_data, size_t input_len,
                    char *output_data)
{
  SC_ASSERT (input_len == 0 || input_data != NULL);
  SC_ASSERT (output_data != NULL);

  size_t              num_pad_bytes;

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
 */
static int
sc_scda_get_pad_to_mod (char *padded_data, size_t padded_len, size_t raw_len,
                        char *raw_data)
{
  SC_ASSERT (padded_data != NULL);
  SC_ASSERT (raw_len == 0 || raw_data != NULL);

  size_t              si;
  size_t              num_pad_bytes;

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
 * TODO: We may want to add an error parameter in indicate if the options are
 * invalid.
 */
static              sc_MPI_Info
sc_scda_examine_options (sc_scda_fopen_options_t * opt, int *fuzzy,
                         unsigned *fuzzy_seed, int *fuzzy_freq,
                         sc_MPI_Comm mpicomm)
{
  /* TODO: Check options if opt is valid? */

  sc_MPI_Info         info;
  int                 mpiret;
  int                 seed;

  if (opt != NULL) {
    info = opt->info;
    *fuzzy = opt->fuzzy_errors;
    *fuzzy_seed =
      (opt->fuzzy_seed < 0) ? ((unsigned) time (NULL)) : opt->fuzzy_seed;
    if (opt->fuzzy_seed < 0) {
      /* Rank dependent returns for functions that return a snychronized 
       * error value do not make sense. However, time (NULL) may differ between
       * the ranks and hence we broadcast the time (NULL) result from rank 0.
       */
      seed = *fuzzy_seed;
      mpiret = sc_MPI_Bcast (&seed, 1, sc_MPI_INT, 0, mpicomm);
      SC_CHECK_MPI (mpiret);
    }
    *fuzzy_freq =
      (opt->fuzzy_freq < 0) ? SC_SCDA_FUZZY_FREQUENCY : opt->fuzzy_freq;
  }
  else {
    info = sc_MPI_INFO_NULL;
    /* no fuzzy error return by default */
    *fuzzy = 0;
  }

  return info;
}

static void
sc_scda_fill_mpi_data (sc_scda_fcontext_t * fc, sc_MPI_Comm mpicomm)
{
  SC_ASSERT (fc != NULL);

  int                 mpiret;

  mpiret = sc_MPI_Comm_size (mpicomm, &fc->mpisize);
  SC_CHECK_MPI (mpiret);

  mpiret = sc_MPI_Comm_rank (mpicomm, &fc->mpirank);
  SC_CHECK_MPI (mpiret);

  fc->mpicomm = mpicomm;
}

/** Get the user string length for writing.
 *
 * This functions return -1 if the input user string is not
 * compliant with the scda file format.
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

/** Create a random but consistent scdaret.
 */
static int
sc_scda_get_fuzzy_scdaret (unsigned freq)
{
  sc_scda_ret_t       sample;

  /* draw an error with the empirical probalilty of 1 / freq */
  if (rand () < (RAND_MAX + 1U) / freq) {
    /* draw an error  */
    sample =
      SC_SCDA_RAND_RANGE (SC_SCDA_FERR_FORMAT, SC_SCDA_FERR_LASTCODE - 1);
  }
  else {
    sample = SC_SCDA_FERR_SUCCESS;
  }

  return sample;
}

static int
sc_scda_get_fuzzy_mpiret (unsigned freq)
{
  int                 index_sample, sample;

  /* draw an error with the empirical probalilty of 1 / freq */
  if (rand () < (RAND_MAX + 1U) / freq) {
    /* draw an MPI 2.0 I/O error */
    /* The MPI standard does not guarantee that the MPI error codes
     * are contiguous. Hence, take the minimal available error code set and
     * draw an index into this set.
     */
    index_sample = SC_SCDA_RAND_RANGE (0, 15);

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
  SC_ASSERT (SC_SCDA_FERR_SUCCESS <= scda_ret &&
             scda_ret < SC_SCDA_FERR_LASTCODE);
  SC_ASSERT (scda_errorcode != NULL);
  SC_ASSERT (!fc->fuzzy_errors || fc->fuzzy_freq > 0);

  /* if we have an MPI error; we need \ref sc_scda_mpiret_to_errcode */
  SC_ASSERT (scda_ret != SC_SCDA_FERR_MPI);

  sc_scda_ret_t       scda_ret_internal;
  int                 mpiret_internal;

  if (scda_ret != SC_SCDA_FERR_SUCCESS) {
    /* an error happened and we do not return fuzzy in any case */
    scda_ret_internal = scda_ret;
    mpiret_internal = sc_MPI_SUCCESS;
  }
  else {
    /* no error occurred, we may return fuzzy */
    scda_ret_internal =
      (!fc->fuzzy_errors) ? scda_ret :
      sc_scda_get_fuzzy_scdaret (fc->fuzzy_freq);
    if (scda_ret_internal == SC_SCDA_FERR_MPI) {
      SC_ASSERT (fc->fuzzy_errors);
      /* we must draw an MPI error */
      /* frequency 1 since we need mpiret != sc_MPI_SUCCESS */
      mpiret_internal = sc_scda_get_fuzzy_mpiret (1);
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
  SC_ASSERT ((sc_MPI_SUCCESS <= mpiret && mpiret < sc_MPI_ERR_LASTCODE));
  SC_ASSERT (scda_errorcode != NULL);
  SC_ASSERT (!fc->fuzzy_errors || fc->fuzzy_freq > 0);

  sc_scda_ret_t       scda_ret_internal;
  int                 mpiret_internal;

  if (!fc->fuzzy_errors) {
    scda_ret_internal =
      (mpiret == sc_MPI_SUCCESS) ? SC_SCDA_FERR_SUCCESS : SC_SCDA_FERR_MPI;
    mpiret_internal = mpiret;
  }
  else {
    /* fuzzy error testing */
    mpiret_internal = sc_scda_get_fuzzy_mpiret (fc->fuzzy_freq);
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
sc_scda_is_success (sc_scda_ferror_t errorcode)
{
  SC_ASSERT (sc_scda_errcode_is_valid (errorcode));

  return !errorcode.scdaret && !errorcode.mpiret;
}

static void
sc_scda_init_fuzzy_seed (unsigned seed)
{
  srand (seed);
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

sc_scda_fcontext_t *
sc_scda_fopen_write (sc_MPI_Comm mpicomm,
                     const char *filename,
                     const char *user_string, size_t *len,
                     sc_scda_fopen_options_t * opt,
                     sc_scda_ferror_t * errcode)
{
  SC_ASSERT (filename != NULL);
  SC_ASSERT (user_string != NULL);
  SC_ASSERT (errcode != NULL);

  int                 mpiret;
  sc_MPI_Info         info;
  sc_scda_fcontext_t *fc;

  SC_SCDA_DECLARE_COUNT_VAR;

  /* We assume the filename to be nul-terminated. */

  /* allocate the file context */
  fc = SC_ALLOC (sc_scda_fcontext_t, 1);

  /* examine options */
  info = sc_scda_examine_options (opt, &fc->fuzzy_errors, &fc->fuzzy_seed,
                                  &fc->fuzzy_freq, mpicomm);
  /* TODO: check if the options are valid */

  /* fill convenience MPI information */
  sc_scda_fill_mpi_data (fc, mpicomm);

  if (fc->fuzzy_errors) {
    sc_scda_init_fuzzy_seed (fc->fuzzy_seed);
  }

  /* open the file for writing */
  mpiret =
    sc_io_open (mpicomm, filename, SC_IO_WRITE_CREATE, info, &fc->file);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_COLL_ERR (errcode, fc, "File open write");

  if (fc->mpirank == 0) {
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
    SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Writing the file header section");
    SC_SCDA_CHECK_NONCOLL_COUNT_ERR (SC_SCDA_HEADER_BYTES, count);
  }
  SC_SCDA_HANDLE_NONCOLL_ERR (errcode, fc);
  SC_SCDA_HANDLE_NONCOLL_COUNT_ERR (errcode, fc);

  return fc;
}

static int
sc_scda_check_file_header (char *file_header_data, char *user_string,
                           size_t *len)
{
  SC_ASSERT (file_header_data != NULL);
  SC_ASSERT (user_string != NULL);
  SC_ASSERT (len != NULL);

  int                 current_pos;
  char                vendor_string[SC_SCDA_VENDOR_STRING_BYTES];
  size_t              vendor_len;

  /* TODO: Add errcode as output parameter */

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

sc_scda_fcontext_t *
sc_scda_fopen_read (sc_MPI_Comm mpicomm,
                    const char *filename,
                    char *user_string, size_t *len,
                    sc_scda_fopen_options_t * opt, sc_scda_ferror_t * errcode)
{
  SC_ASSERT (filename != NULL);
  SC_ASSERT (user_string != NULL);
  SC_ASSERT (len != NULL);
  SC_ASSERT (errcode != NULL);

  int                 mpiret;
  sc_MPI_Info         info;
  sc_scda_fcontext_t *fc;

  SC_SCDA_DECLARE_COUNT_VAR;

  /* We assume the filename to be nul-terminated. */

  /* allocate the file context */
  fc = SC_ALLOC (sc_scda_fcontext_t, 1);

  /* examine options */
  info = sc_scda_examine_options (opt, &fc->fuzzy_errors, &fc->fuzzy_seed,
                                  &fc->fuzzy_freq, mpicomm);
  /* TODO: check if options are valid */

  /* fill convenience MPI information */
  sc_scda_fill_mpi_data (fc, mpicomm);

  if (fc->fuzzy_errors) {
    sc_scda_init_fuzzy_seed (fc->fuzzy_seed);
  }

  /* open the file in reading mode */
  mpiret = sc_io_open (mpicomm, filename, SC_IO_READ, info, &fc->file);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_COLL_ERR (errcode, fc, "File open read");

  /* read file header section on rank 0 */
  if (fc->mpirank == 0) {
    int                 count;
    int                 invalid_file_header;
    char                file_header_data[SC_SCDA_HEADER_BYTES];

    mpiret =
      sc_io_read_at (fc->file, 0, file_header_data, SC_SCDA_HEADER_BYTES,
                     sc_MPI_BYTE, &count);
    sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
    SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Read the file header section");
    SC_SCDA_CHECK_NONCOLL_COUNT_ERR (SC_SCDA_HEADER_BYTES, count);

    /* initialize user_string */
    sc_scda_init_nul (user_string, SC_SCDA_USER_STRING_BYTES + 1);

    invalid_file_header =
      sc_scda_check_file_header (file_header_data, user_string, len);
    sc_scda_scdaret_to_errcode (invalid_file_header ? SC_SCDA_FERR_FORMAT :
                                SC_SCDA_FERR_SUCCESS, errcode, fc);
    SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid file header");
  }
  SC_SCDA_HANDLE_NONCOLL_ERR (errcode, fc);
  SC_SCDA_HANDLE_NONCOLL_COUNT_ERR (errcode, fc);
  /* Bcast the user string */
  mpiret = sc_MPI_Bcast (user_string, SC_SCDA_USER_STRING_BYTES + 1,
                         sc_MPI_BYTE, 0, mpicomm);
  SC_CHECK_MPI (mpiret);

  return fc;
}

int
sc_scda_fclose (sc_scda_fcontext_t * fc, sc_scda_ferror_t * errcode)
{
  SC_ASSERT (fc != NULL);
  SC_ASSERT (errcode != NULL);

  int                 mpiret;

  mpiret = sc_io_close (&fc->file);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_VERBOSE (*errcode, "File close");

  SC_FREE (fc);

  return sc_scda_is_success (*errcode) ? 0 : -1;
}

int
sc_scda_ferror_string (sc_scda_ferror_t errcode, char *str, int *len)
{
  SC_ASSERT (str != NULL);
  SC_ASSERT (len != NULL);
  SC_ASSERT (sc_scda_errcode_is_valid (errcode));

  int                 retval;
  const char         *tstr = NULL;

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
