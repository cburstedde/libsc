#include <sc_file.h>
#include <sc_io.h>

typedef struct sc_file_context
{
  sc_MPI_Comm         mpicomm;            /**< corresponding MPI communicator */
  uint64_t           *global_first;       /**< represents the partition */
  int                 gf_owned;          /**< Boolean to indicate if global_first array
                                               is owned. */
  size_t              num_calls;        /**< redundant but for convenience;
                                            counts the number of calls of
                                            write and read, respectively */
  sc_MPI_File         file;             /**< file object */
  sc_MPI_Offset       accessed_bytes;   /**< count only array data bytes and
                                           array metadata bytes */
}
sc_file_context_t;

/** This function calculates a padding string consisting of spaces.
 * We require an already allocated array pad or NULL.
 * The number of bytes in pad must be at least divisor + 1!
 * For NULL the function calculates only the number of padding bytes.
 */
static void
sc_file_get_padding_string (size_t num_bytes, size_t divisor, char *pad,
                            char pad_char, size_t * num_pad_bytes)
{
  SC_ASSERT (divisor != 0 && num_pad_bytes != NULL);

  char                padding[SC_FILE_MAX_NUM_PAD_BYTES];
  char               *padding_dyn;
  int                 dynamic;

  *num_pad_bytes = (divisor - (num_bytes % divisor)) % divisor;
  if (*num_pad_bytes == 0 || *num_pad_bytes == 1) {
    /* In these cases there is no space to add new line characters
     * but this is necessary to ensure a consistent layout in a text editor
     */
    *num_pad_bytes += divisor;
  }

  SC_ASSERT (*num_pad_bytes > 1);
  SC_ASSERT (*num_pad_bytes <= divisor);
  if (pad != NULL) {
    if (*num_pad_bytes >= SC_FILE_MAX_NUM_PAD_BYTES) {
      /* number of padding bytes exceeds stack buffer */
      padding_dyn = SC_ALLOC (char, *num_pad_bytes + 1);
      padding_dyn[*num_pad_bytes] = '\n';
      dynamic = 1;
    }
    else {
      padding[SC_FILE_MAX_NUM_PAD_BYTES - 1] = '\n';
      dynamic = 0;
    }

    /* set string to pad */
    memset ((dynamic) ? padding_dyn : padding, pad_char,
            *num_pad_bytes * sizeof (char));

    snprintf (pad, *num_pad_bytes + 1,
              SC_FILE_LINE_FEED_STR "%*.*s" SC_FILE_LINE_FEED_STR,
              (int) *num_pad_bytes - 2, (int) *num_pad_bytes - 2,
              (dynamic) ? padding_dyn : padding);

    if (dynamic) {
      SC_FREE (padding_dyn);
    }
  }
}

sc_file_context_t  *
sc_file_open_write (const char *filename, sc_MPI_Comm mpicomm,
                    const char *user_string, int *errcode)
{
  SC_ASSERT (filename != NULL);
  SC_ASSERT (user_string != NULL);
  SC_ASSERT (errcode != NULL);

  sc_file_context_t  *fc;
  int                 mpiret, mpirank, count;
  char                pad_version[SC_FILE_VERSION_STR_BYTES + 4];
  char                pad_user[SC_FILE_USER_STRING_BYTES + 4];
  char                file_header[SC_FILE_HEADER_BYTES + 1];
  size_t              num_pad_bytes;
  size_t              user_string_len;

  /* check user string */
  user_string_len = strlen (user_string);
  if (!(user_string_len < SC_FILE_USER_STRING_BYTES)) {
    /* invalid user string */
    *errcode = SC_FILE_ERR_IN_DATA;

    /* TODO: error macro */

    return NULL;
  }

  /* allocate file context */
  fc = SC_ALLOC (sc_file_context_t, 1);

  /* Open the file and create a new file if necessary */
  mpiret =
    sc_io_open (mpicomm, filename, SC_IO_WRITE_CREATE, sc_MPI_INFO_NULL,
                &fc->file);
  /* TODO: check return value */

  /* get MPI rank */
  mpiret = sc_MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  if (mpirank == 0) {
    /* get the padding string for the version string */
    sc_file_get_padding_string (strlen (sc_version ()),
                                SC_FILE_VERSION_STR_BYTES + 3, pad_version,
                                SC_FILE_PAD_STRING_CHAR, &num_pad_bytes);
    /* get the padding string for the version string */
    sc_file_get_padding_string (user_string_len,
                                SC_FILE_USER_STRING_BYTES + 3, pad_user,
                                SC_FILE_PAD_STRING_CHAR, &num_pad_bytes);

    /* write padded libsc-defined file header */
    snprintf (file_header, SC_FILE_HEADER_BYTES + 1,
              "%." SC_TOSTRING (SC_FILE_MAGIC_BYTES) "s" SC_FILE_LINE_FEED_STR
              "%." SC_TOSTRING (SC_FILE_VERSION_STR_BYTES) "s%s"
              "%." SC_TOSTRING (SC_FILE_USER_STRING_BYTES) "s%s",
              SC_FILE_MAGIC_NUMBER, sc_version (),
              pad_version, user_string, pad_user);

    mpiret =
      sc_io_write_at (fc->file, 0, file_header,
                      SC_FILE_HEADER_BYTES, sc_MPI_BYTE, &count);
  }

  /* initialize file context */
  fc->mpicomm = mpicomm;
  fc->accessed_bytes = 0;
  fc->num_calls = 0;
  fc->global_first = NULL;
  fc->gf_owned = 0;

  return fc;
}

int
sc_file_close (sc_file_context_t * fc, int *errcode)
{
  SC_ASSERT (fc != NULL);
  SC_ASSERT (errcode != NULL);

  int                 mpiret;

  mpiret = sc_io_close (&fc->file);

  if (fc->gf_owned) {
    SC_FREE (fc->global_first);
  }
  SC_FREE (fc);

  return 0;
}
