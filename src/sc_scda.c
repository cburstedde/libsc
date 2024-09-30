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
#define SC_SCDA_COUNT_ENTRY 30 /**< byte count of the count variable entry in
                                    the section header including padding */
#define SC_SCDA_COMMON_FIELD (2 + SC_SCDA_USER_STRING_FIELD) /**< byte count for
                                                                  common part of
                                                                  the file
                                                                  section
                                                                  headers */
#define SC_SCDA_COUNT_FIELD (2 + SC_SCDA_COUNT_ENTRY) /**< byte count of the
                                                           complete count
                                                           variable entry in
                                                           the section header */
#define SC_SCDA_COUNT_MAX_DIGITS 26 /**< maximal decimal digits count of a count
                                         variable in a section header */
#define SC_SCDA_PADDING_MOD 32  /**< divisor for variable length padding */
#define SC_SCDA_PADDING_MOD_MAX (6 + SC_SCDA_PADDING_MOD) /**< maximal count of
                                                              mod padding bytes */
#define SC_SCDA_HEADER_ROOT 0 /**< root rank for header I/O operations */

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
 * of non-collective statements. The parameter root is the rank on that the
 * non-collective code runs.
 * Can be only used once in a function.
 */
#define SC_SCDA_HANDLE_NONCOLL_ERR(errcode, root, fc) do{                      \
                                    SC_CHECK_MPI(sc_MPI_Bcast(&errcode->scdaret,\
                                                  1, sc_MPI_INT, root,         \
                                                  fc->mpicomm));               \
                                    SC_CHECK_MPI(sc_MPI_Bcast(&errcode->mpiret,\
                                                  1, sc_MPI_INT, root,         \
                                                  fc->mpicomm));               \
                                    sc_scda_fuzzy_sync_state (fc);             \
                                    if (!sc_scda_ferror_is_success (*errcode)) {\
                                    sc_scda_file_error_cleanup (&fc->file);    \
                                    SC_FREE (fc);                              \
                                    return NULL;}} while (0)

/** Check for a count error of a collective I/O operation.
 * This macro is only valid to use after checking that there was not I/O error.
 * The macro must be called collectively with \b fc and \b errcode being
 * collective parameters. \b icount and \b ocount may depend on the MPI rank.
 * The calling function must return NULL in case of an error.
 */
#define SC_SCDA_CHECK_COLL_COUNT_ERR(icount, ocount, fc, errcode) do {       \
                                    int sc_scda_global_cerr;                 \
                                    int sc_scda_local_cerr;                  \
                                    int sc_scda_mpiret;                      \
                                    SC_ASSERT (                              \
                                      sc_scda_ferror_is_success (*errcode)); \
                                    sc_scda_local_cerr =                     \
                                                  ((int) icount != ocount);  \
                                    sc_scda_mpiret = sc_MPI_Allreduce (      \
                                                        &sc_scda_local_cerr, \
                                                        &sc_scda_global_cerr,\
                                                        1, sc_MPI_INT,       \
                                                        sc_MPI_LOR,          \
                                                        fc->mpicomm);        \
                                    SC_CHECK_MPI (sc_scda_mpiret);           \
                                    sc_scda_scdaret_to_errcode (             \
                                      sc_scda_global_cerr ? SC_SCDA_FERR_COUNT :\
                                                        SC_SCDA_FERR_SUCCESS,\
                                      errcode, fc);                          \
                                    SC_SCDA_CHECK_VERBOSE_COLL (*errcode,    \
                                                  "Read/write count check"); \
                                    if (sc_scda_global_cerr) {               \
                                    SC_ASSERT (                              \
                                      !sc_scda_ferror_is_success (*errcode));\
                                    SC_GLOBAL_LERRORF ("Count error for "    \
                                                "collective I/O at %s:%d.\n",\
                                                __FILE__, __LINE__);         \
                                    sc_scda_file_error_cleanup (&fc->file);  \
                                    SC_FREE (fc);                            \
                                    return NULL;                             \
                                    }} while (0)                             \

/** Check for a count error of a serial I/O operation.
 * This macro is only valid to use after checking that there was not I/O error.
 * For a correct error handling it is required to skip the rest
 * of the non-collective code and then broadcast the count  error flag.
 * The macro can be used multiple times in a function but will always jump to
 * the end of the calling function that must be a void function.
 * \b cerror must be a pointer to an int that is passed to the subsequent call
 * \ref SC_SCDA_HANDLE_NONCOLL_COUNT_ERR.
 */
#define SC_SCDA_CHECK_NONCOLL_COUNT_ERR(icount, ocount, cerror) do {           \
                                    *cerror = ((int) icount) != ocount;        \
                                    if (*cerror) {                             \
                                    SC_LERRORF ("Count error at "              \
                                                "%s:%d.\n", __FILE__, __LINE__);\
                                    return;}} while (0)

/** Handle a non-collective count error.
 * Use this macro after \ref SC_SCDA_CHECK_NONCOLL_COUNT_ERR *directly* after
 * the end of the non-collective statements but after \ref
 * SC_SCDA_HANDLE_NONCOLL_ERR, which must be executed first.
 * Can be used only once in a function.
 * The parameter root is the rank on that the non-collective code runs.
 * On rank root \b cerror must point to the int that was set by \ref
 * SC_SCDA_CHECK_NONCOLL_COUNT_ERR. On all other ranks \b cerror is set by this
 * macro.
 * In case of activated fuzzy error testing it is important to notice that this
 * macro calls \ref sc_scda_scdaret_to_errcode, which may output fuzzy errors.
 * Hence, for activated fuzzy error testing one may observe reported count
 * errors with an error string for an other error.
 */
#define SC_SCDA_HANDLE_NONCOLL_COUNT_ERR(errorcode, cerror, root, fc) do{      \
                                    SC_ASSERT (                                \
                                        sc_scda_ferror_is_success (*errcode)); \
                                    SC_CHECK_MPI (sc_MPI_Bcast (cerror,        \
                                                  1, sc_MPI_INT, root,         \
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
  sc_MPI_Comm         mpicomm;        /**< associated MPI communicator */
  int                 mpisize;        /**< number of MPI ranks */
  int                 mpirank;        /**< MPI rank */
  sc_MPI_File         file;           /**< file object */
  sc_MPI_Offset       accessed_bytes; /**< number of written/read bytes */
  int                 header_before;  /**< True if the last call was \ref
                                        sc_scda_fread_section_header,
                                        otherwise, false. */
  char                last_type;      /**< If header_before is true, the file
                                        section type of the last \ref
                                        sc_scda_fread_section_header call,
                                        otherwise undefined. */
  unsigned            fuzzy_everyn;   /**< In average every n-th possible error
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
                                        that there are no fuzzy error returns.*/
  sc_rand_state_t     fuzzy_seed;     /**< The seed for the fuzzy error return.
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

/** Merge up to three buffers into one contiguous buffer.
 *
 * \param [in] d1           The first buffer. Must be not NULL.
 * \param [in] len1         The byte count of the first buffer.
 * \param [in] d2           The second buffer. May be NULL.
 * \param [in] len2         The byte count of the second buffer. Must be 0 if
 *                          \b d2 is NULL.
 * \param [in] d3           The third buffer. May be NULL and must be NULL if
 *                          \b d2 is NULL.
 * \param [in] len3         The byte count of the third buffer. Must be 0 if
 *                          \b d3 is NULL.
 * \param [out] out         At least \b len1 + \b len2 + \b len3 bytes.
 */
static void
sc_scda_merge_data_to_buf (const char *d1, size_t len1, const char *d2,
                           size_t len2, const char *d3, size_t len3,
                           char *out)
{
  SC_ASSERT (d1 != NULL);

  sc_scda_copy_bytes (out, d1, len1);
  if (d2 != NULL) {
    sc_scda_copy_bytes (&out[len1], d2, len2);
  }
  if (d3 != NULL) {
    SC_ASSERT (d2 != NULL);
    sc_scda_copy_bytes (&out[len2], d3, len3);
  }
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
 * \param [in]  input_len     The length of \b input_len in number of bytes.
 *                            \b input_len must be less or equal to
 *                            \b pad_len - 4.
 * \param [in]  pad_len       The target padding length.
 * \param [out] padding       On output the padded bytes.
 *                            \b pad_len - \b input_len many bytes.
 */
static void
sc_scda_pad_to_fix_len (size_t input_len, size_t pad_len, char *padding)
{
  SC_ASSERT (padding != NULL);
  SC_ASSERT (input_len <= pad_len - 4);

  /* We assume that padding has at least pad_len - input_len allocated bytes. */

  /* set padding */
  padding[0] = ' ';
  sc_scda_set_bytes (&padding[1], '-', pad_len - input_len - 2);
  padding[pad_len - input_len - 1] = '\n';
}

/** Pad the input data inplace to a fixed length.
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
sc_scda_pad_to_fix_len_inplace (const char *input_data, size_t input_len,
                                char *output_data, size_t pad_len)
{
  SC_ASSERT (input_data != NULL);
  SC_ASSERT (output_data != NULL);
  SC_ASSERT (input_len <= pad_len - 4);

  /* We assume that output_data has at least pad_len allocated bytes. */

  /* copy input data into output_data */
  sc_scda_copy_bytes (output_data, input_data, input_len);

  /* append padding */
  sc_scda_pad_to_fix_len (input_len, pad_len, &output_data[input_len]);
}

/** This function checks if \b padded_data is actually padded to \b pad_len.
 *
 * \param [in]  padded_data   The padded data.
 * \param [in]  pad_len       The length of \b padded_data in number of bytes.
 * \param [out] raw_len       The length of \b raw_data in number of bytes.
 *                            Undefined if the function returns true.
 * \return                    True if \b padded_data does not satisfy the
 *                            scda padding convention for fixed-length paddding.
 *                            False, otherwise.
 */
static int
sc_scda_check_pad_to_fix_len (const char *padded_data, size_t pad_len,
                              size_t *raw_len)
{
  size_t              si;

  *raw_len = 0;

  SC_ASSERT (padded_data != NULL);
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

  /* the padding is valid */
  *raw_len = si;

  return 0;
}

/** Checks the padding convention and extracts the data in case of success.
 *
 * In more concrete terms, the function checks if \b padded_data is actually
 * padded to \b pad_len and extracts the raw data in case of success.
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
sc_scda_get_pad_to_fix_len (const char *padded_data, size_t pad_len,
                            char *raw_data, size_t *raw_len)
{
  SC_ASSERT (padded_data != NULL);
  SC_ASSERT (raw_data != NULL);
  SC_ASSERT (raw_len != NULL);

  if (sc_scda_check_pad_to_fix_len (padded_data, pad_len, raw_len)) {
    /* invalid padding */
    return -1;
  }

  /* the padding is valid and the remaining data is the actual data */
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
    num_pad_bytes += SC_SCDA_PADDING_MOD *
      (size_t) ceil (((7. - num_pad_bytes) / ((double) SC_SCDA_PADDING_MOD)));
    /* The factor is necessary for the case that SC_SCDA_PADDING_MOD < 7. */
  }
  SC_ASSERT (num_pad_bytes >= 7);

  return num_pad_bytes;
}

/** Pad data to a length that is congurent to 0 modulo \ref SC_SCDA_PADDING_MOD.
 *
 * \param [in]  last_byte   Pointer to the last data byte.
 *                          This byte is required since the padding byte
 *                          content depends on the trailing data byte.
 *                          Must be NULL if \b data_len equals 0.
 * \param [in]  data_len    The length of the data in number of bytes.
 * \param [out] padding     On output the padding bytes. Must be at least
 *                          \ref sc_scda_pad_to_mod_len (\b data_len) bytes.
 */
static void
sc_scda_pad_to_mod (const char *last_byte, size_t data_len,
                    char *padding)
{
  size_t              num_pad_bytes;

  SC_ASSERT (data_len == 0 || last_byte != NULL);
  SC_ASSERT (data_len != 0 || last_byte == NULL);
  SC_ASSERT (padding != NULL);

  /* compute the number of padding bytes */
  num_pad_bytes = sc_scda_pad_to_mod_len (data_len);

  SC_ASSERT (num_pad_bytes >= 7);
  SC_ASSERT (num_pad_bytes <= SC_SCDA_PADDING_MOD_MAX);

  /* check for last byte to decide on padding format */
  if (data_len > 0 && *last_byte == '\n') {
    /* input data ends with a line break */
    padding[0] = '=';
  }
  else {
    /* add a line break add the beginning of the padding */
    padding[0] = '\n';
  }
  padding[1] = '=';

  /* append the remaining padding bytes */
  sc_scda_set_bytes (&padding[2], '=', num_pad_bytes - 4);
  padding[num_pad_bytes - 2] = '\n';
  padding[num_pad_bytes - 1] = '\n';
}

/** Pad data inplace to a len. that is cong. to 0 mod. \ref SC_SCDA_PADDING_MOD.
 *
 * \param [in]  input_data  The input data. At least \b input_len bytes.
 * \param [in]  input_len   The length of \b input_data in number of bytes.
 * \param [out] output_data On output the padded input data. Must be at least
 *                          \ref sc_scda_pad_to_mod_len (\b input_len) +
 *                          \b input_len bytes.
 */
static void
sc_scda_pad_to_mod_inplace (const char *input_data, size_t input_len,
                            char *output_data)
{
  SC_ASSERT (input_len == 0 || input_data != NULL);
  SC_ASSERT (output_data != NULL);

  const char         *last_byte;

  /* copy the input data */
  sc_scda_copy_bytes (output_data, input_data, input_len);

  /* append the padding */
  last_byte = (input_len > 0) ? &input_data[input_len - 1] : NULL;
  sc_scda_pad_to_mod (last_byte, input_len, &output_data[input_len]);
}

/** Check if the padding bytes are correct w.r.t. \ref SC_SCDA_PADDING_MOD.
 *
 * Since the mod padding depends on the trailing data byte this function also
 * requires the raw data.
 *
 * \param [in]  data          The raw data with byte count \b data_len.
 * \param [in]  data_len      The length of \b data in number of bytes.
 * \param [in]  pad           The padding bytes with byte count \b pad_len.
 * \param [in]  pad_len       The length of \b pad in number of bytes.
 *                            Must be at least 7.
 * \return                    True if \b pad does not satisfy the scda
 *                            padding convention for padding to a modulo
 *                            condition. False, otherwise.
 */
static int
sc_scda_check_pad_to_mod (const char* data, size_t data_len, const char *pad,
                          size_t pad_len)
{
  size_t              si;
  size_t              num_pad_bytes;

  SC_ASSERT (pad != NULL);

  num_pad_bytes = sc_scda_pad_to_mod_len (data_len);

  /* check if padding data length conforms to the padding format */
  if (num_pad_bytes != pad_len) {
    /* data_len and pad_len are not consistent */
    return -1;
  }
  SC_ASSERT (pad_len >= 7);

  /* check the content of the padding bytes */
  if (pad[pad_len - 1] != '\n' ||
      pad[pad_len - 2] != '\n') {
    /* terminating line breaks are missing */
    return -1;
  }

  for (si = pad_len - 3; si != 0; --si) {
    if (pad[si] != '=') {
      /* wrong padding character */
      return -1;
    }
  }

  /* padding depends on the trailing data byte */
  if ((!((pad[si] == '=' && data_len != 0 &&
          data[data_len - 1] == '\n') || pad[si] == '\n'))) {
    /* wrong padding start */
    return -1;
  }

  /* correct padding bytes */
  return 0;
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
sc_scda_get_pad_to_mod (const char *padded_data, size_t padded_len,
                        size_t raw_len, char *raw_data)
{
  SC_ASSERT (padded_data != NULL);
  SC_ASSERT (raw_len == 0 || raw_data != NULL);

  if (padded_len < raw_len || padded_len - raw_len < 7) {
    /* invalid lengths */
    return -1;
  }

  if (sc_scda_check_pad_to_mod (padded_data, raw_len, &padded_data[raw_len],
                                padded_len - raw_len)) {
    /* invalid padding bytes */
    return -1;
  }

  if (raw_len != 0) {
    /* get the raw data if we required raw_data != NULL */
    sc_scda_copy_bytes (raw_data, padded_data, raw_len);
  }

  return 0;
}

/** Check if the given parameters are collective.
 *
 * This function assumes that the parameters \b len1, \b len2 and \b len3 are
 * collective.
 *
 * \param [in] fc           A file context with filled MPI data.
 * \param [in] param1       Pointer to the first parameter to check.
 *                          Must be not NULL.
 * \param [in] len1         The byte count of \b param1.
 * \param [in] param2       Pointer to the second parameter to check.
 *                          May be NULL.
 * \param [in] len2         The byte count of \b param2. If \b param2 is NULL,
 *                          \b len2 must be 0.
 * \param [in] param3       Pointer to the third parameter to check.
 *                          May be NULL and must be NULL if \b param2 is NULL.
 * \param [in] len3         The byte count of \b param3. If \b param3 is NULL,
 *                          \b len3 must be 0.
 * \return                  \ref SC_SCDA_FERR_ARG if the at least one parameter
 *                          does not match in parallel. Otherwise, \ref
 *                          SC_SCDA_FERR_SUCCESS.
 */
static sc_scda_ret_t
sc_scda_check_coll_params (sc_scda_fcontext_t *fc, const char *param1,
                           size_t len1, const char *param2, size_t len2,
                           const char *param3, size_t len3)
{
  int                 mpiret;
  int                 mismatch, collective_mismatch;
  char               *buffer, *recv_buf = NULL;
  size_t              len;

  SC_ASSERT (fc != NULL);
  SC_ASSERT (param1 != NULL);
  SC_ASSERT (param2 != NULL || len2 == 0);
  SC_ASSERT (param3 != NULL || len3 == 0);

  len = len1 + len2 + len3;

  /* allocate contiguous buffer */
  buffer = (char *) SC_ALLOC (char, len);

  /* get buffer with parameter data */
  sc_scda_merge_data_to_buf (param1, len1, param2, len2, param3, len3, buffer);

  /* For the sake of simplicity, we use a Bcast followed by an Allreduce
  * instead of one Allreduce call with a custom reduction function.
  */
  /* In the future we may want to use a checksum on the buffer data if the data
  * is large.
  */
  if (fc->mpirank == 0) {
    mpiret = sc_MPI_Bcast (buffer, (int) len, sc_MPI_BYTE, 0, fc->mpicomm);
  }
  else {
    recv_buf = (char *) SC_ALLOC (char, len);
    mpiret = sc_MPI_Bcast (recv_buf, (int) len, sc_MPI_BYTE, 0, fc->mpicomm);
  }
  SC_CHECK_MPI (mpiret);

  if (fc->mpirank > 0) {
    /* compare data */
    mismatch = memcmp (recv_buf, buffer, len);

    /* free data buffer */
    SC_FREE (buffer);
    SC_FREE (recv_buf);
  }
  else {
    mismatch = 0;
    SC_FREE (buffer);
  }

  /* synchronize comparison results */
  mpiret = sc_MPI_Allreduce (&mismatch, &collective_mismatch, 1, sc_MPI_INT,
                             sc_MPI_LOR, fc->mpicomm);
  SC_CHECK_MPI (mpiret);

  return collective_mismatch ? SC_SCDA_FERR_ARG : SC_SCDA_FERR_SUCCESS;
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
    sc_scda_ret_t       ret;

    /* check if fuzzy_everyn and fuzzy_seed are collective */
    ret = sc_scda_check_coll_params (fc, (const char *) &opt->fuzzy_everyn,
                                     sizeof (unsigned),
                                     (const char *) &opt->fuzzy_seed,
                                     sizeof (sc_rand_state_t), NULL, 0);
    SC_ASSERT (ret == SC_SCDA_FERR_SUCCESS || ret == SC_SCDA_FERR_ARG);

    if (ret == SC_SCDA_FERR_ARG) {
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

  /* initialize output: otherwise we get compiler warnings */
  *out_len = 0;

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

/** Synchronize the random state.
 *
 * This function is required since the fuzzy error state (cf. fuzzy_seed in
 * \ref sc_scda_fcontext) may become unsynchronized due to a different number
 * of samples on different MPI ranks, which is due to non-collective code
 * paths.
 *
 * \param [in, out]  fc      The file context used in the last \ref
 *                           sc_scda_scdaret_to_errcode or \ref
 *                           sc_scda_mpiret_to_errcode call.
 *                           On output \b fuzzy_seed is set to the global
 *                           maximum of all local \b fuzzy_seed values.
 */
static void
sc_scda_fuzzy_sync_state (sc_scda_fcontext_t *fc)
{
  int                 mpiret;
  unsigned long       fuzzy_state, global_state;

  SC_ASSERT (fc != NULL);

  /* get local fuzzy_state */
  fuzzy_state = (unsigned long) fc->fuzzy_seed;

  /* determine maximal state */
  mpiret = sc_MPI_Allreduce (&fuzzy_state, &global_state, 1,
                             sc_MPI_UNSIGNED_LONG, sc_MPI_MAX, fc->mpicomm);
  SC_CHECK_MPI (mpiret);

  /* assign global state */
  fc->fuzzy_seed = (sc_rand_state_t) global_state;
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

/** Write common section data to \b output.
 *
 * All section headers in the scda format contain a part with a
 * section-identifying character followed by the user string. Therefore,
 * we have one internal function to get this data given a \b section_char and
 * a \b user_string.
 *
 * \param [in]  section_char       The section-identifying character.
 * \param [in]  user_string        As in the documentation of \ref
 *                                 sc_scda_fopen_write or any other scda writing
 *                                 function.
 * \param [in]  len                As in the documentation of \ref
 *                                 sc_scda_fopen_write or any other scda writing
 *                                 function.
 * \param [out] output             At least \ref SC_SCDA_COMMON_FIELD bytes.
 *                                 If the function returns false, \b output is
 *                                 filled with the common section header data.
 *                                 Otherwise, it stays untouched.
 * \return                         True if \b user_string is not compliant with
 *                                 the scda file format, i.e. too long or
 *                                 missing nul termination. False, otherwise.
 */
static int
sc_scda_get_common_section_header (char section_char, const char* user_string,
                                   size_t *len, char *output)
{
  int                 invalid_user_string;
  size_t              user_string_len;

  SC_ASSERT (output != NULL);

  /* check the user string */
  invalid_user_string = sc_scda_get_user_string_len (user_string, len,
                                                     &user_string_len);
  if (invalid_user_string) {
    return invalid_user_string;
  }

  /* user string is valid */
  SC_ASSERT (!invalid_user_string);

  /* write the section char */
  output[0] = section_char;
  output[1] = ' ';

  /* write \b user_string to \b output including padding */
  sc_scda_pad_to_fix_len_inplace (user_string, user_string_len, &output[2],
                                  SC_SCDA_USER_STRING_FIELD);

  return invalid_user_string;
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
sc_scda_fopen_write_header_internal (sc_scda_fcontext_t *fc,
                                     const char *user_string, size_t *len,
                                     int *count_err, sc_scda_ferror_t *errcode)
{
  int                 mpiret;
  int                 count;
  int                 current_len;
  int                 invalid_user_string;
  char                file_header_data[SC_SCDA_HEADER_BYTES];

  *count_err = 0;

  /* get scda file header section */
  /* magic */
  sc_scda_copy_bytes (file_header_data, SC_SCDA_MAGIC, SC_SCDA_MAGIC_BYTES);
  current_len = SC_SCDA_MAGIC_BYTES;

  file_header_data[current_len++] = ' ';

  /* vendor string */
  sc_scda_pad_to_fix_len_inplace (SC_SCDA_VENDOR_STRING,
                                  strlen (SC_SCDA_VENDOR_STRING),
                                  &file_header_data[current_len],
                                  SC_SCDA_VENDOR_STRING_FIELD);
  current_len += SC_SCDA_VENDOR_STRING_FIELD;

  /* get common file section header part */
  /* check the user string */
  /* According to 'A.2 Parameter conventions' in the scda specification
   * it is an unchecked runtime error if the user string is not collective,
   * and it leads to undefined behavior.
   * Therefore, we just check the user string on rank 0.
   */
  invalid_user_string =
    sc_scda_get_common_section_header ('F', user_string, len,
                                       &file_header_data[current_len]);
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

  current_len += SC_SCDA_COMMON_FIELD;

  /* pad the file header section */
  sc_scda_pad_to_mod_inplace (NULL, 0, &file_header_data[current_len]);
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

  if (fc->mpirank == SC_SCDA_HEADER_ROOT) {
    sc_scda_fopen_write_header_internal (fc, user_string, len, &count_err,
                                         errcode);
  }
  /* This macro must be the first expression after the non-collective code part
   * since it must be the point where \ref SC_SCDA_CHECK_NONCOLL_ERR and \ref
   * SC_SCDA_CHECK_NONCOLL_COUNT_ERR can jump to, which are called in \ref
   * sc_scda_fopen_write_header_internal. The macro handles the non-collective
   * error, i.e. it broadcasts the errcode, which may encode success, from
   * rank 0 to all other ranks and in case of an error it closes the file,
   * frees the file context and returns NULL. Hence, it is valid that errcode
   * is only initialized on rank SC_SCDA_HEADER_ROOT before calling this macro.
   * This macro is only valid to be called once in a function and this macro is
   * only valid to be called directly after a non-collective code part that
   * contains at least one call \ref SC_SCDA_CHECK_NONCOLL_ERR.
   */
  SC_SCDA_HANDLE_NONCOLL_ERR (errcode, SC_SCDA_HEADER_ROOT, fc);
  /* The macro to check potential non-collective count errors. It is only valid
   * to be called directly after \ref SC_SCDA_HANDLE_NONCOLL_ERR and only
   * if the preceding non-collective code block contains at least one call of
   * \ref SC_SCDA_CHECK_NONCOLL_COUNT_ERR. The macro is only valid to be called
   * once per function. The count error status is broadcasted and the macro
   * prints an error message using \ref SC_LERRORF. This means in particular
   * that it is valid that errcode is only initialized on rank 0 before calling
   * this macro. The macro argument count_err must point to the count error
   * Boolean that was set on rank SC_SCDA_HEADER_ROOT by \ref
   * sc_scda_fopen_write_header_internal.
   */
  SC_SCDA_HANDLE_NONCOLL_COUNT_ERR (errcode, &count_err, SC_SCDA_HEADER_ROOT,
                                    fc);

  /* store number of written bytes */
  fc->accessed_bytes = SC_SCDA_HEADER_BYTES;

  /* initialize remaining file context variables; stay untouched for writing */
  fc->header_before = 0;
  fc->last_type = '\0';

  return fc;
}

/** Internal function to write the inline section header.
 *
 * This function is dedicated to be called in \ref sc_scda_fwrite_inline.
 *
 * \param [in] fc           The file context as in \ref sc_scda_fwrite_inline
 *                          before running the first serial code part.
 * \param [in] user_string  As in the documentation of \ref
 *                          sc_scda_fwrite_inline.
 * \param [in] len          As in the documentation of \ref
 *                          sc_scda_fwrite_inline.
 * \param [out] count_err   A Boolean indicating if a count error occurred.
 * \param [out] errcode     An errcode that can be interpreted by \ref
 *                          sc_scda_ferror_string or mapped to an error class
 *                          by \ref sc_scda_ferror_class.
 */
static void
sc_scda_fwrite_inline_header_internal (sc_scda_fcontext_t *fc,
                                       const char *user_string, size_t *len,
                                       int *count_err,
                                       sc_scda_ferror_t *errcode)
{
  int                 mpiret;
  int                 count;
  int                 current_len;
  int                 invalid_user_string;
  char                header_data[SC_SCDA_COMMON_FIELD];

  *count_err = 0;

  /* get inline file section header */

  /* section-identifying character */
  current_len = 0;

  invalid_user_string =
    sc_scda_get_common_section_header ('I', user_string, len, header_data);
  /* We always translate the error code to have full coverage for the fuzzy
   * error testing.
   */
  sc_scda_scdaret_to_errcode (invalid_user_string ? SC_SCDA_FERR_ARG :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid user string");

  current_len += SC_SCDA_COMMON_FIELD;

  SC_ASSERT (current_len == SC_SCDA_COMMON_FIELD);

  /* write inline section header */
  mpiret = sc_io_write_at (fc->file, fc->accessed_bytes, header_data,
                           SC_SCDA_COMMON_FIELD, sc_MPI_BYTE, &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Writing inline section header");
  SC_SCDA_CHECK_NONCOLL_COUNT_ERR (SC_SCDA_COMMON_FIELD, count, count_err);
}

/** Internal function to write the inline section data.
 *
 * This function is dedicated to be called in \ref sc_scda_fwrite_inline.
 *
 * \param [in] fc           The file context as in \ref sc_scda_fwrite_inline
 *                          before running the first serial code part.
 * \param [in] inline_data  As in the documentation of \ref
 *                          sc_scda_fwrite_inline.
 * \param [out] count_err   A Boolean indicating if a count error occurred.
 * \param [out] errcode     An errcode that can be interpreted by \ref
 *                          sc_scda_ferror_string or mapped to an error class
 *                          by \ref sc_scda_ferror_class.
 */
static void
sc_scda_fwrite_inline_data_internal (sc_scda_fcontext_t *fc,
                                     sc_array_t * inline_data, int *count_err,
                                     sc_scda_ferror_t * errcode)
{
  int                 mpiret;
  int                 count;
  int                 invalid_inline_data;

  *count_err = 0;

  /* check inline data */
  invalid_inline_data = !(inline_data->elem_size == 32 &&
                          inline_data->elem_count == 1);
  sc_scda_scdaret_to_errcode (invalid_inline_data ? SC_SCDA_FERR_ARG :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid inline data");

  /* write the inline data to the file section */
  mpiret = sc_io_write_at (fc->file, fc->accessed_bytes, inline_data->array,
                           SC_SCDA_INLINE_FIELD, sc_MPI_BYTE, &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Writing inline data");
  SC_SCDA_CHECK_NONCOLL_COUNT_ERR (SC_SCDA_INLINE_FIELD, count, count_err);
}

sc_scda_fcontext_t *
sc_scda_fwrite_inline (sc_scda_fcontext_t *fc, const char *user_string,
                       size_t *len, sc_array_t * inline_data, int root,
                       sc_scda_ferror_t * errcode)
{
  int                 count_err;

  SC_ASSERT (fc != NULL);
  SC_ASSERT (user_string != NULL);
  SC_ASSERT (root >= 0);
  /* inline_data is ignored on all ranks except of root */
  SC_ASSERT (fc->mpirank != root || inline_data != NULL);
  SC_ASSERT (errcode != NULL);

  /* The file header section is always written and read on rank 0. */
  if (fc->mpirank == SC_SCDA_HEADER_ROOT) {
    sc_scda_fwrite_inline_header_internal (fc, user_string, len, &count_err,
                                           errcode);
  }
  SC_SCDA_HANDLE_NONCOLL_ERR (errcode, SC_SCDA_HEADER_ROOT, fc);
  SC_SCDA_HANDLE_NONCOLL_COUNT_ERR (errcode, &count_err, SC_SCDA_HEADER_ROOT,
                                    fc);

  /* add number of written bytes */
  fc->accessed_bytes += SC_SCDA_COMMON_FIELD;

  /* The inline data is written on the the user-defined rank root. */
  if (fc->mpirank == root) {
    sc_scda_fwrite_inline_data_internal (fc, inline_data, &count_err,
                                         errcode);
  }
  SC_SCDA_HANDLE_NONCOLL_ERR (errcode, root, fc);
  SC_SCDA_HANDLE_NONCOLL_COUNT_ERR (errcode, &count_err, root, fc);

  fc->accessed_bytes += SC_SCDA_INLINE_FIELD;

  return fc;
}

/** Write the specified count field to \b output.
 *
 * The number of decimal digits is checked in this function. This function is
 * only called in serial code places but we assume that it was checked in
 * advance if the count variable is collective.
 *
 * \param [in]  ident       The char that identifies the count variable.
 * \param [in]  var         The count variable, which must be representable by
 *                          at most \ref SC_SCDA_COUNT_MAX_DIGITS decimal
 *                          digits.
 * \param [out] output      The specified count field. Must be at least \ref
 *                          SC_SCDA_COUNT_FIELD bytes.
 * \return                  True if count is too large and false otherwise.
 */
static int
sc_scda_get_section_header_entry (char ident, size_t var, char *output)
{
  char                var_str[BUFSIZ];
  size_t              len;
#ifdef SC_ENABLE_DEBUG
  long long unsigned  cmp;
#endif

  SC_ASSERT (output != NULL);

  /* write the identifier */
  output[0] = ident;
  output[1] = ' ';

  /* get var as string */
  /* BUFSIZ must be larger than \ref SC_SCDA_COUNT_MAX_DIGITS + 1 to ensure that
   * this code is working.
   * According to C89 section 4.9.2 BUFSIZ shall be at least 256.
   */
  snprintf (var_str, BUFSIZ, "%llu", (long long unsigned) var);
  len = strlen (var_str);

  SC_ASSERT (len > 0);
  if (len > SC_SCDA_COUNT_MAX_DIGITS) {
    /* count value is too large */
    return -1;
  }

#ifdef SC_ENABLE_DEBUG
  /* verify content of var_str */
  SC_ASSERT (sscanf (var_str, "%llu", &cmp) == 1);
  SC_ASSERT (cmp == (unsigned long long) var);
#endif

  /* pad var_str */
  sc_scda_pad_to_fix_len_inplace (var_str, len, &output[2],
                                  SC_SCDA_COUNT_ENTRY);

  return 0;
}

/** Internal function to write the block section header.
 *
 * This function is dedicated to be called in \ref sc_scda_fwrite_block.
 *
 * \param [in] fc           The file context as in \ref sc_scda_fwrite_block
 *                          before running the first serial code part.
 * \param [in] user_string  As in the documentation of \ref
 *                          sc_scda_fwrite_block.
 * \param [in] len          As in the documentation of \ref
 *                          sc_scda_fwrite_block.
 * \param [in] block_size   As in the documentation of \ref
 *                          sc_scda_fwrite_block.
 * \param [out] count_err   A Boolean indicating if a count error occurred.
 * \param [out] errcode     An errcode that can be interpreted by \ref
 *                          sc_scda_ferror_string or mapped to an error class
 *                          by \ref sc_scda_ferror_class.
 */
static void
sc_scda_fwrite_block_header_internal (sc_scda_fcontext_t *fc,
                                      const char *user_string, size_t *len,
                                      size_t block_size, int *count_err,
                                      sc_scda_ferror_t *errcode)
{
  int                 mpiret;
  int                 count;
  int                 current_len;
  int                 invalid_user_string, invalid_count;
  int                 header_len;
  char                header_data[SC_SCDA_COMMON_FIELD + SC_SCDA_COUNT_FIELD];

  *count_err = 0;

  header_len = SC_SCDA_COMMON_FIELD + SC_SCDA_COUNT_FIELD;

  /* get block file section header */

  /* section-identifying character */
  current_len = 0;

  invalid_user_string =
    sc_scda_get_common_section_header ('B', user_string, len, header_data);
  /* We always translate the error code to have full coverage for the fuzzy
   * error testing.
   */
  sc_scda_scdaret_to_errcode (invalid_user_string ? SC_SCDA_FERR_ARG :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid user string");

  current_len += SC_SCDA_COMMON_FIELD;

  /* get count variable entry */
  invalid_count = sc_scda_get_section_header_entry ('E', block_size,
                                                    &header_data[current_len]);
  sc_scda_scdaret_to_errcode (invalid_count ? SC_SCDA_FERR_ARG :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid count");

  current_len += SC_SCDA_COUNT_FIELD;

  SC_ASSERT (current_len == header_len);

  /* write block section header */
  mpiret = sc_io_write_at (fc->file, fc->accessed_bytes, header_data,
                           header_len, sc_MPI_BYTE, &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Writing block section header");
  SC_SCDA_CHECK_NONCOLL_COUNT_ERR (header_len, count, count_err);
}

/** Internal function to write the block section data.
 *
 * This function is dedicated to be called in \ref sc_scda_fwrite_block.
 *
 * \param [in] fc           The file context as in \ref sc_scda_fwrite_block
 *                          before running the first serial code part.
 * \param [in] block_data   As in the documentation of \ref
 *                          sc_scda_fwrite_block.
 * \param [out] count_err   A Boolean indicating if a count error occurred.
 * \param [out] errcode     An errcode that can be interpreted by \ref
 *                          sc_scda_ferror_string or mapped to an error class
 *                          by \ref sc_scda_ferror_class.
 */
static void
sc_scda_fwrite_block_data_internal (sc_scda_fcontext_t *fc,
                                     sc_array_t * block_data, size_t block_size,
                                     int *count_err, sc_scda_ferror_t * errcode)
{
  int                 mpiret;
  int                 count;
  int                 invalid_block_data;
  size_t              num_pad_bytes;
  /* \ref SC_SCDA_PADDING_MOD + 6 is the maximum number of mod padding bytes */
  char                padding[SC_SCDA_PADDING_MOD + 6];

  *count_err = 0;

  /* check block data */
  invalid_block_data = !(block_data->elem_size == block_size &&
                         block_data->elem_count == 1);
  sc_scda_scdaret_to_errcode (invalid_block_data ? SC_SCDA_FERR_ARG :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid block data");

  /* write the block data to the file section */
  mpiret = sc_io_write_at (fc->file, fc->accessed_bytes, block_data->array,
                           block_size, sc_MPI_BYTE, &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Writing block data");
  SC_SCDA_CHECK_NONCOLL_COUNT_ERR (block_size, count, count_err);

  /* get the padding bytes */
  num_pad_bytes = sc_scda_pad_to_mod_len (block_size);
  sc_scda_pad_to_mod (&block_data->array[block_size - 1], block_size, padding);

  /* write the padding bytes */
  mpiret = sc_io_write_at (fc->file, fc->accessed_bytes + block_size,
                           padding, (int) num_pad_bytes, sc_MPI_BYTE, &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Writing block data padding");
  SC_SCDA_CHECK_NONCOLL_COUNT_ERR (num_pad_bytes, count, count_err);
}

sc_scda_fcontext_t *
sc_scda_fwrite_block (sc_scda_fcontext_t *fc, const char *user_string,
                      size_t *len, sc_array_t * block_data, size_t block_size,
                      int root, int encode, sc_scda_ferror_t * errcode)
{
  int                 count_err;
  size_t              num_pad_bytes;
  sc_scda_ret_t       ret;

  SC_ASSERT (fc != NULL);
  SC_ASSERT (user_string != NULL);
  SC_ASSERT (root >= 0);
  /* block_data is ignored on all ranks except of root */
  SC_ASSERT (fc->mpirank != root || block_data != NULL);
  SC_ASSERT (errcode != NULL);

  /* check if block_size is collective */
  ret = sc_scda_check_coll_params (fc, (const char*) &block_size,
                                   sizeof (size_t), NULL, 0, NULL, 0);
  sc_scda_scdaret_to_errcode (ret, errcode, fc);
  SC_SCDA_CHECK_COLL_ERR (errcode, fc, "fwrite_block: block_size is not "
                          "collective");

  /* TODO: respect encode parameter */

  /* section header is always written and read on rank SC_SCDA_HEADER_ROOT */
  if (fc->mpirank == SC_SCDA_HEADER_ROOT) {
    sc_scda_fwrite_block_header_internal (fc, user_string, len, block_size,
                                          &count_err, errcode);
  }
  SC_SCDA_HANDLE_NONCOLL_ERR (errcode, SC_SCDA_HEADER_ROOT, fc);
  SC_SCDA_HANDLE_NONCOLL_COUNT_ERR (errcode, &count_err, SC_SCDA_HEADER_ROOT,
                                    fc);

  /* add number of written bytes */
  fc->accessed_bytes += SC_SCDA_COMMON_FIELD + SC_SCDA_COUNT_FIELD;

  /* The block data is written on the the user-defined rank root. */
  if (fc->mpirank == root) {
    sc_scda_fwrite_block_data_internal (fc, block_data, block_size,
                                        &count_err, errcode);
  }
  SC_SCDA_HANDLE_NONCOLL_ERR (errcode, root, fc);
  SC_SCDA_HANDLE_NONCOLL_COUNT_ERR (errcode, &count_err, root, fc);

  /* get number of padding bytes to update internal file pointer */
  num_pad_bytes = sc_scda_pad_to_mod_len (block_size);

  fc->accessed_bytes += (sc_MPI_Offset) (block_size + num_pad_bytes);

  return fc;
}

/** Internal function to write the array section header.
 *
 * This function is dedicated to be called in \ref sc_scda_fwrite_array.
 *
 * \param [in] fc           The file context as in \ref sc_scda_fwrite_array
 *                          before running the first serial code part.
 * \param [in] user_string  As in the documentation of \ref
 *                          sc_scda_fwrite_array.
 * \param [in] len          As in the documentation of \ref
 *                          sc_scda_fwrite_array.
 * \param [in] elem_count   As in the documentation of \ref
 *                          sc_scda_fwrite_array.
 * \param [in] elem_size    As in the documentation of \ref
 *                          sc_scda_fwrite_array.
 * \param [out] count_err   A Boolean indicating if a count error occurred.
 * \param [out] errcode     An errcode that can be interpreted by \ref
 *                          sc_scda_ferror_string or mapped to an error class
 *                          by \ref sc_scda_ferror_class.
 */
static void
sc_scda_fwrite_array_header_internal (sc_scda_fcontext_t *fc,
                                      const char *user_string, size_t *len,
                                      size_t elem_count, size_t elem_size,
                                      int *count_err, sc_scda_ferror_t *errcode)
{
  int                 mpiret;
  int                 count;
  int                 current_len;
  int                 invalid_user_string, invalid_count;
  int                 header_len;
  char                header_data[SC_SCDA_COMMON_FIELD + 2 * SC_SCDA_COUNT_FIELD];

  *count_err = 0;

  header_len = SC_SCDA_COMMON_FIELD + 2 * SC_SCDA_COUNT_FIELD;

  /* get array file section header */

  /* section-identifying character */
  current_len = 0;

  invalid_user_string =
    sc_scda_get_common_section_header ('A', user_string, len, header_data);
  /* We always translate the error code to have full coverage for the fuzzy
   * error testing.
   */
  sc_scda_scdaret_to_errcode (invalid_user_string ? SC_SCDA_FERR_ARG :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid user string");

  current_len += SC_SCDA_COMMON_FIELD;

  /* get count variable entry */
  /* element count */
  invalid_count = sc_scda_get_section_header_entry ('N', elem_count,
                                                    &header_data[current_len]);
  sc_scda_scdaret_to_errcode (invalid_count ? SC_SCDA_FERR_ARG :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid count");

  current_len += SC_SCDA_COUNT_FIELD;

  /* element size */
  invalid_count = sc_scda_get_section_header_entry ('E', elem_size,
                                                    &header_data[current_len]);
  sc_scda_scdaret_to_errcode (invalid_count ? SC_SCDA_FERR_ARG :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid count");

  current_len += SC_SCDA_COUNT_FIELD;

  SC_ASSERT (current_len == header_len);

  /* write array section header */
  mpiret = sc_io_write_at (fc->file, fc->accessed_bytes, header_data,
                           header_len, sc_MPI_BYTE, &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Writing array section header");
  SC_SCDA_CHECK_NONCOLL_COUNT_ERR (header_len, count, count_err);
}

/** Internal function to write the fixed-length array padding.
 *
 * This function is dedicated to be called in \ref sc_scda_fwrite_array.
 * This function is only valid to be called on the maximal not empty rank.
 *
 * \param [in] fc           The file context as in \ref sc_scda_fwrite_array
 *                          after running the collective write part.
 * \param [in] last_byte    A pointer to the last global data byte.
 * \param [in] byte_count   Number of collectively written bytes in the function
 *                          \ref sc_scda_fwrite_array.
 * \param [out] count_err   A Boolean indicating if a count error occurred.
 * \param [out] errcode     An errcode that can be interpreted by \ref
 *                          sc_scda_ferror_string or mapped to an error class
 *                          by \ref sc_scda_ferror_class.
 */
static void
sc_scda_fwrite_padding_internal (sc_scda_fcontext_t *fc, const char *last_byte,
                                 size_t byte_count, int *count_err,
                                 sc_scda_ferror_t *errcode)
{
  int                 mpiret;
  int                 count;
  size_t              num_pad_bytes;
  /* \ref SC_SCDA_PADDING_MOD + 6 is the maximum number of mod padding bytes */
  char                padding[SC_SCDA_PADDING_MOD + 6];

  *count_err = 0;

  /* get the padding bytes */
  num_pad_bytes = sc_scda_pad_to_mod_len (byte_count);
  sc_scda_pad_to_mod (last_byte, byte_count, padding);

  /* write the padding bytes */
  mpiret = sc_io_write_at (fc->file, fc->accessed_bytes, padding,
                           (int) num_pad_bytes, sc_MPI_BYTE, &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Writing fixed-len. array data padding");
  SC_SCDA_CHECK_NONCOLL_COUNT_ERR (num_pad_bytes, count, count_err);
}

/** Determine the maximal rank that is not empty.
 *
 * \param [in] fc           A file context with filled MPI data.
 * \param [in] elem_counts  As the parameter \b elem_counts in \ref
 *                          sc_scda_fwrite_array or \ref sc_scda_fwrite_varray.
 *                          This array stores the number of elements per rank.
 * \return                  The maximal rank that is not empty. If all ranks
 *                          are empty, 0 is returned.
 */
static int
sc_scda_get_last_byte_owner (sc_scda_fcontext_t *fc, sc_array_t *elem_counts)
{
  int                 i;
  int                 last_byte_owner;

  SC_ASSERT (fc != NULL);
  SC_ASSERT (elem_counts != NULL);

  /* determine the rank that holds the last byte */
  last_byte_owner = 0;
  for (i = fc->mpisize - 1; i >= 0; --i) {
    if (*((sc_scda_ulong *)sc_array_index_int (elem_counts, i)) != 0) {
      /* found maximal rank that is not empty */
      last_byte_owner = i;
      break;
    }
  }

  return last_byte_owner;
}

sc_scda_fcontext_t *
sc_scda_fwrite_array (sc_scda_fcontext_t *fc, const char *user_string,
                      size_t *len, sc_array_t *array_data,
                      sc_array_t *elem_counts, size_t elem_size, int indirect,
                      int encode, sc_scda_ferror_t *errcode)
{
  int                 mpiret;
  int                 i;
  int                 invalid_elem_counts, global_invalid_elem_counts;
  int                 ret, count_err;
  int                 num_local_elements, bytes_to_write;
  int                 count;
  int                 last_byte_owner;
  size_t              elem_count, si;
  size_t              collective_byte_count;
  size_t              num_pad_bytes;
  sc_MPI_Offset       offset;
#if 0
  const void         *local_array_data;
#endif

  SC_ASSERT (fc != NULL);
  SC_ASSERT (user_string != NULL);
  SC_ASSERT (array_data != NULL);
  SC_ASSERT (elem_counts != NULL);
  SC_ASSERT (errcode != NULL);

  /* TODO: Also check elem_counts and/or indirect? */
  /* check if elem_size is collective */
  ret = sc_scda_check_coll_params (fc, (const char *) &elem_size,
                                   sizeof (size_t), NULL, 0, NULL, 0);
  sc_scda_scdaret_to_errcode (ret, errcode, fc);
  SC_SCDA_CHECK_COLL_ERR (errcode, fc, "fwrite_array: elem_size is not "
                          "collective");

  /* TODO: respect encode parameter */

  /* check elem_counts array */
  invalid_elem_counts = !(elem_counts->elem_size == sizeof (sc_scda_ulong) &&
                          elem_counts->elem_count == fc->mpisize);
  /* synchronize */
  mpiret =
    sc_MPI_Allreduce (&invalid_elem_counts, &global_invalid_elem_counts, 1,
                      sc_MPI_INT, sc_MPI_LOR, fc->mpicomm);
  SC_CHECK_MPI (mpiret);
  sc_scda_scdaret_to_errcode (invalid_elem_counts ? SC_SCDA_FERR_ARG :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  SC_SCDA_CHECK_COLL_ERR (errcode, fc, "Invalid elem_counts array");

  /* compute the global element count */
  /* TODO: Use a checksum for the colletivness test? */
  elem_count = 0;
  for (si = 0; si < elem_counts->elem_count; ++si) {
    elem_count += *((size_t *) sc_array_index (elem_counts, si));
  }

  ret = sc_scda_check_coll_params (fc, (const char *) &elem_count,
                                   sizeof (size_t), (const char *) &elem_size,
                                   sizeof (size_t), NULL, 0);
  sc_scda_scdaret_to_errcode (ret, errcode, fc);
  SC_SCDA_CHECK_COLL_ERR (errcode, fc,
                          "fwrite_array: elem_counts or elem_size"
                          " is not collective");

  /* TODO: check array_data; depends on indirect parameter */
  /* Call Allreduce to synchronize on check of array_data or collective test? */

  /* section header is always written and read on rank SC_SCDA_HEADER_ROOT */
  if (fc->mpirank == SC_SCDA_HEADER_ROOT) {
    sc_scda_fwrite_array_header_internal (fc, user_string, len, elem_count,
                                          elem_size, &count_err, errcode);
  }
  SC_SCDA_HANDLE_NONCOLL_ERR (errcode, SC_SCDA_HEADER_ROOT, fc);
  SC_SCDA_HANDLE_NONCOLL_COUNT_ERR (errcode, &count_err, SC_SCDA_HEADER_ROOT,
                                    fc);

  /* add number of written bytes */
  fc->accessed_bytes += SC_SCDA_COMMON_FIELD + 2 * SC_SCDA_COUNT_FIELD;

#if 0
  /* get pointer to local array data */
  if (indirect) {
    /* indirect addressing */
    /* TODO: copy the data or use muliple write calls; maybe in batches? */
  }
  else {
    /* direct addressing */
    local_array_data = (const void *) array_data->array;
  }

  /* write array data in parallel */
#endif

  /* TODO: temporary */
  SC_ASSERT (!indirect);

  /* compute rank-dependent offset */
  offset = 0;
  /* sum all element counts on previous processes */
  for (i = 0; i < fc->mpirank; ++i) {
    offset +=
      (sc_MPI_Offset) *
      ((sc_scda_ulong *) sc_array_index_int (elem_counts, i));
  }
  offset *= (sc_MPI_Offset) elem_size;

  /* computer number of array data bytes that are locally written */
  num_local_elements =
    (int) *((sc_scda_ulong *) sc_array_index_int (elem_counts, fc->mpirank));
  bytes_to_write = (int) elem_size * num_local_elements;

  mpiret = sc_io_write_at_all (fc->file, fc->accessed_bytes + offset,
                               array_data->array, bytes_to_write, sc_MPI_BYTE,
                               &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_COLL_ERR (errcode, fc, "Writing fixed-length array padding");
  /* check for count error of the collective I/O operation */
  SC_SCDA_CHECK_COLL_COUNT_ERR (bytes_to_write, count, fc, errcode);

  /* update global number of written bytes */
  collective_byte_count = elem_count * elem_size;
  fc->accessed_bytes += (sc_MPI_Offset) collective_byte_count;

  /* determine the rank that holds the last byte */
  last_byte_owner = sc_scda_get_last_byte_owner (fc, elem_counts);

  /* get and write padding bytes in serial */
  if (fc->mpirank == last_byte_owner) {
    const char       *last_byte;

    /* get last local/global byte */
    SC_ASSERT (elem_count == 0 || bytes_to_write > 0);
    last_byte = (elem_count > 0) ?
                              &array_data->array[bytes_to_write - 1] : NULL;
    /* the padding depends on the last data byte */
    sc_scda_fwrite_padding_internal (fc, last_byte, collective_byte_count,
                                     &count_err, errcode);
  }
  SC_SCDA_HANDLE_NONCOLL_ERR (errcode, last_byte_owner, fc);
  SC_SCDA_HANDLE_NONCOLL_COUNT_ERR (errcode, &count_err, last_byte_owner, fc);

  /* update global number of written bytes */
  num_pad_bytes = sc_scda_pad_to_mod_len (collective_byte_count);
  fc->accessed_bytes += (sc_MPI_Offset) num_pad_bytes;

  return fc;
}

/** Check a read file header section and extract the user string.
 *
 * \param [in] file_header_data The read file header data as a byte buffer.
 * \param [out] user_string     On output the read user string including
 *                              nul-termination. At least \ref
 *                              SC_SCDA_USER_STRING_BYTES + 1 bytes. Must be
 *                              initialized with '\0'.
 * \param [out] len             On output \b len is set to the number of bytes
 *                              written to \b user_string excluding the
 *                              terminating nul.
 * \return                      -1 for an invalid header and 0 for a valid
 *                              file header. In the first case the output
 *                              parameters are undefined.
 */
static int
sc_scda_check_file_header (const char *file_header_data, char *user_string,
                           size_t *len)
{
  int                 current_pos;
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
  if (sc_scda_check_pad_to_fix_len (&file_header_data[current_pos],
                                    SC_SCDA_VENDOR_STRING_FIELD, &vendor_len)) {
    /* wrong padding format */
    return -1;
  }
  /* vendor string content is not checked and hence not read */

  current_pos += SC_SCDA_VENDOR_STRING_FIELD + 2;
  /* check the user string */
  if (sc_scda_get_pad_to_fix_len
      (&file_header_data
       [current_pos], SC_SCDA_USER_STRING_FIELD, user_string, len)) {
    /* wrong padding format */
    return -1;
  }
  /* the user string content is not checked */

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
sc_scda_fopen_read_header_internal (sc_scda_fcontext_t * fc,
                                    char *user_string, size_t *len,
                                    int *count_err, sc_scda_ferror_t *errcode)
{
  int                 mpiret;
  int                 count;
  int                 invalid_file_header;
  char                file_header_data[SC_SCDA_HEADER_BYTES];

  *count_err = 0;

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

  /* read file header section on rank SC_SCDA_HEADER_ROOT */
  if (fc->mpirank == SC_SCDA_HEADER_ROOT) {
    sc_scda_fopen_read_header_internal (fc, user_string, len, &count_err,
                                        errcode);
  }
  /* The macro to handle a non-collective error that is associated to a
   * preceding call of \ref SC_SCDA_CHECK_NONCOLL_ERR.
   * More information can be found in the comments in \ref sc_scda_fopen_write
   * and in the documentation of the \ref SC_SCDA_HANDLE_NONCOLL_ERR.
   */
  SC_SCDA_HANDLE_NONCOLL_ERR (errcode, SC_SCDA_HEADER_ROOT, fc);
  /* The macro to handle a non-collective count error that is associated to a
   * preceding call of \ref SC_SCDA_CHECK_NONCOLL_COUNT_ERR.
   * More information can be found in the comments in \ref sc_scda_fopen_write
   * and in the documentation of the \ref SC_SCDA_HANDLE_NONCOLL_COUNT_ERR.
   */
  SC_SCDA_HANDLE_NONCOLL_COUNT_ERR (errcode, &count_err, SC_SCDA_HEADER_ROOT,
                                    fc);
  /* Bcast the user string */
  mpiret = sc_MPI_Bcast (user_string, SC_SCDA_USER_STRING_BYTES + 1,
                         sc_MPI_BYTE, SC_SCDA_HEADER_ROOT, mpicomm);
  SC_CHECK_MPI (mpiret);

  /* store the number of read bytes */
  fc->accessed_bytes = SC_SCDA_HEADER_BYTES;

  /* initialize remaining file context variables */
  fc->header_before = 0;
  fc->last_type = '\0';

  return fc;
}

/** Internal function to read and check the common part of file section header.
 *
 * \param [in] fc           The file context as in \ref
 *                          sc_scda_fread_section_header before running the
 *                          first serial code part.
 * \param [out] type        As in the documentation of \ref
 *                          sc_scda_fread_section_header.
 * \param [out] user_string As in the documentation of \ref
 *                          sc_scda_fread_section_header.
 * \param [out] len         As in the documentation of \ref
 *                          sc_scda_fread_section_header.
 * \param [out] count_err   A Boolean indicating if a count error occurred.
 * \param [out] errcode     An errcode that can be interpreted by \ref
 *                          sc_scda_ferror_string or mapped to an error class
 *                          by \ref sc_scda_ferror_class.
 */
static void
sc_scda_fread_section_header_common_internal (sc_scda_fcontext_t *fc,
                                              char *type, char *user_string,
                                              size_t *len, int *count_err,
                                              sc_scda_ferror_t *errcode)
{
  int                 mpiret;
  int                 count;
  int                 wrong_format;
  char                common[SC_SCDA_COMMON_FIELD];

  *count_err = 0;

  /* read common file section header */
  mpiret = sc_io_read_at (fc->file, fc->accessed_bytes, common,
                          SC_SCDA_COMMON_FIELD, sc_MPI_BYTE, &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Read common file section header part");
  SC_SCDA_CHECK_NONCOLL_COUNT_ERR (SC_SCDA_COMMON_FIELD, count, count_err);

  wrong_format = 0;
  /* check file section type */
  switch (common[0]) {
  case 'I':
    *type = 'I';
    break;
  case 'B':
    *type = 'B';
    break;
  case 'A':
    *type = 'A';
    break;
  default:
    /* an invalid/unsupported format */
    wrong_format = 1;
  }
  sc_scda_scdaret_to_errcode (wrong_format ? SC_SCDA_FERR_FORMAT :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid file section type");

  /* check common file section header format */
  if (common[1] != ' ') {
    /* wrong format */
    wrong_format = 1;
  }
  sc_scda_scdaret_to_errcode (wrong_format ? SC_SCDA_FERR_FORMAT :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Missing space in file section header");

  /* initialize user_string */
  sc_scda_init_nul (user_string, SC_SCDA_USER_STRING_BYTES + 1);

  /* check and extract the user string */
  if (sc_scda_get_pad_to_fix_len
      (&common[2], SC_SCDA_USER_STRING_FIELD, user_string, len)) {
    /* wrong padding format */
    wrong_format = 1;
  }
  sc_scda_scdaret_to_errcode (wrong_format ? SC_SCDA_FERR_FORMAT :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode,
                             "Invalid user string in section header");
}

/** Internal function to check a count entry in a section header.
 *
 * This code is only valid to be run in serial.
 *
 * \param [in] fc           The file context as in \ref
 *                          sc_scda_fread_section_header before running the
 *                          second serial code part.
 * \param [out] ident       The character that identifies the count entry.
 *                          If this character is not conforming to the scda
 *                          convention, the function call is not completed and
 *                          results in \ref SC_SCDA_FERR_FORMAT as error,
 *                          cf. \b errcode.
 * \param [in] expc_ident   The expected count entry identifier. If the read
 *                          identifier is not as expected the function returns
 *                          false.
 * \param [out] count_var   The count variable read from the count entry.
 * \param [out] count_err   A Boolean indicating if a count error occurred.
 *                          For this parameter the term count refers to the
 *                          expected byte count for reading count (different
 *                          count) entry.
 * \return                  True if the count entry is valid and has the
 *                          expected identifier.
 */
static int
sc_scda_check_count_entry_internal (const char *count_entry, char expc_ident,
                                    size_t *count_var)
{
  char                var_str[SC_SCDA_COUNT_MAX_DIGITS + 1];
  char                ident;
  int                 wrong_format, wrong_ident;
  long long unsigned  read_count;
  size_t              len = 0;

  SC_ASSERT (count_var != NULL);

  *count_var = 0;

  wrong_format = 0;
  /* check and get the count variable identifier */
  switch (count_entry[0])
  {
  case 'E':
    ident = 'E';
    break;
  case 'N':
    ident = 'N';
    break;
  default:
    /* invalid/unsupported count identifier */
    wrong_format = 1;
    break;
  }
  if (wrong_format) {
    /* invalid count identifier */
    return -1;
  }

  /* compare read count identifier to the expected count identifier  */
  wrong_ident = (ident != expc_ident);
  if (wrong_ident) {
    /* Wrong count identifier in count entry */
    return -1;
  }

  /* check count entry format */
  if (count_entry[1] != ' ') {
    /* wrong format */
    wrong_format = 1;
  }
  if (wrong_format) {
    /* Missing space in count entry */
    return -1;
  }

  /* check padding and extract count variable string */
  sc_scda_init_nul (var_str, SC_SCDA_COUNT_MAX_DIGITS + 1);
  if (sc_scda_get_pad_to_fix_len (&count_entry[2], SC_SCDA_COUNT_ENTRY, var_str,
                                  &len)) {
    wrong_format = 1;
  }
  if (wrong_format) {
    /* Invalid count variable padding */
    return -1;
  }

  /* If the padding to the length \ref SC_SCDA_COUNT_ENTRY was valid this
   * assertion must hold.
   */
  SC_ASSERT (len <= SC_SCDA_COUNT_MAX_DIGITS);

  /* get count variable value */
  /* The initialization above guarantees that var_str is nul-terminated. */
  if (len == 0 || sscanf (var_str, "%llu", &read_count) != 1) {
    /* conversion failed or is not possible */
    wrong_format = 1;
  }
  if (wrong_format) {
    /* Extraction of count value failed */
    return -1;
  }

  *count_var = (size_t) read_count;

  return 0;
}

/** An internal function to read and check the block section header.
 *
 * This function is only valid to be called in serial.
 * This function updates \b fc->accessed_bytes. Hence, it is crucial that
 * \b fc->accessed_bytes is set collectivley afterwards.
 *
 * \param [in]  fc          The file context as in \ref
 *                          sc_scda_fread_section_header before running the
 *                          serial code part.
 * \param [in]  elem_size   The read element size.
 * \param [out] count_err   A Boolean indicating if a count error occurred.
 * \param [out] errcode     An errcode that can be interpreted by \ref
 *                          sc_scda_ferror_string or mapped to an error class
 *                          by \ref sc_scda_ferror_class.
 */
static void
sc_scda_fread_block_header_internal (sc_scda_fcontext_t *fc, size_t *elem_size,
                                     int *count_err, sc_scda_ferror_t *errcode)
{
  char                count_entry[SC_SCDA_COUNT_FIELD];
  int                 mpiret;
  int                 count;
  int                 invalid_count_entry;

  SC_ASSERT (fc != NULL);
  SC_ASSERT (count_err != NULL);
  SC_ASSERT (errcode != NULL);

  *count_err = 0;

  /* read the count entry */
  mpiret = sc_io_read_at (fc->file, fc->accessed_bytes, count_entry,
                          SC_SCDA_COUNT_FIELD, sc_MPI_BYTE, &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Read block section header count entry");
  SC_SCDA_CHECK_NONCOLL_COUNT_ERR (SC_SCDA_COUNT_FIELD, count, count_err);

  /* check read count entry */
  invalid_count_entry = sc_scda_check_count_entry_internal (count_entry, 'E',
                                                            elem_size);
  sc_scda_scdaret_to_errcode (invalid_count_entry ? SC_SCDA_FERR_FORMAT :
                                                    SC_SCDA_FERR_SUCCESS,
                              errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid block count entry");

  /* update the internal file pointer; only in serial */
  fc->accessed_bytes += SC_SCDA_COUNT_FIELD;
}

/** An internal function to read and check the fixed-len array section header.
 *
 * This function is only valid to be called in serial.
 * This function updates \b fc->accessed_bytes. Hence, it is crucial that
 * \b fc->accessed_bytes is set collectivley afterwards.
 *
 * \param [in]  fc          The file context as in \ref
 *                          sc_scda_fread_section_header before running the
 *                          serial code part.
 * \param [in]  elem_size   The read element size.
 * \param [out] count_err   A Boolean indicating if a count error occurred.
 * \param [out] errcode     An errcode that can be interpreted by \ref
 *                          sc_scda_ferror_string or mapped to an error class
 *                          by \ref sc_scda_ferror_class.
 */
static void
sc_scda_fread_array_header_internal (sc_scda_fcontext_t *fc, size_t *elem_count,
                                     size_t *elem_size, int *count_err,
                                     sc_scda_ferror_t *errcode)
{
  char                count_entry[SC_SCDA_COUNT_FIELD];
  int                 mpiret;
  int                 count;
  int                 invalid_count_entry;

  SC_ASSERT (fc != NULL);
  SC_ASSERT (count_err != NULL);
  SC_ASSERT (errcode != NULL);

  *count_err = 0;

  /* read the first count entry */
  mpiret = sc_io_read_at (fc->file, fc->accessed_bytes, count_entry,
                          SC_SCDA_COUNT_FIELD, sc_MPI_BYTE, &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Read block section header count entry");
  SC_SCDA_CHECK_NONCOLL_COUNT_ERR (SC_SCDA_COUNT_FIELD, count, count_err);

  /* check read count entry */
  invalid_count_entry = sc_scda_check_count_entry_internal (count_entry,'N',
                                                            elem_count);
  sc_scda_scdaret_to_errcode (invalid_count_entry ? SC_SCDA_FERR_FORMAT :
                                                    SC_SCDA_FERR_SUCCESS,
                              errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid first fixed-length array count "
                             "entry");

  /* update the internal file pointer; only in serial */
  fc->accessed_bytes += SC_SCDA_COUNT_FIELD;

  /* read the second count entry */
  mpiret = sc_io_read_at (fc->file, fc->accessed_bytes, count_entry,
                          SC_SCDA_COUNT_FIELD, sc_MPI_BYTE, &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Read block section header count entry");
  SC_SCDA_CHECK_NONCOLL_COUNT_ERR (SC_SCDA_COUNT_FIELD, count, count_err);

  /* check read count entry */
  invalid_count_entry = sc_scda_check_count_entry_internal (count_entry, 'E',
                                                            elem_size);
  sc_scda_scdaret_to_errcode (invalid_count_entry ? SC_SCDA_FERR_FORMAT :
                                                    SC_SCDA_FERR_SUCCESS,
                              errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid second fixed-length array count "
                             "entry");

  /* update the internal file pointer; only in serial */
  fc->accessed_bytes += SC_SCDA_COUNT_FIELD;
}

sc_scda_fcontext_t *
sc_scda_fread_section_header (sc_scda_fcontext_t *fc, char *user_string,
                              size_t *len, char *type, size_t *elem_count,
                              size_t *elem_size, int *decode,
                              sc_scda_ferror_t *errcode)
{
  int                 count_err;
  int                 mpiret;

  SC_ASSERT (fc != NULL);
  SC_ASSERT (user_string != NULL);
  SC_ASSERT (type != NULL);
  SC_ASSERT (elem_count != NULL);
  SC_ASSERT (elem_size != NULL);
  SC_ASSERT (decode != NULL);
  SC_ASSERT (errcode != NULL);

  *elem_count = 0;
  *elem_size = 0;

  /* read the common section header part first */
  if (fc->mpirank == SC_SCDA_HEADER_ROOT) {
    sc_scda_fread_section_header_common_internal (fc, type, user_string, len,
                                                  &count_err, errcode);
  }
  SC_SCDA_HANDLE_NONCOLL_ERR (errcode, SC_SCDA_HEADER_ROOT, fc);
  SC_SCDA_HANDLE_NONCOLL_COUNT_ERR (errcode, &count_err, SC_SCDA_HEADER_ROOT,
                                    fc);

  fc->accessed_bytes += SC_SCDA_COMMON_FIELD;

  if (fc->mpirank == SC_SCDA_HEADER_ROOT) {
    /* read count entries */
    switch (*type)
    {
    case 'I':
      /* no count entries to read */
      break;
    case 'B':
      sc_scda_fread_block_header_internal (fc, elem_size, &count_err, errcode);
      break;
    case 'A':
      sc_scda_fread_array_header_internal (fc, elem_count, elem_size,
                                           &count_err, errcode);
      break;
    default:
      /* rank SC_SCDA_HEADER_ROOT already checked if type is valid/supported */
      SC_ABORT_NOT_REACHED ();
    }
  }
  SC_SCDA_HANDLE_NONCOLL_ERR (errcode, SC_SCDA_HEADER_ROOT, fc);
  SC_SCDA_HANDLE_NONCOLL_COUNT_ERR (errcode, &count_err, SC_SCDA_HEADER_ROOT,
                                    fc);

  /* Bcast type and user string */
  mpiret = sc_MPI_Bcast (type, 1, sc_MPI_CHAR, SC_SCDA_HEADER_ROOT,
                         fc->mpicomm);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Bcast (user_string, SC_SCDA_USER_STRING_BYTES + 1,
                         sc_MPI_BYTE, SC_SCDA_HEADER_ROOT, fc->mpicomm);
  SC_CHECK_MPI (mpiret);

  /* set global outputs and Bcast the counts if it is necessary */
  switch (*type) {
  /* set elem_count and elem_size according to the scda convention */
  /* TODO: Handle decode parameter */
  case 'I':
    /* inline */
    *elem_count = 0;
    *elem_size = 0;
    break;
  case 'B':
    /* block */
    *elem_count = 0;
    /* elem_size was read on rank SC_SCDA_HEADER_ROOT */
    mpiret = sc_MPI_Bcast (elem_size, sizeof (size_t), sc_MPI_BYTE,
                           SC_SCDA_HEADER_ROOT, fc->mpicomm);
    SC_CHECK_MPI (mpiret);
    if (fc->mpirank != SC_SCDA_HEADER_ROOT) {
      /* update internal file pointer */
      fc->accessed_bytes += SC_SCDA_COUNT_FIELD;
    }
    break;
  case 'A':
    /* fixed-length array */
    /* elem_count and elem_size were read on rank SC_SCDA_HEADER_ROOT */
    mpiret = sc_MPI_Bcast (elem_count, sizeof (size_t), sc_MPI_BYTE,
                           SC_SCDA_HEADER_ROOT, fc->mpicomm);
    SC_CHECK_MPI (mpiret);
    mpiret = sc_MPI_Bcast (elem_size, sizeof (size_t), sc_MPI_BYTE,
                           SC_SCDA_HEADER_ROOT, fc->mpicomm);
    SC_CHECK_MPI (mpiret);
    if (fc->mpirank != SC_SCDA_HEADER_ROOT) {
      /* update internal file pointer */
      fc->accessed_bytes += 2 * SC_SCDA_COUNT_FIELD;
    }
    break;
  default:
    /* rank SC_SCDA_HEADER_ROOT already checked if type is valid/supported */
    SC_ABORT_NOT_REACHED ();
  }

  /* this is to check if the scda workflow is respected */
  fc->header_before = 1;
  fc->last_type = *type;

  return fc;
}

/** Internal function to read the inline data.
 *
 * \param [in] fc           The file context as in \ref
 *                          sc_scda_fread_inline_data before running the
 *                          serial code part.
 * \param [out] data        As in the documentation of \ref
 *                          sc_scda_fread_inline_data.
 * \param [out] count_err   A Boolean indicating if a count error occurred.
 * \param [out] errcode     An errcode that can be interpreted by \ref
 *                          sc_scda_ferror_string or mapped to an error class
 *                          by \ref sc_scda_ferror_class.
 */
static void
sc_scda_fread_inline_data_serial_internal (sc_scda_fcontext_t *fc,
                                           sc_array_t *data, int *count_err,
                                           sc_scda_ferror_t *errcode)
{
  int                 mpiret;
  int                 count;
  int                 invalid_array;

  *count_err = 0;

  /* check the passed sc_array */
  invalid_array = !(data->elem_count == 1 && data->elem_size == 32);
  sc_scda_scdaret_to_errcode (invalid_array ? SC_SCDA_FERR_ARG :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);

  /* read inline data  */
  mpiret = sc_io_read_at (fc->file, fc->accessed_bytes, data->array,
                          SC_SCDA_INLINE_FIELD, sc_MPI_BYTE, &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Read inline data");
  SC_SCDA_CHECK_NONCOLL_COUNT_ERR (SC_SCDA_INLINE_FIELD, count, count_err);
  /* there are no conditions on the inline data and hence no checks */
}

sc_scda_fcontext_t *
sc_scda_fread_inline_data (sc_scda_fcontext_t *fc, sc_array_t *data, int root,
                           sc_scda_ferror_t *errcode)
{
  int                 count_err;
  int                 wrong_usage;

  SC_ASSERT (fc != NULL);
  SC_ASSERT (root >= 0);
  SC_ASSERT (errcode != NULL);

  /* It is necessary that sc_scda_fread_section_header was called as last
   * function call on fc and that it returned the inline section type.
   */
  wrong_usage = !(fc->header_before && fc->last_type == 'I');
  sc_scda_scdaret_to_errcode (wrong_usage ? SC_SCDA_FERR_USAGE :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  SC_SCDA_CHECK_COLL_ERR (errcode, fc, "Wrong usage of scda functions");

  if (data != NULL) {
    /* the data is not skipped */
    if (fc->mpirank == root) {
      sc_scda_fread_inline_data_serial_internal (fc, data, &count_err, errcode);
    }
    SC_SCDA_HANDLE_NONCOLL_ERR (errcode, root, fc);
    SC_SCDA_HANDLE_NONCOLL_COUNT_ERR (errcode, &count_err, root, fc);
  }

  /* if no error occurred, we move the internal file pointer */
  fc->accessed_bytes += SC_SCDA_INLINE_FIELD;

  /* last function call can not be \ref sc_scda_fread_section_header anymore */
  fc->header_before = 0;

  return fc;
}

/** Internal function to read the block data.
 *
 * \param [in] fc           The file context as in \ref
 *                          sc_scda_fread_block_data before running the
 *                          serial code part.
 * \param [out] data        As in the documentation of \ref
 *                          sc_scda_fread_block_data.
 * \param [in]  block_size  As in the documentation of \ref
 *                          sc_scda_fread_block_data.
 * \param [out] count_err   A Boolean indicating if a count error occurred.
 * \param [out] errcode     An errcode that can be interpreted by \ref
 *                          sc_scda_ferror_string or mapped to an error class
 *                          by \ref sc_scda_ferror_class.
 */
static void
sc_scda_fread_block_data_serial_internal (sc_scda_fcontext_t *fc,
                                          sc_array_t *data, size_t block_size,
                                          int *count_err,
                                          sc_scda_ferror_t *errcode)
{
  int                 mpiret;
  int                 count;
  int                 invalid_array, invalid_padding;
  size_t              num_pad_bytes;
  char                paddding[SC_SCDA_PADDING_MOD_MAX];

  *count_err = 0;

  /* check the passed sc_array */
  invalid_array = !(data->elem_count == 1 && data->elem_size == block_size);
  sc_scda_scdaret_to_errcode (invalid_array ? SC_SCDA_FERR_ARG :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid block array during reading");

  /* read block data  */
  mpiret = sc_io_read_at (fc->file, fc->accessed_bytes, data->array,
                          (int) block_size, sc_MPI_BYTE, &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Read inline data");
  SC_SCDA_CHECK_NONCOLL_COUNT_ERR (block_size, count, count_err);

  num_pad_bytes = sc_scda_pad_to_mod_len (block_size);

  /* read the padding the bytes */
  /* the padding depends on the trailing byte of the data */
  mpiret = sc_io_read_at (fc->file, fc->accessed_bytes +
                          (sc_MPI_Offset) block_size, paddding,
                          (int) num_pad_bytes, sc_MPI_BYTE, &count);
  sc_scda_mpiret_to_errcode (mpiret, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Read inline data padding");
  SC_SCDA_CHECK_NONCOLL_COUNT_ERR (num_pad_bytes, count, count_err);

  /* check the padding */
  invalid_padding = sc_scda_check_pad_to_mod (data->array, block_size, paddding,
                                              num_pad_bytes);
  sc_scda_scdaret_to_errcode (invalid_padding ? SC_SCDA_FERR_FORMAT :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  SC_SCDA_CHECK_NONCOLL_ERR (errcode, "Invalid block data padding");
}

sc_scda_fcontext_t *
sc_scda_fread_block_data (sc_scda_fcontext_t *fc, sc_array_t *block_data,
                          size_t block_size, int root,
                          sc_scda_ferror_t *errcode)
{
  int                 count_err;
  int                 wrong_usage;
  sc_scda_ret_t       ret;

  SC_ASSERT (fc != NULL);
  SC_ASSERT (root >= 0);
  SC_ASSERT (errcode != NULL);

  /* check if block_size is collective */
  ret = sc_scda_check_coll_params (fc, (const char*) &block_size,
                                   sizeof (size_t), NULL, 0, NULL, 0);
  sc_scda_scdaret_to_errcode (ret, errcode, fc);
  SC_SCDA_CHECK_COLL_ERR (errcode, fc, "fread_block_data: block_size is not "
                          "collective");

  /* It is necessary that sc_scda_fread_section_header was called as last
   * function call on fc and that it returned the block section type.
   */
  wrong_usage = !(fc->header_before && fc->last_type == 'B');
  sc_scda_scdaret_to_errcode (wrong_usage ? SC_SCDA_FERR_USAGE :
                              SC_SCDA_FERR_SUCCESS, errcode, fc);
  SC_SCDA_CHECK_COLL_ERR (errcode, fc, "Wrong usage of scda functions");

  if (block_data != NULL) {
    /* the data is not skipped */
    if (fc->mpirank == root) {
      sc_scda_fread_block_data_serial_internal (fc, block_data, block_size,
                                                &count_err, errcode);
    }
    SC_SCDA_HANDLE_NONCOLL_ERR (errcode, root, fc);
    SC_SCDA_HANDLE_NONCOLL_COUNT_ERR (errcode, &count_err, root, fc);
  }

  /* if no error occurred, we move the internal file pointer */
  fc->accessed_bytes += (sc_MPI_Offset) (block_size +
                                          sc_scda_pad_to_mod_len (block_size));

  /* last function call can not be \ref sc_scda_fread_section_header anymore */
  fc->header_before = 0;

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
