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

#define SCDAT_HEADER_BYTES 128 /**< number of file header bytes in total incl. padding */
#define SCDAT_USER_STRING_BYTES 58 /**< number of user string bytes */

/** Opaque context used for writing a libsc data file. */
typedef struct scdat_fcontext scdat_fcontext_t;

/** Error values for sc_file functions.
 */
/* TODO */
typedef enum scdat_ferror
{
  SCDAT_FERROR_SUCCESS = sc_MPI_ERR_LASTCODE,
  SCDAT_FERR_FILE, /**< invalid file handle */
  SCDAT_FERR_NOT_SAME, /**< collective arg not identical */
  SCDAT_FERR_AMODE, /**< access mode error */
  SCDAT_FERR_NO_SUCH_FILE, /**< file does not exist */
  SCDAT_FERR_FILE_EXIST, /**< file exists already */
  SCDAT_FERR_BAD_FILE, /**< invalid file name */
  SCDAT_FERR_ACCESS, /**< permission denied */
  SCDAT_FERR_NO_SPACE, /**< not enough space */
  SCDAT_FERR_QUOTA, /**< quota exceeded */
  SCDAT_FERR_READ_ONLY, /**< read only file (system) */
  SCDAT_FERR_IN_USE, /**< file currently open by other process */
  SCDAT_FERR_IO, /**< other I/O error */
  SCDAT_FERR_FORMAT,  /**< read file has a wrong format */
  SCDAT_FERR_SECTION_TYPE, /**< a valid non-matching section type */
  SCDAT_FERR_IN_DATA, /**< input data of file function is invalid */
  SCDAT_FERR_COUNT,   /**< read or write count error that was not
                                 classified as a format error */
  SCDAT_FERR_UNKNOWN, /**< unknown error */
  SCDAT_FERR_LASTCODE /**< to define own error codes for
                                  a higher level application
                                  that is using sc_file
                                  functions */
}
scdat_ferror_t;

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
 *                          case of error, i.e. errcode != SCDAT_FERROR_SUCCESS.
 */
scdat_fcontext_t   *scdat_fopen (sc_MPI_Comm mpicomm,
                                 const char *filename,
                                 char mode,
                                 const char *user_string, int *errcode);

/** Write an inline data section.
 *
 * This is a collective function.
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
 * This is a collective function.
 * This function writes a data block of fixed size to the file. The data
 * and its section header is written on the MPI rank \b root.
 * The number of block bytes must be less or equal 10^{26} - 1.
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
 * \param [in]      block_size  The size of the data block in bytes. Must be
 *                              less or equal than 10^{26} - 1.
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
                                        int root, int *errcode);

/** Write a fixed-size array file section.
 *
 * This is a collective function.
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
 *                              of the sc_array must be equal to sizeof (uint8_t).
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
 * This is a collective function.
 * This function can be used instead of \ref scdat_fwrite_array if the array
 * elements do not have a constant element size in bytes.
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
 *                              \b proc_sizes. The data of the array must be the
 *                              local array elements conforming with
 *                              \b elem_counts, \b proc_sizes and \b elem_sizes.
 *                              If \b indirect is true, \b array_data must be
 *                              a sc_array with element count equal to the p-th
 *                              array entry of \b elem_counts and element size
 *                              equal to sizeof (sc_array_t). Each array element
 *                              is again a sc_array. Now, with element count 1
 *                              and element size equals to the actual array
 *                              element size as passed in \b elem_sizes.
 *                              corresponding entries in \b proc_sizes.
 * \param [in]      elem_counts An sc_array that must be equal on all
 *                              ranks. The element count of \b elem_counts
 *                              must be the mpisize of the MPI communicator
 *                              that was used to create \b fc. The element size
 *                              of the sc_array must be equal to sizeof (unint8_t).
 *                              The sc_array must contain the local array elements
 *                              counts (unsigned int). That is why it induces
 *                              the partition that is used to write the array
 *                              data in parallel.
 * \param [in]      elem_sizes  A sc_array with the element sizes for the local
 *                              array elements. The sc_array has an element
 *                              count of p-th entry of \b elem_counts for p
 *                              being the calling rank. The element size is
 *                              sizeof (uint8_t).
 * \param [in]      proc_sizes  An sc_array that must be equal on all
 *                              ranks. The element count and element size
 *                              must be the same as for \b elem_counts. The
 *                              array must contain the overall byte count per
 *                              rank conforming with the passed array element
 *                              partition \b elem_counts and the local array
 *                              element sizes in \b elem_sizes.
 * \param [in]      indirect    A Boolean to determine whether \b array_data
 *                              must be a sc_array of sc_arrays to write
 *                              indirectly and in particular from potentially
 *                              non-contigous memory. In the remaining case of
 *                              \b indirect being false \b array_data must be
 *                              a sc_array with the actual array elements as
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
                                         sc_array_t * proc_sizes,
                                         int indirect,
                                         const char *user_string,
                                         int *errcode);

/** Read the next file section header.
 *
 * This is a collective function.
 * This functions reads the next file section header and provides the user
 * information on the subsequent file section that can be used to read the
 * actual data in a next calling depending on the file section type one
 * (or for a variable-size array two) functions out of \ref scdat_fread_block,
 * \ref scdat_fread_array_data, \ref scdat_fread_varray_sizes and \ref
 * scdat_fread_varray_data. In the case that the considered file section is a
 * an inline data section one does not need any further function calls since
 * the file section is already completely read and the user could proceed by
 * trying to read the next file section header.
 *
 * Except of \b bytes32 all output parameters are internally broadcasted.
 *
 * This function does not abort on MPI I/O errors but returns NULL.
 * Without MPI I/O the function may abort on file system dependent
 * errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_fopen with mode 'r'.
 * \param [out]     type        On output this char is set
 *                              'I' (inline data), 'B' (block of given size),
 *                              'A' (fixed-size array) or 'V' (variable-size
 *                              array) depending on the file section type.
 * \param [out]     bytes32     Exactly 32 bytes on the rank \b root or NULL
 *                              on \b root to not read the bytes. These bytes
 *                              are only filled on output if \b type
 *                              is equal to 'I'. The parameter is ignored on
 *                              all ranks unequal to \b root.
 * \param [out]     elem_count  On output set to the number of bytes if \b type
 *                              equals 'B' to the global number of array elements
 *                              if \b type equals 'A' or 'V'. For 'I' as \b type
 *                              \b elem_count is set 0.
 * \param [out]     elem_size   On output set to the byte count of the array
 *                              elements if \b type is 'A'. Otherwise
 *                              \b elem_size is set to 0.
 * \param [out]     user_string At least \ref SCDAT_USER_STRING_BYTES + 1 bytes.
 *                              On output filled with the user section string.
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
 *                              reading and eventually closing the file.
 */
scdat_fcontext_t   *scdat_fread_section_header (scdat_fcontext_t * fc,
                                                char *type,
                                                void *bytes_32,
                                                size_t *elem_count,
                                                size_t *elem_size,
                                                char *user_string,
                                                int root, int *errcode);

/** Read the data of a block of given size.
 *
 * This is a collective function.
 * This function is only valid to call directly after a call of \ref
 * scdat_fread_section_header. This preceding call gives also the required
 * \b block_size.
 *
 * This function does not abort on MPI I/O errors but returns NULL.
 * Without MPI I/O the function may abort on file system dependent
 * errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_fopen with mode 'r'.
 * \param [out]     block_data  A sc_array with element count 1 and element
 *                              size \b block_size. On output the sc_array is
 *                              filled with the block data of the read block data
 *                              section.
 * \param [in]      block_size  The number of bytes of the block as retrieved
 *                              from the preceding call of \ref
 *                              scdat_fread_section_header.
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
 *                              reading and eventually closing the file.
 */
scdat_fcontext_t   *sc_fread_block (scdat_fcontext_t * fc,
                                    sc_array_t * block_data,
                                    size_t block_size, int root,
                                    int *errcode);

/** Read the data of a fixed-size array.
 *
 * This is a collective function.
 * This function is only valid to call directly after a call of \ref
 * scdat_fread_section_header. This preceding call gives also the required
 * \b elem_size and the global number of array elements. The user must pass
 * a parallel partition of the array elements by \b elem_counts.
 *
 * This function does not abort on MPI I/O errors but returns NULL.
 * Without MPI I/O the function may abort on file system dependent
 * errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_fopen with mode 'r'.
 * \param [out]     array_data  If \b indirect is false, a sc_array with
 *                              element count equals to the p-th entry of
 *                              \b elem_counts for p being the calling rank.
 *                              The element size must be equal to \b elem_size.
 *                              If \b indirect is true, a sc_array with the
 *                              same element count as fir \b indirect false
 *                              but with sizeof (sc_array_t) as element size.
 *                              Each array element is then again a sc_array but
 *                              with element count 1 and element size
 *                              \b elem_size.
 * \param [in]      elem_counts An sc_array that must be equal on all
 *                              ranks. The element count of \b elem_counts
 *                              must be the mpisize of the MPI communicator
 *                              that was used to create \b fc. The element size
 *                              of the sc_array must be equal to sizeof (uint8_t).
 *                              The sc_array must contain the local array elements
 *                              counts. That is why it induces
 *                              the partition that is used to read the array
 *                              data in parallel. The sum of all array elements
 *                              must be equal to elem_count as retrieved from
 *                              \ref scdat_fread_section_header.
 * \param [in]      elem_size   The element size of one array element on number
 *                              of bytes. Must be the same on all ranks and as
 *                              retrieved from \ref scdat_fread_section_header.
 * \param [in]      indirect    A Boolean to determine whether \b array_data
 *                              must be a sc_array of sc_arrays to read
 *                              indirectly and in particular from potentially
 *                              non-contigous memory. See the documentation of
 *                              the parameter \b array_data for more information.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_ferror_string.
 * \return                      Return a pointer to input context or NULL in case
 *                              of errors that does not abort the program.
 *                              In case of error the file is tried to close
 *                              and \b fc is freed.
 *                              The scdat file context can be used to continue
 *                              reading and eventually closing the file.
 */
scdat_fcontext_t   *scdat_fread_array_data (scdat_fcontext_t * fc,
                                            sc_array_t * array_data,
                                            sc_array_t * elem_counts,
                                            size_t elem_size,
                                            int indirect, int *errcode);

/** Read the element sizes of a variable-size array.
 *
 * This is a collective function.
 * This function is only valid to call directly after a call of \ref
 * scdat_fread_section_header. This preceding call gives also the for
 * \b elem_counts required global number of array elements. The user must pass
 * a parallel partition of the array elements by \b elem_counts.
 *
 * This function does not abort on MPI I/O errors but returns NULL.
 * Without MPI I/O the function may abort on file system dependent
 * errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_fopen with mode 'r'.
 * \param [out]     elem_sizes  A sc_array with element count equals to
 *                              p-th entry of \b elem_counts for p being the
 *                              calling rank. The element size must be
 *                              sizeof (uint8_t). On output the array is
 *                              filled with the local array element byte counts,
 *                              where locality is determined by \b elem_counts.
 * \param [in]      elem_counts An sc_array that must be equal on all
 *                              ranks. The element count of \b elem_counts
 *                              must be the mpisize of the MPI communicator
 *                              that was used to create \b fc. The element size
 *                              of the sc_array must be equal to sizeof (uint8_t).
 *                              The sc_array must contain the local array elements
 *                              counts. That is why it induces
 *                              the partition that is used to read the array
 *                              data in parallel. The sum of all array elements
 *                              must be equal to elem_count as retrieved from
 *                              \ref scdat_fread_section_header.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_ferror_string.
 * \return                      Return a pointer to input context or NULL in case
 *                              of errors that does not abort the program.
 *                              In case of error the file is tried to close
 *                              and \b fc is freed.
 *                              The scdat file context can be used to continue
 *                              reading and eventually closing the file.
 */
scdat_fcontext_t   *scdat_fread_varray_sizes (scdat_fcontext_t * fc,
                                              sc_array_t * elem_sizes,
                                              sc_array_t * elem_counts,
                                              int *errcode);

/** Read the data of a variable-size array.
 *
 * This is a collective function.
 * This function is only valid to call directly after a call of \ref
 * scdat_fread_varray_sizes. This preceding call gives also the required
 * \b elem_sizes.
 *
 * This function does not abort on MPI I/O errors but returns NULL.
 * Without MPI I/O the function may abort on file system dependent
 * errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_fopen with mode 'r'.
 * \param [out]     array_data  Let p be the calling rank. If \b indirect is
 *                              false, \b array_data must have element count 1
 *                              and as element size the p-th entry of
 *                              \b proc_sizes. On output the data of the array
 *                              is set to the local array elements conforming
 *                              with \b elem_counts, \b proc_sizes and
 *                              \b elem_sizes.
 *                              If \b indirect is true, \b array_data must be
 *                              a sc_array with element count equal to the p-th
 *                              array entry of \b elem_counts and element size
 *                              equal to sizeof (sc_array_t). Each array element
 *                              is again a sc_array. Now, with element count 1
 *                              and element size equals to the actual array
 *                              element size as passed in \b elem_sizes.
 *                              On output these arrays are filled with the
 *                              actual elements of the read variable-size array.
 * \param [in]    elem_sizes    The local element sizes conforming to the array
 *                              element partition \b elem_counts as retrieved
 *                              from \ref scdat_fread_varray_sizes.
 * \param [in]    elem_counts   An sc_array that must be equal on all
 *                              ranks. The element count of \b elem_counts
 *                              must be the mpisize of the MPI communicator
 *                              that was used to create \b fc. The element size
 *                              of the sc_array must be equal to sizeof (uint8_t).
 *                              The sc_array must contain the local array elements
 *                              counts. That is why it induces
 *                              the partition that is used to read the array
 *                              data in parallel. The sum of all array elements
 *                              must be equal to elem_count as retrieved from
 *                              \ref scdat_fread_section_header.
 * \param [in]      proc_sizes  An sc_array that must be equal on all
 *                              ranks. The element count and element size
 *                              must be the same as for \b elem_counts. The
 *                              array must contain the overall byte count per
 *                              rank conforming with the passed array element
 *                              partition \b elem_counts and the local array
 *                              element sizes in \b elem_sizes.
 * \param [in]      indirect    A Boolean to determine whether \b array_data
 *                              must be a sc_array of sc_arrays to read
 *                              indirectly and in particular from potentially
 *                              non-contigous memory. See the documentation of
 *                              the parameter \b array_data for more information.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_ferror_string.
 * \return                      Return a pointer to input context or NULL in case
 *                              of errors that does not abort the program.
 *                              In case of error the file is tried to close
 *                              and \b fc is freed.
 *                              The scdat file context can be used to continue
 *                              reading and eventually closing the file.
 */
scdat_fcontext_t   *scdat_fread_varray_data (scdat_fcontext_t * fc,
                                             sc_array_t * array_data,
                                             sc_array_t * elem_sizes,
                                             sc_array_t * elem_counts,
                                             sc_array_t * proc_sizes,
                                             int indirect, int *errcode);

/** Translate a scdat error code to an error string.
 *
 * \param [in]    errcode       An errcode that is output by a
 *                              scdat function.
 * \param [out]   string        At least sc_MPI_MAX_ERROR_STRING bytes.
 * \param [out]   len           Length of string on return.
 * \return                      SCDAT_FERROR_SUCCESS on success or
 *                              something else on invalid arguments.
 */
int                 scdat_ferror_string (int errcode, char *str, int *len);

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
 * \return                    \ref SCDAT_FERROR_SUCCESS for a successful call
 *                            and -1 in case a of an error.
 *                            See also \b errcode argument.
 */
int                 sc_fclose (scdat_fcontext_t * fc, int *errcode);

SC_EXTERN_C_END;

#endif /* SC_FILE_H */
