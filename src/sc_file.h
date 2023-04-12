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

#define SC_FILE_MAGIC_NUMBER "scdata0" /**< magic string for libsc data files */
#define SC_FILE_HEADER_BYTES 128 /**< number of file header bytes in total incl. padding */
/* the following macros are the number of bytes without the line feed */
#define SC_FILE_MAGIC_BYTES 7 /**< number of bytes of the magic number */
#define SC_FILE_VERSION_STR_BYTES 53 /**< number of bytes of the version string*/
#define SC_FILE_ARRAY_METADATA_BYTES 14 /**< number of array metadata bytes */
/* subtract 2 for '\n' at the beginning and end of the array metadata */
#define SC_FILE_ARRAY_METADATA_CHARS (SC_FILE_ARRAY_METADATA_BYTES - 2) /**< number of array metadata chars */
#define SC_FILE_BYTE_DIV 16 /**< All data blocks are padded to be divisible by this. */
#define SC_FILE_MAX_NUM_PAD_BYTES (SC_FILE_BYTE_DIV + 1) /**< We enforce to pad in any
                                                               case and the padding string
                                                               needs to contain two
                                                               newline characters and
                                                               therefore this is the
                                                               maximal number of pad
                                                               bytes. */
#define SC_FILE_LINE_FEED_STR "\n" /**< line feed as string */
#define SC_FILE_PAD_CHAR '=' /**< the padding char as string */
#define SC_FILE_PAD_STRING_CHAR '-' /**< the padding char for user strings as string */
#define SC_FILE_USER_STRING_BYTES 61 /**< number of user string bytes */
#define SC_FILE_SECTION_USER_STRING_BYTES 29 /**< number of section user string bytes */
#define SC_FILE_FIELD_HEADER_BYTES (2 + SC_FILE_ARRAY_METADATA_BYTES + SC_FILE_USER_STRING_BYTES)
                                     /**< number of bytes of one field header */
#define SC_FILE_MAX_BLOCK_SIZE 9999999999999 /**< maximal number of block bytes */
#define SC_FILE_MAX_FIELD_ENTRY_SIZE 9999999999999 /**< maximal number of bytes per field entry */

/** libsc data file format
 * All libsc data files have a \ref SC_FILE_HEADER_BYTES bytes file header at
 * the beginning of the file.
 *
 * File header (\ref SC_FILE_HEADER_BYTES bytes):
 * \ref SC_FILE_MAGIC_BYTES bytes magic number (scdata0) and one byte \ref SC_FILE_LINE_FEED_STR.
 * \ref SC_FILE_VERSION_STR_BYTES 
 * TODO: continue.
 */

/** Opaque context used for writing a libsc data file. */
typedef struct sc_file_context sc_file_context_t;

/** Section types in libsc data file. */
typedef enum sc_file_section
{
  SC_FILE_INLINE, /**< inline data */
  SC_FILE_BLOCK, /**< block of given size */
  SC_FILE_FIXED, /**< array with a fixed size partition */
  SC_FILE_VARIABLE, /**< array with a variable size partition */
  SC_FILE_NUM_SECTIONS /**< number of current sections in sc_file */
}
sc_file_section_t;

/** An allocation callback to read data from a file using \ref sc_file_read.
 *
 * \param [in]  element_index     The index of the current element.
 * \param [in]  element_count     The number of elements that are read from the
 *                                file.
 * \param [in]  element_size      The size of the current element in number of
 *                                bytes.
 * \param [in]  sizes             An array of the sizes in number of bytes for
 *                                each element. \b sizes->elem_count equals
 *                                \b element_count.
 * \param [in]  type              The file section type.
 * \param [in,out] user_data      User data that the user passed to \ref
 *                                sc_file_read and that is passed onto the
 *                                allocation callback
 * \return                        A pointer to at least \b element_size
 *                                many allocated bytes.
 */
typedef void        (*sc_file_allocate_t) (size_t element_index,
                                           size_t element_count,
                                           size_t element_size,
                                           sc_array_t * sizes,
                                           sc_file_section_t type,
                                           void *user_data);

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

/** Open a file for writing and write the file header to the file.
 *
 * This function creates a new file or overwrites an existing one.
 * It is collective and creates the file on a parallel file system.
 * This function leaves the file open if MPI I/O is available.
 * Independent of the availability of MPI I/O the user can write one or more
 * file sections before closing the file using \ref sc_file_close.
 *
 * It is the user's responsibility to write any further metadata of the file
 * that is required by the application. This can be done by writing file
 * sections. However, the user can use \ref sc_file_info to parse the structure
 * of a given file and some metadata that is written by sc_file.
 * In addition, the user can read file sections without knowing their type
 * and data size(s) using \ref sc_file_read.
 *
 * This function does not abort on MPI I/O errors but returns NULL.
 * Without MPI I/O the function may abort on file system dependent errors.
 *
 * \param [in]  mpicomm     The MPI communicator that is used to open the
 *                          parallel file.
 * \param [in]  filename    Path to parallel file that is to be created.
 * \param [in]  user_string At most \ref SC_FILE_USER_STRING_BYTES characters in
 *                          a nul-terminated string. These characters are
 *                          written to the file header.
 * \param [out] errcode     An errcode that can be interpreted by \ref
 *                          sc_file_error_string.
 * \return                  Newly allocated context to continue writing
 *                          and eventually closing the file. NULL in
 *                          case of error, i.e. errcode != SC_FILE_SUCCESS.
 */
sc_file_context_t  *sc_file_open_write (sc_MPI_Comm mpicomm,
                                        const char *filename,
                                        const char *user_string,
                                        int *errcode);

/** Open a file for reading and read its file header on rank 0.
 *
 * This function is a collective function.
 * The read user string is broadcasted to all ranks.
 * The file must exist and be at least of the size of the file header, i.e.
 * \ref SC_FILE_HEADER_BYTES bytes.
 *
 * If the file has a file header that does not satisfy the sc_file file
 * header format, the function reports the error using \ref SC_LERRORF,
 * collectively close the file and deallocate the file context. In this case
 * the function returns NULL on all ranks. A wrong file header format causes
 * \ref SC_FILE_ERR_FORMAT as \b errcode.
 *
 * This function does not abort on MPI I/O errors but returns NULL.
 * Without MPI I/O the function may abort on file system dependent
 * errors.
 *
 * \param [in]    mpicomm     The MPI communicator that is used to open the
 *                            parallel file.
 * \param [in]    filename    Path to parallel file that is to be opened.
 * \param [out]   user_string At least \ref SC_FILE_USER_STRING_BYTES + 1 bytes.
 *                            The user string is read on rank 0 and internally
 *                            broadcasted to all ranks.
 * \param [out]   errcode     An errcode that can be interpreted by \ref
 *                            sc_file_error_string.
 * \return                    Newly allocated context to continue reading
 *                            and eventually closing the file. NULL in case
 *                            of error, i.e. errcode != SC_FILE_SUCCESS.
 */
sc_file_context_t  *sc_file_open_read (sc_MPI_Comm mpicomm,
                                       const char *filename,
                                       char *user_string, int *errcode);

sc_file_context_t  *sc_file_write_block (sc_file_context_t * fc,
                                         size_t block_size,
                                         sc_array_t * block_data,
                                         const char *user_string,
                                         int *errcode);

/**
 *
 * \note                    If one wants to read a file section without knowing
 *                          file section type and the sizes, one can use the
 *                          function \ref sc_file_read.
 */
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

/** Deallocates memory that was allocated by \ref sc_file_read.
 *
 * This function is only dedicated to free memory that was allocated by
 * \ref sc_file_read using \b alloc_callback equals NULL. If the data was
 * allocated by \b alloc_callback the user is responsible to free the memory.
 *
 * \param [in]  data      The array that points to the data that is freed.
 * \param [out] errcode   An errcode that can be interpreted by \ref
 *                        sc_file_error_string.
 * \return                0 for a successful call and -1 in case of error.
 *                        See also \b errcode for further information on the
 *                        error.
 *
 * \note                  It is important to notice that this function
 *                        deallocates the memory that the sc_array \b data
 *                        points to but the sc_array structure was created
 *                        by the user and therefore must be reset or destroyed
 *                        by the user.
 */
int                 sc_file_free (sc_array_t * data, int *errcode);

/* sc_file_info */

/* sc_file_error_string */

/** Close a file opened for parallel write/read and the free the file context.
 *
 * Every call of \ref sc_file_open_read or \ref sc_file_open_write must be
 * matched by a corresponding call of \ref sc_file_close on the created file
 * context.
 *
 * This function does not abort on MPI I/O errors but returns NULL.
 * Without MPI I/O the function may abort on file system dependent
 * errors.
 *
 * \param [in,out]  fc        File context previously created by
 *                            \ref sc_file_open_read or \ref sc_file_open_write.
 *                            This file context is freed after a call of this
 *                            function.
 * \param [out]     errcode   An errcode that can be interpreted by \ref
 *                            sc_file_error_string.
 * \return                    0 for a successful call and -1 in case a of an
 *                            error. See also \b errcode argument.
 */
int                 sc_file_close (sc_file_context_t * fc, int *errcode);

SC_EXTERN_C_END;

#endif /* SC_FILE_H */
