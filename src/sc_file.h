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

/** \file sc_file.h
 *
 * Routines for parallel I/O format.
 *
 * \ingroup sc_file
 */

/** \defgroup sc_file Parallel file format
 * 
 * Functionality read and write in parallel using a prescribed
 * file format.
 *
 * \ingroup sc 
 */

#ifndef SC_FILE_H
#define SC_FILE_H

#include <sc.h>

SC_EXTERN_C_BEGIN;

#define SCDAT_MAGIC_NUMBER "scdata0" /**< magic string for libsc data files */
#define SCDAT_HEADER_BYTES 128 /**< number of file header bytes in total incl. padding */
/* the following macros are the number of bytes without the line feed */
#define SCDAT_MAGIC_BYTES 7 /**< number of bytes of the magic number */
#define SCDAT_VERSION_STR_BYTES 53 /**< number of bytes of the version string*/
#define SCDAT_ARRAY_METADATA_BYTES 14 /**< number of array metadata bytes */
/* subtract 2 for '\n' at the beginning and end of the array metadata */
#define SCDAT_ARRAY_METADATA_CHARS (SCDAT_ARRAY_METADATA_BYTES - 2) /**< number of array metadata chars */
#define SCDAT_BYTE_DIV 16 /**< All data blocks are padded to be divisible by this. */
#define SCDAT_MAX_NUM_PAD_BYTES (SCDAT_BYTE_DIV + 1) /**< We enforce to pad in any
                                                               case and the padding string
                                                               needs to contain two
                                                               newline characters and
                                                               therefore this is the
                                                               maximal number of pad
                                                               bytes. */
#define SCDAT_LINE_FEED_STR "\n" /**< line feed as string */
#define SCDAT_PAD_CHAR '=' /**< the padding char as string */
#define SCDAT_PAD_STRING_CHAR '-' /**< the padding char for user strings as string */
#define SCDAT_USER_STRING_BYTES 58 /**< number of user string bytes */
#define SCDAT_SECTION_USER_STRING_BYTES 29 /**< number of section user string bytes */
#define SCDAT_FIELD_HEADER_BYTES (2 + SCDAT_ARRAY_METADATA_BYTES + SCDAT_USER_STRING_BYTES)
                                     /**< number of bytes of one field header */
#define SCDAT_MAX_BLOCK_SIZE 9999999999999 /**< maximal number of block bytes */
#define SCDAT_MAX_FIELD_ENTRY_SIZE 9999999999999 /**< maximal number of bytes per field entry */

/** libsc data file format
 * All libsc data files have a \ref SCDAT_HEADER_BYTES bytes file header at
 * the beginning of the file.
 *
 * File header (\ref SCDAT_HEADER_BYTES bytes):
 * \ref SCDAT_MAGIC_BYTES bytes magic number (scdata0) and one byte \ref SCDAT_LINE_FEED_STR.
 * \ref SCDAT_VERSION_STR_BYTES
 * TODO: continue.
 */

/** Opaque context used for writing a libsc data file. */
typedef struct scdat_fcontext scdat_fcontext_t;

/** Section types in libsc data file. */
typedef enum scdat_section
{
  SCDAT_INLINE, /**< inline data */
  SCDAT_BLOCK, /**< block of given size */
  SCDAT_FIXED, /**< array with a fixed size partition */
  SCDAT_VARIABLE, /**< array with a variable size partition */
  SCDAT_NUM_SECTIONS /**< number of current sections in sc_file */
}
scdat_section_t;

/** Error values for sc_file functions.
 */
/* TODO */
typedef enum sc_file_error
{
  SC_FILE_ERROR_SUCCESS = sc_MPI_ERR_LASTCODE,
  SC_FILE_ERR_FILE, /**< invalid file handle */
  SC_FILE_ERR_NOT_SAME, /**< collective arg not identical */
  SC_FILE_ERR_AMODE, /**< access mode error */
  SC_FILE_ERR_NO_SUCH_FILE, /**< file does not exist */
  SC_FILE_ERR_FILE_EXIST, /**< file exists already */
  SC_FILE_ERR_BAD_FILE, /**< invalid file name */
  SC_FILE_ERR_ACCESS, /**< permission denied */
  SC_FILE_ERR_NO_SPACE, /**< not enough space */
  SC_FILE_ERR_QUOTA, /**< quota exceeded */
  SC_FILE_ERR_READ_ONLY, /**< read only file (system) */
  SC_FILE_ERR_IN_USE, /**< file currently open by other process */
  SC_FILE_ERR_IO, /**< other I/O error */
  SC_FILE_ERR_FORMAT,  /**< read file has a wrong format */
  SC_FILE_ERR_SECTION_TYPE, /**< a valid non-matching section type */
  SC_FILE_ERR_IN_DATA, /**< input data of file function is invalid */
  SC_FILE_ERR_COUNT,   /**< read or write count error that was not
                                 classified as a format error */
  SC_FILE_ERR_UNKNOWN, /**< unknown error */
  SC_FILE_ERR_LASTCODE /**< to define own error codes for
                                  a higher level application
                                  that is using sc_file
                                  functions */
}
sc_file_error_t;

/** Open a file for writing/reading and write/read the file header to the file.
 *
 * This function creates a new file or overwrites an existing one.
 * It is collective and creates the file on a parallel file system.
 * This function leaves the file open if MPI I/O is available.
 * Independent of the availability of MPI I/O the user can write one or more
 * file sections before closing the file using \ref sc_file_close.
 *
 * In the case of writing it is the user's responsibility to write any further
 * metadata of the file that is required by the application. This can be done
 * by writing file sections. However, the user can use \ref
 * sc_fread_section_header and skipping the respective data bytes using the
 * respective read functions scdat_fread_*_data to parse the structure
 * of a given file and some metadata that is written by scdat.
 *
 * In the case of reading  the file must exist and be at least of the size of
 * the file header, i.e. \ref SCDAT_HEADER_BYTES bytes. If the file has a file
 * header that does not satisfy the scdat file header format, the function
 * reports the error using \ref SC_LERRORF, collectively close the file and
 * deallocate the file context. In this case the function returns NULL on all
 * ranks. A wrong file header format causes \ref SCDAT_ERR_FORMAT as \b errcode.
 *
 * This function does not abort on MPI I/O errors but returns NULL.
 * Without MPI I/O the function may abort on file system dependent errors.
 *
 * \param [in]     mpicomm   The MPI communicator that is used to open the
 *                           parallel file.
 * \param [in]     filename  Path to parallel file that is to be created or
 *                           to be opened.
 * \param [in]     mode      Either 'w' for writing to newly created file or
 *                          'r' to read from a file.
 * \param [in,out] user_string For \b mode == 'w' at most \ref
 *                          SCDAT_USER_STRING_BYTES characters in
 *                          a nul-terminated string. These characters are
 *                          written on rank 0 to the file header.
 *                          For \b mode == 'r' at least \ref
 *                          SCDAT_USER_STRING_BYTES + 1 bytes. The user string
 *                          is read on rank 0 and internally broadcasted to all
 *                          ranks.
 * \param [out]    errcode  An errcode that can be interpreted by \ref
 *                          sc_ferror_string.
 * \return                  Newly allocated context to continue writing/reading
 *                          and eventually closing the file. NULL in
 *                          case of error, i.e. errcode != SCDAT_SUCCESS.
 */
scdat_fcontext_t   *scdat_fopen (sc_MPI_Comm mpicomm,
                                 const char *filename,
                                 char mode,
                                 const char *user_string, int *errcode);

/** Write an inline data section.
 *
 * This function writes 32 bytes of user-defined data preceded by a file
 * section header containing a user string. In contrast to other file sections
 * the inline data section does not end with padded data bytes and therefore
 * require exactly 32 bytes data from the user. This enables the user to
 * implement custom file structuring or padding.
 *
 * This function does not abort on MPI I/O errors but returns NULL.
 * Without MPI I/O the function may abort on file system dependent
 * errors. 
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_fopen with mode 'w'.
 * \param [in]      data        On the rank \b root a sc_array with element
 *                              count 1 and element size 32. On all other ranks
 *                              this parameter is ignored.
 * \param [in]      user_string Maximal \ref SCDAT_USER_STRING_BYTES + 1 bytes
 *                              on rank \b root and otherwise ignored.
 *                              The user string is written without the
 *                              nul-termination by MPI rank \b root.
 * \param [in]      root        An integer between 0 and mpisize of the MPI
 *                              communicator that was used to create \b fc.
 *                              \b root indicates the MPI rank on that the
 *                              IO operations take place.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_ferror_string.
 * \return                      Return a pointer to input context or NULL in case
 *                              of errors that does not abort the program.
 *                              In case of error the file is tried to close
 *                              and \b fc is freed.
 *                              The scdat file context can be used to continue
 *                              writing and eventually closing the file.
 */
scdat_fcontext_t   *scdat_fwrite_inline (scdat_fcontext_t * fc,
                                         sc_array_t * data,
                                         const char *user_string, int root,
                                         int *errcode);

/** Write a fixed-size block file section.
 *
 * This function writes a data block of fixed size to the file. The data
 * and its section header is written on the MPI rank \b root.
 * The number of block bytes must be less or equal \ref SCDAT_MAX_BLOCK_SIZE.
 *
 * This function does not abort on MPI I/O errors but returns NULL.
 * Without MPI I/O the function may abort on file system dependent
 * errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_fopen with mode 'w'.
 * \param [in]      block_data  On rank \b root a sc_array with one element and
 *                              element size equals to \b block_size. On all
 *                              other ranks the parameter is ignored.
 * \param [in]      block_size  The size of the data block in bytes.
 * \param [in]      user_string Maximal \ref SCDAT_USER_STRING_BYTES + 1 bytes
 *                              on rank \b root and otherwise ignored.
 *                              The user string is written without the
 *                              nul-termination by MPI rank \b root.
 * \param [in]      root        An integer between 0 and mpisize of the MPI
 *                              communicator that was used to create \b fc.
 *                              \b root indicates the MPI rank on that the
 *                              IO operations take place.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_ferror_string.
 * \return                      Return a pointer to input context or NULL in case
 *                              of errors that does not abort the program.
 *                              In case of error the file is tried to close
 *                              and \b fc is freed.
 *                              The scdat file context can be used to continue
 *                              writing and eventually closing the file.
 */
scdat_fcontext_t   *scdat_fwrite_block (scdat_fcontext_t * fc,
                                        sc_array_t * block_data,
                                        size_t block_size,
                                        const char *user_string,
                                        int *errcode);

/** Write a fixed-size array file section.
 *
 * The fixed-size array is the simplest file section that enables the user to
 * write and read data in parallel. This function writes an array of a given
 * element global count and a fixed element size.
 *
 * This function does not abort on MPI I/O errors but returns NULL.
 * Without MPI I/O the function may abort on file system dependent
 * errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_fopen with mode 'w'.
 * \param [in]      array_data  On rank p the p-th entry of \b elem_counts
 *                              must be the element count of \b array_data.
 *                              The element size of the sc_array must be equal
 *                              to \b elem_size if \b indirect is false. Otherwise,
 *                              \b array_data must be a sc_array of sc_arrays,
 *                              i.e. a sc_array with element size
 *                              sizeof (sc_array_t). Each of the elements of
 *                              \b array_data is then a sc_array with element
 *                              count 1 and element size \b elem_size.
 *                              See also the documentation of the parameter
 *                              \b indirect.
 * \param [in]      elem_counts An sc_array that must be equal on all
 *                              ranks. The element count of \b elem_counts
 *                              must be the mpisize of the MPI communicator
 *                              that was used to create \b fc. The element size
 *                              of the sc_array must be equal to 8.
 *                              The sc_array must contain the local array elements
 *                              counts (unsigned int). That is why it induces
 *                              the partition that is used to write the array
 *                              data in parallel.
 * \param [in]      elem_size   The element size of one array element on number
 *                              of bytes. Must be the same on all ranks.
 * \param [in]      indirect    A Boolean to determine whether \b array_data
 *                              must be a sc_array of sc_arrays to write
 *                              indirectly and in particular from potentially
 *                              non-contigous memory. In the remaining case of
 *                              \b indirect being false \b array_data must be
 *                              a sc_array with element size equals to
 *                              \b elem_size that contains the actual array
 *                              elements.
 * \param [in]      user_string Maximal \ref SCDAT_USER_STRING_BYTES + 1 bytes
 *                              on rank \b root and otherwise ignored.
 *                              The user string is written without the
 *                              nul-termination by MPI rank 0.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_ferror_string.
 * \return                      Return a pointer to input context or NULL in case
 *                              of errors that does not abort the program.
 *                              In case of error the file is tried to close
 *                              and \b fc is freed.
 *                              The scdat file context can be used to continue
 *                              writing and eventually closing the file.
 */
scdat_fcontext_t   *scdat_fwrite_array (scdat_fcontext_t * fc,
                                        sc_array_t * array_data,
                                        sc_array_t * elem_counts,
                                        size_t elem_size,
                                        int indirect,
                                        const char *user_string,
                                        int *errcode);

/** Write a variable-size array file section.
 *
 * This function can be used instead of \ref scdat_fwrite_array if the array
 * elements do not have a constant element size in bytes. For the sake of
 * efficiency this functions never gets to know  about the element sizes for
 * each array element but only about the cumulative byte count per process
 * conforming with the partition of the array elements.
 *
 * This function does not abort on MPI I/O errors but returns NULL.
 * Without MPI I/O the function may abort on file system dependent
 * errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_fopen with mode 'w'.
 * \param [in]      array_data  Let p be the calling rank. If \b indirect is
 *                              false, \b array_data must have element count 1
 *                              and as element size the p-th entry of
 *                              \b elem_sizes. The data of the array must be the
 *                              local array elements according to \b elem_counts
 *                              \b elem_sizes.
 *                              If \b indirect is true, \b array_data must be
 *                              a sc_array with element count equal to the p-th
 *                              array entry of \b elem_counts and element size
 *                              equal to sizeof (sc_array_t). Each array element
 *                              is again a sc_array. Now, with element count 1
 *                              and element size equals to the actual array
 *                              element size that is not checked by this function.
 *                              This function only ensures that the sum of all
 *                              local array element byte counts equals its
 *                              corresponding entries in \b elem_sizes.
 * \param [in]      elem_counts An sc_array that must be equal on all
 *                              ranks. The element count of \b elem_counts
 *                              must be the mpisize of the MPI communicator
 *                              that was used to create \b fc. The element size
 *                              of the sc_array must be equal to 8.
 *                              The sc_array must contain the local array elements
 *                              counts (unsigned int). That is why it induces
 *                              the partition that is used to write the array
 *                              data in parallel.
 * \param [in]      elem_sizes  An sc_array that must be equal on all
 *                              ranks. The element count and element size
 *                              must be the same as for \b elem_counts. The
 *                              array must contain the overall byte count per
 *                              rank conforming with the passed array element
 *                              partition \b elem_counts.
 * \param [in]      indirect    A Boolean to determine whether \b array_data
 *                              must be a sc_array of sc_arrays to write
 *                              indirectly and in particular from potentially
 *                              non-contigous memory. In the remaining case of
 *                              \b indirect being false \b array_data must be
 *                              a sc_array with the acutal array elements as
 *                              data as further explained in the documentation
 *                              of \b array_data.
 * \param [in]      user_string Maximal \ref SCDAT_USER_STRING_BYTES + 1 bytes
 *                              on rank \b root and otherwise ignored.
 *                              The user string is written without the
 *                              nul-termination by MPI rank 0.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_ferror_string.
 * \return                      Return a pointer to input context or NULL in case
 *                              of errors that does not abort the program.
 *                              In case of error the file is tried to close
 *                              and \b fc is freed.
 *                              The scdat file context can be used to continue
 *                              writing and eventually closing the file.
 */
scdat_fcontext_t   *scdat_fwrite_varray (scdat_fcontext_t * fc,
                                         sc_array_t * array_data,
                                         sc_array_t * elem_counts,
                                         sc_array_t * elem_sizes,
                                         int indirect,
                                         const char *user_string,
                                         int *errcode);


sc_file_context_t  *sc_file_read_block (sc_file_context_t * fc,
                                        size_t block_size,
                                        sc_array_t * block_data,
                                        char *user_string, int *errcode);

sc_file_context_t  *sc_file_write_fixed (sc_file_context_t * fc,
                                         size_t element_count,
                                         size_t element_size,
                                         sc_array_t * data_array,
                                         const char *user_string,
                                         int *errcode);

sc_file_context_t  *sc_file_read_fixed (sc_file_context_t * fc,
                                        size_t element_count,
                                        size_t element_size,
                                        sc_array_t * data_array,
                                        char *user_string, int *errcode);

/** Write a variable size array file section.
 *
 * This function writes an array of variable-sized elements in parallel
 * to the opened file. In addition, this function prepends a file section
 * header containing metadata. The file section header is written on MPI rank 0.
 *
 * This function does not abort on MPI I/O errors but returns NULL.
 * Without MPI I/O the function may abort on file system dependent
 * errors.
 *
 * \param [in,out]  fc      File context previously opened by \ref
 *                          sc_file_open_write.
 * \param [in]      sizes   An allocated sc_array that has as element count
 *                          the number of local elements and as element size
 *                          sizeof (uint64_t). 
 * \param [in]      data    An allocated sc_array of sizes->elem_count many
 *                          sc_arrays. For i in 0 ... \b data->elem_count
 *                          the i-th element of \b data, which is
 *                          itself a sc_array, has the element size 1 and
 *                          the element count equals the i-th element of
 *                          \b sizes.
 * \param [in] user_string  Maximal \ref SC_FILE_SECTION_USER_STRING_BYTES + 1
 *                          bytes. The user string is written without the
 *                          nul-termination by MPI rank 0.
 * \param [out]     errcode An errcode that can be interpreted by \ref
 *                          sc_file_error_string.
 * \return                  Return a pointer to input context or NULL in case
 *                          of errors that does not abort the program.
 *                          In case of error the file is tried to close
 *                          and \b fc is freed.
 *                          The file context can be used to continue writing
 *                          and eventually closing the file.
 */
sc_file_context_t  *sc_file_write_variable (sc_file_context_t * fc,
                                            size_t element_count,
                                            sc_array_t * sizes,
                                            sc_array_t * data,
                                            const char *user_string,
                                            int *errcode);

sc_file_context_t  *sc_file_read_variable (sc_file_context_t * fc,
                                           size_t element_count,
                                           sc_array_t * sizes,
                                           sc_array_t * data,
                                           char *user_string, int *errcode);

/** Read a file section of an arbitrary section type.
 *
 * This function reads the next file section without requiring the file section
 * type and the size(s) of the file section.
 *
 * The function reads the file section header to determine the file section
 * type and the sizes.
 * If the user passes NULL for \b alloc_callback, \b data must be unequal NULL.
 * In this case, the function allocates the memory for the user and sets
 * the data pointer of \b data to the allcocated data. This data must be freed
 * using the function \ref sc_file_free.
 * Alternatively, the user can pass NULL for \b data and then must pass an
 * allocation callback \b alloc_callback. Then the allocation callback can
 * give a pointer to allocated memory for each read element. The parameters
 * of \ref sc_file_allocate_t inform the user about the file section type and
 * the size(s). In the case of using \b alloc_callback the user is responsible
 * to free the allocated data.
 *
 * This function does not abort on MPI I/O errors but returns NULL.
 * Without MPI I/O the function may abort on file system dependent
 * errors.
 *
 * \param [in]      fc              File context previously opened by
 *                                  \ref sc_file_open_read.
 * \param [out]     data            If not NULL, \b data is set to newly
 *                                  allocated data that is filled with the read
 *                                  data.
 *                                  Use \b type to get the data layout of
 *                                  \b data.
 * \param [out]     type            The file section type that is read from the
 *                                  file.
 * \param [in]      alloc_callback  An allocation callback to give a pointer
 *                                  to memory for each element that is read from
 *                                  the file.
 * \param [in,out]  user_data       Anonymous user data that is passed to
 *                                  \b alloc_callback.
 * \param [out]     user_string     At least
 *                                  \ref SC_FILE_SECTION_USER_STRING_BYTES + 1
 *                                  bytes. The user string is read on rank 0 and
 *                                  internally broadcasted to all ranks.
 * \param [out]     errcode         An errcode that can be interpreted by \ref
 *                                  sc_file_error_string.
 * \return                          Return a pointer to input context or NULL in
 *                                  case of errors that does not abort the
 *                                  program. In case of error the file is tried
 *                                  to close and \b fc is freed.
 *
 * \note                            This function differs from the
 *                                  sc_file_read_* functions in the sense that
 *                                  it does not expect the user to know the file
 *                                  section type and the size(s).
 */
sc_file_context_t  *sc_file_read (sc_file_context_t * fc,
                                  sc_array_t * data,
                                  sc_file_section_t type,
                                  sc_file_allocate_t alloc_callback,
                                  void *user_data,
                                  const char *user_string, int *errcode);

/* sc_file_info */

/* sc_file_error_string */

/** Close a file opened for parallel write/read and the free the file context.
 *
 * This is a collective function.
 * Every call of \ref sc_fopen must be matched by a corresponding call of \ref
 * sc_fclose on the created file context.
 *
 * This function does not abort on MPI I/O errors but returns NULL.
 * Without MPI I/O the function may abort on file system dependent
 * errors.
 *
 * \param [in,out]  fc        File context previously created by
 *                            \ref sc_fopen. This file context is freed after
 *                            a call of this function.
 * \param [out]     errcode   An errcode that can be interpreted by \ref
 *                            sc_ferror_string.
 * \return                    0 for a successful call and -1 in case a of an
 *                            error. See also \b errcode argument.
 */
int                 sc_fclose (scdat_fcontext_t * fc, int *errcode);

SC_EXTERN_C_END;

#endif /* SC_FILE_H */
