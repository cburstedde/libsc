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
#define SC_FILE_FIELD_HEADER_BYTES (2 + SC_FILE_ARRAY_METADATA_BYTES + SC_FILE_USER_STRING_BYTES)
                                     /**< number of bytes of one field header */
#define SC_FILE_MAX_GLOBAL_QUAD 9999999999999999 /**< maximal number of global quadrants */
#define SC_FILE_MAX_BLOCK_SIZE 9999999999999 /**< maximal number of block bytes */
#define SC_FILE_MAX_FIELD_ENTRY_SIZE 9999999999999 /**< maximal number of bytes per field entry */

/** libsc data file format
 * All libsc data files have a \ref SC_FILE_HEADER_BYTES bytes file header at
 * the beginning of the file.
 *
 * File header (\ref SC_FILE_HEADER_BYTES bytes):
 * \ref SC_FILE_MAGIC_BYTES bytes magic number (scdata0) and one byte \ref SC_FILE_LINE_FEED_STR.
 * \ref SC_FILE_VERSION_STR_BYTES 
 * 
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

sc_file_context_t  *sc_file_open_write (const char *filename,
                                        sc_MPI_Comm mpicomm,
                                        const char *user_string,
                                        int *errcode);

sc_file_context_t  *sc_file_open_read (sc_MPI_Comm mpicomm,
                                       const char *filename,
                                       char *user_string, int *errcode);

sc_file_context_t  *sc_file_write_block (sc_file_context_t * fc,
                                         size_t block_size,
                                         sc_array_t * block_data,
                                         const char *user_string,
                                         int *errcode);

/** Write a variable size array file section.
 *
 * This function writes an array of variable-sized elements in parallel
 * to the opened file.
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
 * \param [out]     errcode An errcode that can be interpreted by \ref
 *                          sc_file_error_string.
 * \return                  Return a pointer to input context or NULL in case
 *                          of errors that does not abort the program.
 *                          In case of error the file is tried to close
 *                          and \b fc is freed.
 */
sc_file_context_t  *sc_file_write_variable (sc_file_context_t * fc,
                                            sc_array_t * sizes,
                                            sc_array_t * data, int *errcode);

/** Read a file section of an arbitrary section type.
 * 
 */
sc_file_context_t  *sc_file_read (sc_file_context_t * fc, sc_array_t * data,
                                  sc_file_section_t type,
                                  const char *user_string);

int                 sc_file_close (sc_file_context_t * fc, int *errcode);

SC_EXTERN_C_END;

#endif /* SC_FILE_H */
