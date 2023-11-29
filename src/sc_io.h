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

/** \file sc_io.h
 *
 * Helper routines for general and parallel I/O.
 *
 * This file provides various sets of functions related to write and read
 * data as well es to encode and decode it to certain formats.
 *
 *  - To abstract transparent writing/reading into/from files or buffers, we
 *    provide functions centered around \ref sc_io_sink_new and \ref
 *    sc_io_source_new.
 *  - To abstract parallel file I/O in a way that works both with and
 *    without MPI I/O support, we provide \ref sc_io_open, \ref sc_io_write
 *    and friends.
 *  - To write to the VTK binary compressed format, we provide suitable
 *    functions to base64 encode and zlib-compress as required; see
 *    \ref sc_vtk_write_binary and \ref sc_vtk_write_compressed.
 *  - To support self-contained ASCII-armored compression, we provide the
 *    functions \ref sc_io_encode, \ref sc_io_decode_info and \ref
 *    sc_io_decode.
 *    They losslessly transform a block of arbitrary data into a compressed
 *    and base64-encoded format and back that is unambiguously defined and
 *    human-friendly.
 *
 * \note For the function \ref sc_io_write_at_all without MPI IO but with MPI
 *       the \b offset argument is ignored. In this case the function writes at
 *       the current end of the file. Hereby, the MPI ranks write in the
 *       rank-induced order. That is why the function may work equivalent to
 *       the MPI IO and non-MPI case but it can not be guaranteed.
 *       Furthermore, it is important to notice that \ref sc_io_write_at and
 *       \ref sc_io_read_at are only valid to call with zcount > 0 on rank 0,
 *       if MPI IO is not available.
 *
 * \ingroup io
 */

#ifndef SC_IO_H
#define SC_IO_H

#include <sc_containers.h>

/** Examine the MPI return value and print an error if there is one.
 * The message passed is appended to MPI, file and line information.
 */
#define SC_CHECK_MPI_VERBOSE(errcode,user_msg) do {            \
  char sc_msg[sc_MPI_MAX_ERROR_STRING];                        \
  int sc_msglen;                                               \
  if ((errcode) != sc_MPI_SUCCESS) {                           \
    sc_MPI_Error_string (errcode, sc_msg, &sc_msglen);         \
    SC_LERRORF ("%s at %s:%d: %s\n",                           \
                (user_msg), __FILE__, __LINE__, sc_msg);       \
  }} while (0)

SC_EXTERN_C_BEGIN;

/** Error values for io.
 */
typedef enum
{
  SC_IO_ERROR_NONE,     /**< The value of zero means no error. */
  SC_IO_ERROR_FATAL = -1,       /**< The io object is now dysfunctional. */
  SC_IO_ERROR_AGAIN = -2        /**< Another io operation may resolve it.
                                The function just returned was a noop. */
}
sc_io_error_t;

/** The I/O mode for writing using \ref sc_io_sink. */
typedef enum
{
  SC_IO_MODE_WRITE,     /**< Semantics as "w" in fopen. */
  SC_IO_MODE_APPEND,    /**< Semantics as "a" in fopen. */
  SC_IO_MODE_LAST       /**< Invalid entry to close list */
}
sc_io_mode_t;

/** Enum to specify encoding for \ref sc_io_sink and \ref sc_io_source. */
typedef enum
{
  SC_IO_ENCODE_NONE,    /**< No encoding */
  SC_IO_ENCODE_LAST     /**< Invalid entry to close list */
}
sc_io_encode_t;

/** The type of I/O operation \ref sc_io_sink and \ref sc_io_source. */
typedef enum
{
  SC_IO_TYPE_BUFFER,    /**< Write to a buffer */
  SC_IO_TYPE_FILENAME,  /**< Write to a file to be opened */
  SC_IO_TYPE_FILEFILE,  /**< Write to an already opened file */
  SC_IO_TYPE_LAST       /**< Invalid entry to close list */
}
sc_io_type_t;

/** A generic data sink. */
typedef struct sc_io_sink
{
  sc_io_type_t        iotype;          /**< type of the I/O operation */
  sc_io_mode_t        mode;            /**< write semantics */
  sc_io_encode_t      encode;          /**< encoding of data */
  sc_array_t         *buffer;          /**< buffer for the iotype
                                            SC_IO_TYPE_BUFFER*/
  size_t              buffer_bytes;    /**< distinguish from array elems */
  FILE               *file;            /**< file pointer for iotype unequal to
                                            SC_IO_TYPE_BUFFER */
  size_t              bytes_in;        /**< input bytes count */
  size_t              bytes_out;       /**< written bytes count */
}
sc_io_sink_t;

/** A generic data source. */
typedef struct sc_io_source
{
  sc_io_type_t        iotype;          /**< type of the I/O operation */
  sc_io_encode_t      encode;          /**< encoding of data */
  sc_array_t         *buffer;          /**< buffer for the iotype
                                            SC_IO_TYPE_BUFFER*/
  size_t              buffer_bytes;    /**< distinguish from array elems */
  FILE               *file;            /**< file pointer for iotype unequal to
                                            SC_IO_TYPE_BUFFER */
  size_t              bytes_in;        /**< input bytes count */
  size_t              bytes_out;       /**< read bytes count */
  sc_io_sink_t       *mirror;          /**< if activated, a sink to store the
                                            data*/
  sc_array_t         *mirror_buffer;   /**< if activated, the buffer for the
                                            mirror */
}
sc_io_source_t;

/** Open modes for \ref sc_io_open */
typedef enum
{
  SC_IO_READ,                         /**< open a file in read-only mode */
  SC_IO_WRITE_CREATE,                 /**< open a file in write-only mode;
                                           if the file exists, the file will
                                           be truncated to length zero and
                                           then overwritten */
  SC_IO_WRITE_APPEND                  /**< append to an already existing file */
}
sc_io_open_mode_t;

/** Create a generic data sink.
 * \param [in] iotype           Type must be a value from \ref sc_io_type_t.
 *                              Depending on iotype, varargs must follow:
 *                              BUFFER: sc_array_t * (existing array).
 *                              FILENAME: const char * (name of file to open).
 *                              FILEFILE: FILE * (file open for writing).
 *                              These buffers are only borrowed by the sink.
 * \param [in] iomode           Mode must be a value from \ref sc_io_mode_t.
 *                              For type FILEFILE, data is always appended.
 * \param [in] ioencode         Must be a value from \ref sc_io_encode_t.
 * \return                      Newly allocated sink, or NULL on error.
 */
sc_io_sink_t       *sc_io_sink_new (int iotype, int iomode,
                                    int ioencode, ...);

/** Free data sink.
 * Calls sc_io_sink_complete and discards the final counts.
 * Errors from complete lead to SC_IO_ERROR_FATAL returned from this function.
 * Call sc_io_sink_complete yourself if bytes_out is of interest.
 * \param [in,out] sink         The sink object to complete and free.
 * \return                      0 on success, nonzero on error.
 */
int                 sc_io_sink_destroy (sc_io_sink_t * sink);

/** Write data to a sink.  Data may be buffered and sunk in a later call.
 * The internal counters sink->bytes_in and sink->bytes_out are updated.
 * \param [in,out] sink         The sink object to write to.
 * \param [in] data             Data passed into sink.
 * \param [in] bytes_avail      Number of data bytes passed in.
 * \return                      0 on success, nonzero on error.
 */
int                 sc_io_sink_write (sc_io_sink_t * sink,
                                      const void *data, size_t bytes_avail);

/** Flush all buffered output data to sink.
 * This function may return SC_IO_ERROR_AGAIN if another write is required.
 * Currently this may happen if BUFFER requires an integer multiple of bytes.
 * If successful, the updated value of bytes read and written is returned
 * in bytes_in/out, and the sink status is reset as if the sink had just
 * been created.  In particular, the bytes counters are reset to zero.
 * The internal state of the sink is not changed otherwise.
 * It is legal to continue writing to the sink hereafter.
 * The sink actions taken depend on its type.
 * BUFFER, FILEFILE: none.
 * FILENAME: call fclose on sink->file.
 * \param [in,out] sink         The sink object to write to.
 * \param [in,out] bytes_in     Bytes received since the last new or complete
 *                              call.  May be NULL.
 * \param [in,out] bytes_out    Bytes written since the last new or complete
 *                              call.  May be NULL.
 * \return                      0 if completed, nonzero on error.
 */
int                 sc_io_sink_complete (sc_io_sink_t * sink,
                                         size_t *bytes_in, size_t *bytes_out);

/** Align sink to a byte boundary by writing zeros.
 * \param [in,out] sink         The sink object to align.
 * \param [in] bytes_align      Byte boundary.
 * \return                      0 on success, nonzero on error.
 */
int                 sc_io_sink_align (sc_io_sink_t * sink,
                                      size_t bytes_align);

/** Create a generic data source.
 * \param [in] iotype           Type must be a value from \ref sc_io_type_t.
 *                              Depending on iotype, varargs must follow:
 *                              BUFFER: sc_array_t * (existing array).
 *                              FILENAME: const char * (name of file to open).
 *                              FILEFILE: FILE * (file open for reading).
 * \param [in] ioencode         Encoding value from \ref sc_io_encode_t.
 * \return                      Newly allocated source, or NULL on error.
 */
sc_io_source_t     *sc_io_source_new (int iotype, int ioencode, ...);

/** Free data source.
 * Calls sc_io_source_complete and requires it to return no error.
 * This is to avoid discarding buffered data that has not been passed to read.
 * \param [in,out] source       The source object to free.
 * \return                      0 on success.  Nonzero if an error is
 *                              encountered or is_complete returns one.
 */
int                 sc_io_source_destroy (sc_io_source_t * source);

/** Read data from a source.
 * The internal counters source->bytes_in and source->bytes_out are updated.
 * Data is read until the data buffer has not enough room anymore, or source
 * becomes empty.  It is possible that data already read internally remains
 * in the source object for the next call.  Call sc_io_source_complete and
 * check its return value to find out.
 * Returns an error if bytes_out is NULL and less than bytes_avail are read.
 * \param [in,out] source       The source object to read from.
 * \param [in] data             Data buffer for reading from sink.
 *                              If NULL the output data will be thrown away.
 * \param [in] bytes_avail      Number of bytes available in data buffer.
 * \param [in,out] bytes_out    If not NULL, byte count read into data buffer.
 *                              Otherwise, requires to read exactly bytes_avail.
 * \return                      0 on success, nonzero on error.
 */
int                 sc_io_source_read (sc_io_source_t * source,
                                       void *data, size_t bytes_avail,
                                       size_t *bytes_out);

/** Determine whether all data buffered from source has been returned by read.
 * If it returns SC_IO_ERROR_AGAIN, another sc_io_source_read is required.
 * If the call returns no error, the internal counters source->bytes_in and
 * source->bytes_out are returned to the caller if requested, and reset to 0.
 * The internal state of the source is not changed otherwise.
 * It is legal to continue reading from the source hereafter.
 *
 * \param [in,out] source       The source object to read from.
 * \param [in,out] bytes_in     If not NULL and true is returned,
 *                              the total size of the data sourced.
 * \param [in,out] bytes_out    If not NULL and true is returned,
 *                              total bytes passed out by source_read.
 * \return                      SC_IO_ERROR_AGAIN if buffered data remaining.
 *                              Otherwise return ERROR_NONE and reset counters.
 */
int                 sc_io_source_complete (sc_io_source_t * source,
                                           size_t *bytes_in,
                                           size_t *bytes_out);

/** Align source to a byte boundary by skipping.
 * \param [in,out] source       The source object to align.
 * \param [in] bytes_align      Byte boundary.
 * \return                      0 on success, nonzero on error.
 */
int                 sc_io_source_align (sc_io_source_t * source,
                                        size_t bytes_align);

/** Activate a buffer that mirrors (i.e., stores) the data that was read.
 * \param [in,out] source       The source object to activate mirror in.
 * \return                      0 on success, nonzero on error.
 */
int                 sc_io_source_activate_mirror (sc_io_source_t * source);

/** Read data from the source's mirror.
 * Same behaviour as sc_io_source_read.
 * \param [in,out] source       The source object to read mirror data from.
 * \param [in] data             Data buffer for reading from source's mirror.
 *                              If NULL the output data will be thrown away.
 * \param [in] bytes_avail      Number of bytes available in data buffer.
 * \param [in,out] bytes_out    If not NULL, byte count read into data buffer.
 *                              Otherwise, requires to read exactly bytes_avail.
 * \return                      0 on success, nonzero on error.
 */
int                 sc_io_source_read_mirror (sc_io_source_t * source,
                                              void *data,
                                              size_t bytes_avail,
                                              size_t *bytes_out);

/** Encode a block of arbitrary data with the default sc_io format.
 * The corresponding decoder function is \ref sc_io_decode.
 * This function cannot crash unless out of memory.
 *
 * Currently this function calls \ref sc_io_encode_zlib with
 * compression level Z_BEST_COMPRESSION (subject to change).
 * Without zlib configured that function works uncompressed.
 *
 * The encoding method and input data size can be retrieved, optionally,
 * from the encoded data by \ref sc_io_decode_info.  This function decodes
 * the method as a character, which is 'z' for \ref sc_io_encode_zlib.
 * We reserve the characters A-C, d-z indefinitely.
 *
 * \param [in,out] data     If \a out is NULL, we work in place.
 *                          In this case, the array must on input have
 *                          an element size of 1 byte, which is preserved.
 *                          After reading all data from this array, it assumes
 *                          the identity of the \a out argument below.
 *                          Otherwise, this is a read-only argument
 *                          that may have arbitrary element size.
 *                          On input, all data in the array is used.
 * \param [in,out] out      If not NULL, a valid array of element size 1.
 *                          It must be resizable (not a view).
 *                          We resize the array to the output data, which
 *                          always includes a final terminating zero.
 */
void                sc_io_encode (sc_array_t *data, sc_array_t *out);

/** Encode a block of arbitrary data, compressed, into an ASCII string.
 * This is a two-stage process: zlib compress and then encode to base 64.
 * The output is a NUL-terminated string of printable characters.
 *
 * We first compress the data into the zlib deflate format (RFC 1951).
 * The compressor must use no preset dictionary (this is the default).
 * If zlib is detected on configuration, we compress with the given level.
 * If zlib is not detected, we write data equivalent to Z_NO_COMPRESSION.
 * The status of zlib detection can be queried at compile time using
 * \#ifdef SC_HAVE_ZLIB or at run time using \ref sc_have_zlib.
 * Both types of result are readable by a standard zlib uncompress call.
 *
 * Secondly, we process the input data size as an 8-byte big-endian number,
 * then the letter 'z', and then the zlib compressed data, concatenated,
 * with a base 64 encoder.  We break lines after 76 code characters.
 * Each line break consists of two configurable but arbitrary bytes.
 * The line breaks are considered part of the output data specification.
 * The last line is terminated with the same line break and then a NUL.
 *
 * This routine can work in place or write to an output array.
 * The corresponding decoder function is \ref sc_io_decode.
 * This function cannot crash unless out of memory.
 *
 * \param [in,out] data     If \a out is NULL, we work in place.
 *                          In this case, the array must on input have
 *                          an element size of 1 byte, which is preserved.
 *                          After reading all data from this array, it assumes
 *                          the identity of the \a out argument below.
 *                          Otherwise, this is a read-only argument
 *                          that may have arbitrary element size.
 *                          On input, all data in the array is used.
 * \param [in,out] out      If not NULL, a valid array of element size 1.
 *                          It must be resizable (not a view).
 *                          We resize the array to the output data, which
 *                          always includes a final terminating zero.
 * \param [in] zlib_compression_level     Compression level between 0
 *                          (no compression) and 9 (best compression).
 *                          The value -1 indicates some default level.
 * \param [in] line_break_character       This character is arbitrary
 *                          and specifies the first of two line break
 *                          bytes.  The second byte is always '\n'.
 */
void                sc_io_encode_zlib (sc_array_t *data, sc_array_t *out,
                                       int zlib_compression_level,
                                       int line_break_character);

/** Decode length and format of original input from encoded data.
 * We expect at least 12 bytes of the format produced by \ref sc_io_encode.
 * No matter how much data has been encoded by it, this much is available.
 * We decode the original data size and the character indicating the format.
 *
 * This function does not require zlib.  It works with any well-defined data.
 *
 * Note that this function is not required before \ref sc_io_decode.
 * Calling this function on any result produced by \ref sc_io_encode
 * will succeed and report a legal format.  This function cannot crash.
 *
 * \param [in] data     This must be an array with element size 1.
 *                      If it contains less than 12 code bytes we error out.
 *                      It its first 12 bytes do not base 64 decode to 9 bytes
 *                      we error out.  We generally ignore the remaining data.
 * \param [out] original_size   If not NULL and we do not error out,
 *                      set to the original size as encoded in the data.
 * \param [out] format_char     If not NULL and we do not error out, the
 *                      ninth character of decoded data indicating the format.
 * \param [in,out] re   Provided for error reporting, presently must be NULL.
 * \return              0 on success, negative value on error.
 */
int                 sc_io_decode_info (sc_array_t *data,
                                       size_t *original_size,
                                       char *format_char, void *re);

/** Decode a block of base 64 encoded compressed data.
 * The base 64 data must contain two arbitrary bytes after every 76 code
 * characters and also at the end of the last line if it is short,
 * and then a final NUL character.
 * This function does not require zlib but benefits for speed.
 *
 * This is a two-stage process: we decode the input from base 64 first.
 * Then we extract the 8-byte big-endian original data size, the character
 * 'z', and execute a zlib decompression on the remaining decoded data.
 * This function detects malformed input by erroring out.
 *
 * If we should add another format in the future, the format character
 * may be something else than 'z', as permitted by our specification.
 * To this end, we reserve the characters A-C and d-z indefinitely.
 *
 * Any error condition is indicated by a negative return value.
 * Possible causes for error are:
 *
 *  - the input data string is not NUL-terminated
 *  - the first 12 characters of input do not decode properly
 *  - the input data is corrupt for decoding or decompression
 *  - the output data array has non-unit element size and the
 *    length of the output data is not divisible by the size
 *  - the output data would exceed the specified threshold
 *  - the output array is a view of insufficient length
 *
 * We also error out if the data requires a compression dictionary,
 * which would be a violation of above encode format specification.
 *
 * The corresponding encode function is \ref sc_io_encode.
 * When passing an array as output, we resize it properly.
 * This function cannot crash unless out of memory.
 *
 * \param [in,out] data     If \a out is NULL, we work in place.
 *                          In that case, output is written into
 *                          this array after a suitable resize.
 *                          Either way, we expect a NUL-terminated
 *                          base 64 encoded string on input that has
 *                          in turn been obtained by zlib compression.
 *                          It must be in the exact format produced by
 *                          \ref sc_io_encode; please see documentation.
 *                          The element size of the input array must be 1.
 * \param [in,out] out      If not NULL, a valid array (may be a view).
 *                          If NULL, the input array becomes the output.
 *                          If the output array is a view and the output
 *                          data larger than its view size, we error out.
 *                          We expect commensurable element and data size
 *                          and resize the output to fit exactly, which
 *                          restores the original input passed to encoding.
 *                          An output view array of matching size may be
 *                          constructed using \ref sc_io_decode_info.
 * \param [in] max_original_size    If nonzero, this is the maximal data
 *                          size that we will accept after uncompression.
 *                          If exceeded, return a negative value.
 * \param [in,out] re   Provided for error reporting, presently must be NULL.
 * \return                  0 on success, negative on malformed input
 *                          data or insufficient output space.
 */
int                 sc_io_decode (sc_array_t *data, sc_array_t *out,
                                  size_t max_original_size, void *re);

/** This function writes numeric binary data in VTK base64 encoding.
 * \param vtkfile        Stream opened for writing.
 * \param numeric_data   A pointer to a numeric data array.
 * \param byte_length    The length of the data array in bytes.
 * \return               Returns 0 on success, -1 on file error.
 */
int                 sc_vtk_write_binary (FILE * vtkfile, char *numeric_data,
                                         size_t byte_length);

/** This function writes numeric binary data in VTK compressed format.
 * \param vtkfile        Stream opened for writing.
 * \param numeric_data   A pointer to a numeric data array.
 * \param byte_length    The length of the data array in bytes.
 * \return               Returns 0 on success, -1 on file error.
 */
int                 sc_vtk_write_compressed (FILE * vtkfile,
                                             char *numeric_data,
                                             size_t byte_length);

/** Wrapper for fopen(3).
 * We provide an additional argument that contains the error message.
 */
FILE               *sc_fopen (const char *filename, const char *mode,
                              const char *errmsg);

/** Write memory content to a file.
 * \param [in] ptr      Data array to write to disk.
 * \param [in] size     Size of one array member.
 * \param [in] nmemb    Number of array members.
 * \param [in,out] file File pointer, must be opened for writing.
 * \param [in] errmsg   Error message passed to SC_CHECK_ABORT.
 * \note                This function aborts on file errors.
 */
void                sc_fwrite (const void *ptr, size_t size,
                               size_t nmemb, FILE * file, const char *errmsg);

/** Read file content into memory.
 * \param [out] ptr     Data array to read from disk.
 * \param [in] size     Size of one array member.
 * \param [in] nmemb    Number of array members.
 * \param [in,out] file File pointer, must be opened for reading.
 * \param [in] errmsg   Error message passed to SC_CHECK_ABORT.
 * \note                This function aborts on file errors.
 */
void                sc_fread (void *ptr, size_t size,
                              size_t nmemb, FILE * file, const char *errmsg);

/** Best effort to flush a file's data to disc and close it.
 * \param [in,out] file         File open for writing.
 */
void                sc_fflush_fsync_fclose (FILE * file);

/** Opens a MPI file or without MPI I/O or even without MPI a file context.
 * \param[in] mpicomm   MPI communicator
 * \param[in] filename  The path to the file that we want to open.
 * \param[in] amode     An access mode.
 * \param[in] mpiinfo   The MPI info
 * \param[out] mpifile  The MPI file that is opened. This can be a
 *                      an actual MPI IO file or an internal file
 *                      conntext to preserve some MPI IO functionalities
 *                      without MPI IO and to have working code without
 *                      MPI at all. This output variable is only filled if the
 *                      return value of the function is \ref sc_MPI_SUCCESS.
 * \return              A sc_MPI_ERR_* as defined in \ref sc_mpi.h.
 *                      The error code can be passed to
 *                      \ref sc_MPI_Error_string. If the return value is
 *                      not \ref sc_MPI_SUCCESS, \b mpifile is not filled.
 * \note                This function does not exactly follow the MPI_File
 *                      semantic in the sense that it truncates files to the
 *                      length zero before overwriting them.
 */
int                 sc_io_open (sc_MPI_Comm mpicomm,
                                const char *filename, sc_io_open_mode_t amode,
                                sc_MPI_Info mpiinfo, sc_MPI_File * mpifile);

#define sc_mpi_read         sc_io_read   /**< For backwards compatibility. */

/** Read MPI file content into memory.
 * \param [in,out] mpifile      MPI file object opened for reading.
 * \param [out] ptr     Data array to read in from disk.
 * \param [in] zcount   Number of array members.
 * \param [in] t        The MPI type for each array member.
 * \param [in] errmsg   Error message passed to SC_CHECK_ABORT.
 * \note                This function aborts on MPI file and count errors.
 *                      This function does not use the calling convention
 *                      and error handling as the other sc_io MPI file
 *                      functions to ensure backwards compatibility.
 * \note                This function aborts if MPI I/O is not enabled.
 */
void                sc_io_read (sc_MPI_File mpifile, void *ptr,
                                size_t zcount, sc_MPI_Datatype t,
                                const char *errmsg);

/** Read MPI file content into memory for an explicit offset.
 * This function does not update the file pointer of the MPI file.
 * Contrary to \ref sc_io_read, it does not abort on read errors.
 * \param [in,out] mpifile      MPI file object opened for reading.
 * \param [in] offset   Starting offset in counts of the type \b t.
 * \param [in] ptr      Data array to read from disk.
 * \param [in] zcount   Number of array members.
 * \param [in] t        The MPI type for each array member.
 * \param [out] ocount  The number of read elements of type \b t.
 * \return              A sc_MPI_ERR_* as defined in \ref sc_mpi.h.
 *                      The error code can be passed to
 *                      \ref sc_MPI_Error_string.
 * \note                If MPI I/O is not available this function has restricted
 *                      functionality in the sense that for \b zcount > 0, this
 *                      function is only legal to call on rank 0. On all other
 *                      ranks \b zcount must be 0. If this requirement is
 *                      violated this function returns \ref sc_MPI_ERR_ARG.
 */
int                 sc_io_read_at (sc_MPI_File mpifile,
                                   sc_MPI_Offset offset, void *ptr,
                                   int zcount, sc_MPI_Datatype t,
                                   int *ocount);

/** Read MPI file content collectively into memory for an explicit offset.
 * This function does not update the file pointer of the MPI file.
 * \param [in,out] mpifile      MPI file object opened for reading.
 * \param [in] offset   Starting offset in counts of the type \b t.
 * \param [in] ptr      Data array to read from disk.
 * \param [in] zcount   Number of array members.
 * \param [in] t        The MPI type for each array member.
 * \param [out] ocount  The number of read elements of type \b t.
 * \return              A sc_MPI_ERR_* as defined in \ref sc_mpi.h.
 *                      The error code can be passed to
 *                      \ref sc_MPI_Error_string.
 */
int                 sc_io_read_at_all (sc_MPI_File mpifile,
                                       sc_MPI_Offset offset, void *ptr,
                                       int zcount, sc_MPI_Datatype t,
                                       int *ocount);

/** Read memory content collectively from an MPI file.
 * A call of this function is equivalent to call \ref sc_io_read_at_all
 * with offset = 0 but the call of this function is not equivalent
 * to a call of MPI_File_read_all since this function ignores the current
 * position of the file cursor.
 * \param [in,out] mpifile      MPI file object opened for reading.
 * \param [in] ptr      Data array to read from disk.
 * \param [in] zcount   Number of array members.
 * \param [in] t        The MPI type for each array member.
 * \param [out] ocount  The number of read elements of type \b t.
 * \return              A sc_MPI_ERR_* as defined in \ref sc_mpi.h.
 *                      The error code can be passed to
 *                      \ref sc_MPI_Error_string.
 */
int                 sc_io_read_all (sc_MPI_File mpifile, void *ptr,
                                    int zcount, sc_MPI_Datatype t,
                                    int *ocount);

#define sc_mpi_write        sc_io_write  /**< For backwards compatibility. */

/** Write memory content to an MPI file.
 * \param [in,out] mpifile      MPI file object opened for writing.
 * \param [in] ptr      Data array to write to disk.
 * \param [in] zcount   Number of array members.
 * \param [in] t        The MPI type for each array member.
 * \param [in] errmsg   Error message passed to SC_CHECK_ABORT.
 * \note                This function aborts on MPI file and count errors.
 *                      This function does not use the calling convention
 *                      and error handling as the other sc_io MPI file
 *                      functions to ensure backwards compatibility.
 * \note                This function aborts if MPI I/O is not enabled.
 */
void                sc_io_write (sc_MPI_File mpifile, const void *ptr,
                                 size_t zcount, sc_MPI_Datatype t,
                                 const char *errmsg);

/** Write MPI file content into memory for an explicit offset.
 * This function does not update the file pointer that is part of mpifile.
 * Contrary to \ref sc_io_write, it does not abort on read errors. 
 * \param [in,out] mpifile      MPI file object opened for reading.
 * \param [in] offset   Starting offset in etype, where the etype is given by
 *                      the type t.
 * \param [in] ptr      Data array to write to disk.
 * \param [in] zcount   Number of array members.
 * \param [in] t        The MPI type for each array member.
 * \param [out] ocount  The number of written elements of type \b t.
 * \return              A sc_MPI_ERR_* as defined in \ref sc_mpi.h.
 *                      The error code can be passed to
 *                      \ref sc_MPI_Error_string.
 * \note                If MPI I/O is not available this function has restricted
 *                      functionality in the sense that for \b zcount > 0, this
 *                      function is only legal to call on rank 0. On all other
 *                      ranks \b zcount must be 0. If this requirement is
 *                      violated this function returns \ref sc_MPI_ERR_ARG.
 */
int                 sc_io_write_at (sc_MPI_File mpifile,
                                    sc_MPI_Offset offset,
                                    const void *ptr, int zcount,
                                    sc_MPI_Datatype t, int *ocount);

/** Write MPI file content collectively into memory for an explicit offset.
 * This function does not update the file pointer that is part of mpifile.
 *
 * \note  If there is no MPI IO but MPI available, the offset parameter is
 *        ignored and the ranks just write at the current end of the file
 *        according to their rank-induced order.
 * \param [in,out] mpifile      MPI file object opened for reading.
 * \param [in] offset   Starting offset in etype, where the etype is given by
 *                      the type t. This parameter is ignored in the case of
 *                      having MPI but no MPI IO. In this case this function
 *                      writes to the current end of the file as described
 *                      above.
 * \param [in] ptr      Data array to write to disk.
 * \param [in] zcount   Number of array members.
 * \param [in] t        The MPI type for each array member.
 * \param [out] ocount  The number of written elements of type \b t.
 * \return              A sc_MPI_ERR_* as defined in \ref sc_mpi.h.
 *                      The error code can be passed to
 *                      \ref sc_MPI_Error_string.
 */
int                 sc_io_write_at_all (sc_MPI_File mpifile,
                                        sc_MPI_Offset offset,
                                        const void *ptr, size_t zcount,
                                        sc_MPI_Datatype t, int *ocount);

/** Write memory content collectively to an MPI file.
 * A call of this function is equivalent to call \ref sc_io_write_at_all
 * with offset = 0 but the call of this function is not equivalent
 * to a call of MPI_File_write_all since this function ignores the current
 * position of the file cursor.
 * \param [in,out] mpifile      MPI file object opened for writing.
 * \param [in] ptr      Data array to write to disk.
 * \param [in] zcount   Number of array members.
 * \param [in] t        The MPI type for each array member.
 * \param [out] ocount  The number of written elements of type \b t.
 * \return              A sc_MPI_ERR_* as defined in \ref sc_mpi.h.
 *                      The error code can be passed to
 *                      \ref sc_MPI_Error_string.
 */
int                 sc_io_write_all (sc_MPI_File mpifile,
                                     const void *ptr, size_t zcount,
                                     sc_MPI_Datatype t, int *ocount);

/** Close collectively a sc_MPI_File.
 * \param[in] file  MPI file object that is closed.
 * \return              A sc_MPI_ERR_* as defined in \ref sc_mpi.h.
 *                      The error code can be passed to
 *                      \ref sc_MPI_Error_string.
 *
 */
int                 sc_io_close (sc_MPI_File * file);

SC_EXTERN_C_END;

#endif /* SC_IO_H */
