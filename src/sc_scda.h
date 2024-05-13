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

/** \file sc_scda.h
 *
 * Routines for parallel I/O with the \b scda format.
 *
 * Functionality to write and read in parallel using a prescribed
 * serial-equivalent file format called \b scda.
 *
 * ### General
 *
 * The \b scda format is as in this
 * [preprint](https://doi.org/10.48550/arXiv.2307.06789).
 *
 * However, in contrast to the preprint the API in this file provides the two
 * functions \ref sc_scda_fopen_write and \ref sc_scda_fopen_read instead of
 * providing one opening function with a mode parameter to decide and writing
 * and reading.
 *
 * In addition, we add in this file the options structure \ref
 * sc_scda_fopen_options as parameter for opening files.
 *
 * The file format includes
 * metadata in ASCII and therefore enables the human eye to parse the
 * file structure using a standard text editor.
 *
 * The file format \b scda is in particular suitable for parallel I/O and is
 * accompanied by a convention for element-wise compression.
 *
 * The format is designed such that the the parallel partition and in particular
 * the process count can differ between writing and reading.
 *
 * The main purpose of \b scda is to enable the user to implement parallel I/O
 * for numerical appliations, e.g. simulation checkpoint/restart.
 *
 * We elaborate further on the workflow in \ref scda_workflow .
 *
 * ### User Strings
 *
 * The functions
 *
 * - \ref sc_scda_fopen_write,
 * - \ref sc_scda_fwrite_inline,
 * - \ref sc_scda_fwrite_block,
 * - \ref sc_scda_fwrite_array,
 * - \ref sc_scda_fwrite_varray,
 * - \ref sc_scda_fopen_read and
 * - \ref sc_scda_fread_section_header
 *
 * have \b user_string and \b len as an argument.
 *
 * The user string consisting of the two parameters \b user_string and \b len is
 * always a collective parameter.
 *
 * In the case of writing these arguments have the purpose to pass the user
 * string that is written to the file.
 *
 * There are two options writing the user string to the file.
 *
 * 1. A nul-terminated string \b user_string, i.e. a standard C string. In this
 * case \b len is set to NULL since the length of \b user_string is implicitly
 * given by the nul-termination.
 *
 * 2. Arbitrary data for the user string including possible '\0' in
 * non-terminating positions. We still require nul-termination for safety.
 * One needs to explicitly pass the length, i.e. the number of bytes
 * excluding the nul-termination.
 *
 * For both options it must be respected that the number of maximal \b user_string
 * bytes is \ref SC_SCDA_USER_STRING_BYTES + 1 including the nul-termination.
 *
 * In case of reading \b user_string must be at least \ref
 * SC_SCDA_USER_STRING_BYTES + 1 bytes. On output \b len is set to the number
 * of bytes actually written to \b user_string excluding the nul-termination.
 *
 * If it is desired to write arbitrary data without the nul-termination, which
 * is required for the user string, one can write a block data section using
 * the function \ref sc_scda_fwrite_block.
 *
 * ### Encoding
 *
 * \b scda provides optional transparent, element-wise compression of data of
 * file sections.
 * The compression for writing can be enabled by passing true for \b encode to
 * one of the functions
 *
 * - \ref sc_scda_fwrite_inline,
 * - \ref sc_scda_fwrite_block,
 * - \ref sc_scda_fwrite_array and
 * - \ref sc_scda_fwrite_varray.
 *
 * The compression on writing can be decided by the user for each file section
 * separately. All parameters on the size of the data written to the file refer
 * to the uncompressed data.
 *
 * On reading it sufficies to pass \b decode true to the function \ref
 * sc_scda_fread_section_header. Then the file section data is decompressed
 * if it was compressed and otherwise the file section data is read raw.
 * Again all data sizes in reading functions refer to the uncompressed data.
 *
 * If \b decode is false, the data is read raw even if it was written according
 * to the compression convention.
 *
 * ### Error management
 *
 * All \b scda functions that receive a file context have an output parameter
 * called \b errcode. In case of an unsuccessful function call, the respective
 * function returns NULL instead of the file context and sets the output
 * parameter \b errcode to the respective error code. If such a case occurs,
 * the file that is associated to the used file context is closed and the file
 * context is deallocated.
 *
 * The error code can be examined by the user using the two functions
 * - \ref sc_scda_ferror_class and
 * - \ref sc_scda_ferror_string.
 *
 * If MPI is available \b errcode may encode an MPI error code. In this case
 * the two error examination functions output the error class and error string
 * as it would be output by the corresponding MPI functions, respectively.
 * Without MPI it is still possible that \b errcode encodes an I/O
 * operation related error code. This case does not differ concerning the error
 * code examiniation.
 * Moreover, \b errcode can encode an error code related to \b scda, i.e.
 * the I/O operations were successful but there is some violation of the \b scda
 * format, workflow or API.
 *
 * For more technical details on \b errcode see the documentation of \ref
 * sc_scda_ferror_t. Furthermore, the \b scda format, workflow or API errors
 * description can be found in the documentation of \ref sc_scda_ret.
 *
 * \ingroup io
 */

/**
 * \page scda_workflow Parallel I/O workflow
 *
 * The general workflow for the scda format provided by \ref sc_scda.h
 *
 * All workflows start with \ref sc_scda_fopen_write or \ref sc_scda_fopen_read
 * that creates a file context \ref sc_scda_fcontext_t that never moves backwards.
 *
 * Then the user can call a sequence of functions out of:
 *
 * - \ref sc_scda_fwrite_inline,
 * - \ref sc_scda_fwrite_block,
 * - \ref sc_scda_fwrite_array and
 * - \ref sc_scda_fwrite_varray,
 *
 * for the case of using \ref sc_scda_fopen_write.
 *
 * Alternatively, for using \ref sc_scda_fopen_read the user must call
 * \ref sc_scda_fread_section_header that examines the current file section
 * and then call accordingly to the retrieved file section type
 *
 * - \ref sc_scda_fread_inline_data,
 * - \ref sc_scda_fread_block_data,
 * - \ref sc_scda_fread_array_data,
 * - \ref sc_scda_fread_varray_sizes and
 * - \ref sc_scda_fread_varray_data.
 *
 * Finally, the file context is collectively closed and deallocated by \ref
 * sc_scda_fclose.
 *
 * All above mentioned sc_scda functions are collective and output an
 * error code (cf. \ref sc_scda_ferror_t) that can be examined by \ref
 * sc_scda_ferror_string.
 *
 * For more details and the option for encoded data see the functions
 * in \ref sc_scda.h.
 */

#ifndef SC_SCDA_H
#define SC_SCDA_H

#include <sc_containers.h>

SC_EXTERN_C_BEGIN;

#define SC_SCDA_HEADER_BYTES 128 /**< number of file header bytes */
#define SC_SCDA_USER_STRING_BYTES 58 /**< number of user string bytes */
#define SC_SCDA_FILENAME_BYTES BUFSIZ /**< maximal number of filename bytes */

/** Opaque context for writing and reading a libsc data file, i.e. a scda file. */
typedef struct sc_scda_fcontext sc_scda_fcontext_t;

/** Type for element counts and sizes. */
typedef uint64_t    sc_scda_ulong;

/** Error values for scda-related errors.
 *
 * The error codes are part of the struct \ref sc_scda_ferror_t and can be
 * examined as part of this struct.
 */
typedef enum sc_scda_ret
{
  SC_SCDA_FERR_SUCCESS = 0, /**< successful function call */
  SC_SCDA_FERR_FORMAT = 15000, /**< File not conforming to the
                                                   \b scda format. */
  SC_SCDA_FERR_USAGE,   /**< Incorrect workflow of an \b scda reading function.
                             For example, the user might have identified a
                             certain file section type
                             using \ref sc_scda_fread_section_header but then
                             calls a function to read the section
                             data for a different type.
                             Another example is to try reading the data
                             of a 'V' section before reading its element sizes.
                             This error also occurs when the user tries to
                             read section data before reading the section header.
                          */
  SC_SCDA_FERR_DECODE,  /**< The decode parameter to
                            \ref sc_scda_fread_section_header is true but
                            the file section header(s) encountered
                            does not conform to the \b scda encoding convention.
                           */
  SC_SCDA_FERR_ARG, /**< An argument to a \b scda file function is invalid.
                           This occurs for example when an essential pointer
                           argument is NULL or a user string for
                           writing is too long. */
  SC_SCDA_FERR_COUNT, /**< A byte count error that may occur
                           transiently on writing or the file is short
                           on reading. */
  SC_SCDA_FERR_MPI,   /**< An MPI error occurred; see \b mpiret in the
                           corresponding \ref sc_scda_ferror_t. */
  SC_SCDA_FERR_LASTCODE /**< to define own error codes for
                             a higher level application
                             that is using sc_scda
                             functions */
}
sc_scda_ret_t;

/** Error values for the scda functions.
 * An error value is a struct since the error can be related to the scda
 * file format or to (MPI) I/O operations. The error code can be converted to a
 * string by \ref sc_scda_ferror_string and mapped to an error class by \ref
 * sc_scda_ferror_class.
 *
 * The parsing logic of \ref sc_scda_ferror_t is that first \b scdaret is examined
 * and if \b scdaret != \ref SC_SCDA_FERR_MPI, we know that \b mpiret = 0.
 * If \b scdaret = \ref SC_SCDA_FERR_MPI, we know that an MPI error occurred and
 * we can examine \b mpiret for more informartion.
 *
 * Moreover, a valid sc_scda_ferror always satisfy that if \b scdaret = 0 then
 * \b mpiret = 0 and if \b scdaret = \ref SC_SCDA_FERR_MPI then \b mpiret !=0.
 */
typedef struct sc_scda_ferror
{
  int mpiret;            /**< MPI function return value; without MPI
                              this variable can get filled by other I/O operation
                              error codes, which are still interpretable by
                              the error \b scda examination functions */
  sc_scda_ret_t scdaret; /**< scda file format related return value */
}
sc_scda_ferror_t;

/** An options struct for the functions \ref sc_scda_fopen_write and
 * \ref sc_scda_fopen_read. The struct may be extended in the future.
 */
typedef struct sc_scda_fopen_options
{
  sc_MPI_Info         info; /**< info that is passed to MPI_File_open */
  int                 fuzzy_errors; /**< boolean for fuzzy error return */
}
sc_scda_fopen_options_t; /**< type for \ref sc_scda_fopen_options */

/** Open a file for writing and write the file header to the file.
 *
 * This function creates a new file or overwrites an existing one.
 * It is collective and creates the file on a parallel file system.
 * \note
 * All parameters are collective.
 * This function leaves the file open if MPI is available.
 * Independent of the availability of MPI the user can write one or more
 * file sections before closing the file (context) using \ref sc_scda_fclose.
 *
 * It is the user's responsibility to write any further
 * metadata of the file that is required by the application. This can be done
 * by writing file sections. However, the user can use \ref sc_scda_fopen_read
 * to open a not already opened file and then use \ref
 * sc_scda_fread_section_header and skipping the respective data bytes using the
 * respective read functions sc_scda_fread_*_data to parse the structure
 * of a given file and some metadata that is written by sc_scda.
 *
 * This function differs from the one opening function for writing and reading
 * introduced in this \b scda
 * [preprint](https://doi.org/10.48550/arXiv.2307.06789).
 *
 * This function returns NULL on I/O errors.
 *
 * \param [in]     mpicomm   The MPI communicator that is used to open the
 *                           parallel file.
 * \param [in]     filename  Path to parallel file that is to be created or
 *                           to be opened.
 * \param [in]   user_string At most \ref SC_SCDA_USER_STRING_BYTES + 1 bytes
 *                           in a nul-terminated string. See the 'User Strings'
 *                           section in the detailed description of this file
 *                           for more information.
 * \param [in]    len        The number of bytes in \b user_string excluding the
 *                           terminating nul.
 *                           On NULL as input \b user_string is expected to be a
 *                           nul-terminated C string.
 * \param [in]     opt       An options structure that provides the possibility
 *                           to pass further options. See \ref
 *                           sc_scda_fopen_options for more details.
 *                           It is valid to pass NULL for \b opt.
 * \param [out]    errcode   An errcode that can be interpreted by \ref
 *                           sc_scda_ferror_string or mapped to an error class
 *                           by \ref sc_scda_ferror_class.
 * \return                   Newly allocated context to continue writing
 *                           and eventually closing the file. NULL in
 *                           case of error, i.e. errcode != \ref
 *                           SC_SCDA_FERR_SUCCESS.
 */
sc_scda_fcontext_t *sc_scda_fopen_write (sc_MPI_Comm mpicomm,
                                         const char *filename,
                                         const char *user_string, size_t *len,
                                         sc_scda_fopen_options_t * opt,
                                         sc_scda_ferror_t * errcode);

/** Write an inline data section.
 *
 * This is a collective function.
 * This function writes 32 bytes of user-defined data preceded by a file
 * section header containing a user string. In contrast to other file sections
 * the inline data section does not end with padded data bytes and therefore
 * require exactly 32 bytes data from the user. This enables the user to
 * implement custom file structuring or padding.
 * \note
 * All parameters except of \b data are collective.
 *
 * This function returns NULL on I/O errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_scda_fopen_write.
 * \param [in]      user_string At most \ref SC_SCDA_USER_STRING_BYTES + 1 bytes
 *                              in a nul-terminated string. See the 'User Strings'
 *                              section in the detailed description of this file
 *                              for more information.
 * \param [in]      len         The number of bytes in \b user_string excluding
 *                              the terminating nul.
 *                              On NULL as input \b user_string is expected to
 *                              be a nul-terminated C string.
 * \param [in]      inline_data On the rank \b root a sc_array with element
 *                              count 1 and element size 32. On all other ranks
 *                              this parameter is ignored.
 * \param [in]      root        An integer between 0 and mpisize exclusive of
 *                              the MPI communicator that was used to create
 *                              \b fc. \b root indicates the MPI rank on that
 *                              \b inline_data is written to the file.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_scda_ferror_string or mapped to an error class
 *                              by \ref sc_scda_ferror_class.
 * \return                      Return a pointer to the input
 *                              context \b fc on success.
 *                              The context is used to continue
 *                              writing and eventually closing the file.
 *                              In case of any error, attempt to close the
 *                              file and deallocate the context \b fc.
 */
sc_scda_fcontext_t *sc_scda_fwrite_inline (sc_scda_fcontext_t * fc,
                                           const char *user_string,
                                           size_t *len,
                                           sc_array_t * inline_data, int root,
                                           sc_scda_ferror_t * errcode);

/** Write a fixed-size block file section.
 *
 * This is a collective function.
 * This function writes a data block of fixed size to the file. The \b block_data
 * is written on the MPI rank \b root.
 * The number of block bytes must be less or equal 10^{26} - 1.
 * \note
 * All parameters except of \b block_data are collective.
 *
 * This function returns NULL on I/O errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_scda_fopen_write.
 * \param [in]      user_string At most \ref SC_SCDA_USER_STRING_BYTES + 1 bytes
 *                              in a nul-terminated string. See the 'User Strings'
 *                              section in the detailed description of this file
 *                              for more information.
 * \param [in]      len         The number of bytes in \b user_string excluding
 *                              the terminating nul.
 *                              On NULL as input \b user_string is expected to
 *                              be a nul-terminated C string.
 * \param [in]      block_data  On rank \b root a sc_array with one element and
 *                              element size equals to \b block_size. On all
 *                              other ranks the parameter is ignored.
 * \param [in]      block_size  The size of the data block in bytes. Must be
 *                              less or equal than 10^{26} - 1.
 * \param [in]      root        An integer between 0 and mpisize of the MPI
 *                              communicator that was used to create \b fc.
 *                              \b root indicates the MPI rank on that
 *                              \b block_data is written to the file.
 * \param [in]      encode      A Boolean to decide whether the file section
 *                              is written compressed. This results in two
 *                              written file sections that can be read without
 *                              the encoding interpretation by using \ref
 *                              sc_scda_fread_section_header with decode set to
 *                              false followed by the usual sc_scda_fread
 *                              functions. The data can be read as passed to
 *                              this function by using decode true in \ref
 *                              sc_scda_fread_section_header and calling the
 *                              usual sc_scda_fread function. See also
 *                              the 'Encoding' section in the detailed
 *                              description in this file.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_scda_ferror_string or mapped to an error class
 *                              by \ref sc_scda_ferror_class.
 * \return                      Return a pointer to the input
 *                              context \b fc on success.
 *                              The context is used to continue
 *                              writing and eventually closing the file.
 *                              In case of any error, attempt to close the
 *                              file and deallocate the context \b fc.
 */
sc_scda_fcontext_t *sc_scda_fwrite_block (sc_scda_fcontext_t * fc,
                                          const char *user_string,
                                          size_t *len,
                                          sc_array_t * block_data,
                                          size_t block_size, int root,
                                          int encode,
                                          sc_scda_ferror_t * errcode);

/** Write a fixed-size array file section.
 *
 * This is a collective function.
 * The fixed-size array is the simplest file section that enables the user to
 * write and read data in parallel. This function writes an array of a given
 * element global count and a fixed element size.
 * \note
 * All parameters except of \b array_data are collective.
 *
 * This function returns NULL on I/O errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_scda_fopen_write.
 * \param [in]      user_string At most \ref SC_SCDA_USER_STRING_BYTES + 1 bytes
 *                              in a nul-terminated string. See the 'User Strings'
 *                              section in the detailed description of this file
 *                              for more information.
 * \param [in]      len         The number of bytes in \b user_string excluding
 *                              the terminating nul.
 *                              On NULL as input \b user_string is expected to
 *                              be a nul-terminated C string.
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
 *                              of the sc_array must be equal to sizeof (\ref
 *                              sc_scda_ulong). The sc_array must contain the
 *                              local array elements counts (\ref sc_scda_ulong).
 *                              That is why it induces the partition that is
 *                              used to write the array data in parallel.
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
 * \param [in]      encode      A Boolean to decide whether the file section
 *                              is written compressed. This results in two
 *                              written file sections that can be read without
 *                              the encoding interpretation by using \ref
 *                              sc_scda_fread_section_header with decode set to
 *                              false followed by the usual sc_scda_fread
 *                              functions. The data can be read as passed to
 *                              this function by using decode true in \ref
 *                              sc_scda_fread_section_header and calling the
 *                              usual sc_scda_fread function. See also
 *                              the 'Encoding' section in the detailed
 *                              description in this file.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_scda_ferror_string or mapped to an error class
 *                              by \ref sc_scda_ferror_class.
 * \return                      Return a pointer to the input
 *                              context \b fc on success.
 *                              The context is used to continue
 *                              writing and eventually closing the file.
 *                              In case of any error, attempt to close the
 *                              file and deallocate the context \b fc.
 */
sc_scda_fcontext_t *sc_scda_fwrite_array (sc_scda_fcontext_t * fc,
                                          const char *user_string,
                                          size_t *len,
                                          sc_array_t * array_data,
                                          sc_array_t * elem_counts,
                                          size_t elem_size, int indirect,
                                          int encode,
                                          sc_scda_ferror_t * errcode);

/** This is a collective function to determine the processor sizes.
 *
 * The purpose of this function is determine the \b proc_sizes argument
 * as required by \ref sc_scda_fread_varray_data given the data that can
 * be retrieved by calling \ref sc_scda_fread_varray_sizes or as required by
 * \ref sc_scda_fwrite_varray given \b elem_sizes and \b elem_counts as passed
 * to \ref sc_scda_fwrite_varray.
 * \note
 * All parameters are collective.
 *
 * \param [in]    elem_sizes    The \b elem_sizes array as retrieved by \ref
 *                              sc_scda_fread_varray_sizes or passed to \ref
 *                              sc_scda_fwrite_varray.
 * \param [in]    elem_counts   The \b elem_counts array as retrieved by \ref
 *                              sc_scda_fread_varray_sizes or passed to \ref
 *                              sc_scda_fwrite_varray.
 * \param [out]   proc_sizes    A sc_array with element size \ref sc_scda_ulong
 *                              that is resized on output to the length of
 *                              \b elem_sizes and \b elem_counts. The array is
 *                              filled with the number bytes per process.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_scda_ferror_string or mapped to an error class
 *                              by \ref sc_scda_ferror_class.
 * \return                      0 in case of success and -1 otherwise.
 */
int                 sc_scda_proc_sizes (sc_array_t * elem_sizes,
                                        sc_array_t * elem_counts,
                                        sc_array_t * proc_sizes,
                                        sc_scda_ferror_t * errcode);

/** Write a variable-size array file section.
 *
 * This is a collective function.
 * This function can be used instead of \ref sc_scda_fwrite_array if the array
 * elements do not have a constant element size in bytes.
 * \note
 * All parameters except of \b array_data and \b elem_sizes are collective.
 *
 * This function returns NULL on I/O errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_scda_fopen_write.
 * \param [in]      user_string At most \ref SC_SCDA_USER_STRING_BYTES + 1 bytes
 *                              in a nul-terminated string. See the 'User Strings'
 *                              section in the detailed description of this file
 *                              for more information.
 * \param [in]      len         The number of bytes in \b user_string excluding
 *                              the terminating nul.
 *                              On NULL as input \b user_string is expected to
 *                              be a nul-terminated C string.
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
 * \param [in]      elem_counts An sc_array that must be equal on all
 *                              ranks. The element count of \b elem_counts
 *                              must be the mpisize of the MPI communicator
 *                              that was used to create \b fc. The element size
 *                              of the sc_array must be equal to sizeof (unint8_t).
 *                              The sc_array must contain the local array elements
 *                              counts. That is why it induces
 *                              the partition that is used to write the array
 *                              data in parallel.
 * \param [in]      elem_sizes  A sc_array with the element sizes for the local
 *                              array elements. The sc_array has an element
 *                              count of p-th entry of \b elem_counts for p
 *                              being the calling rank. The element size is
 *                              sizeof (\ref sc_scda_ulong).
 * \param [in]      proc_sizes  An sc_array that must be equal on all
 *                              ranks. The element count and element size
 *                              must be the same as for \b elem_counts. The
 *                              array must contain the overall byte count per
 *                              rank conforming with the passed array element
 *                              partition \b elem_counts and the local array
 *                              element sizes in \b elem_sizes. This parameter
 *                              can be computed using \ref sc_scda_proc_sizes.
 * \param [in]      indirect    A Boolean to determine whether \b array_data
 *                              must be a sc_array of sc_arrays to write
 *                              indirectly and in particular from potentially
 *                              non-contigous memory. In the remaining case of
 *                              \b indirect being false \b array_data must be
 *                              a sc_array with the actual array elements as
 *                              data as further explained in the documentation
 *                              of \b array_data.
 * \param [in]      encode      A Boolean to decide whether the file section
 *                              is written compressed. This results in two
 *                              written file sections that can be read without
 *                              the encoding interpretation by using \ref
 *                              sc_scda_fread_section_header with decode set to
 *                              false followed by the usual sc_scda_fread
 *                              functions. The data can be read as passed to
 *                              this function by using decode true in \ref
 *                              sc_scda_fread_section_header and calling the
 *                              usual sc_scda_fread function. See also
 *                              the 'Encoding' section in the detailed
 *                              description in this file.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_scda_ferror_string or mapped to an error class
 *                              by \ref sc_scda_ferror_class.
 * \return                      Return a pointer to the input
 *                              context \b fc on success.
 *                              The context is used to continue
 *                              writing and eventually closing the file.
 *                              In case of any error, attempt to close the
 *                              file and deallocate the context \b fc.
 */
sc_scda_fcontext_t *sc_scda_fwrite_varray (sc_scda_fcontext_t * fc,
                                           const char *user_string,
                                           size_t *len,
                                           sc_array_t * array_data,
                                           sc_array_t * elem_counts,
                                           sc_array_t * elem_sizes,
                                           sc_array_t * proc_sizes,
                                           int indirect, int encode,
                                           sc_scda_ferror_t * errcode);

/** Open a file for reading and read the file header from the file.
 *
 * The file must exist and be at least of the size of the file header, i.e.
 * \ref SC_SCDA_HEADER_BYTES bytes. If the file has a file header that does not
 * satisfy the sc_scda file header format, the function reports the error using
 * SC_LERRORF, collectively close the file and deallocate the file context. In
 * this case the function returns NULL on all ranks. A wrong file header format
 * causes \ref SC_SCDA_FERR_FORMAT as \b errcode.
 * \note
 * All parameters are collective.
 *
 * This function differs from the one opening function for writing and reading
 * introduced in this \b scda
 * [preprint](https://doi.org/10.48550/arXiv.2307.06789).
 *
 * This function returns NULL on I/O errors.
 *
 * \param [in]     mpicomm   The MPI communicator that is used to open the
 *                           parallel file.
 * \param [in]     filename  Path to parallel file that is to be created or
 *                           to be opened.
 * \param [out]  user_string At least \ref SC_SCDA_USER_STRING_BYTES + 1 bytes.
 *                           \b user_string is filled with the read user string
 *                           from file and is nul-terminated.
 * \param [out]    len       On output \b len is set to the number of bytes
 *                           written to \b user_string excluding the terminating
 *                           nul.
 * \param [in]     opt       An options structure that provides the possibility
 *                           to pass further options. See \ref
 *                           sc_scda_fopen_options for more details.
 *                           It is valid to pass NULL for \b opt.
 * \param [out]    errcode   An errcode that can be interpreted by \ref
 *                           sc_scda_ferror_string or mapped to an error class
 *                           by \ref sc_scda_ferror_class.
 * \return                   Newly allocated context to continue reading
 *                           and eventually closing the file. NULL in
 *                           case of error, i.e. errcode != \ref
 *                           SC_SCDA_FERR_SUCCESS.
 */
sc_scda_fcontext_t *sc_scda_fopen_read (sc_MPI_Comm mpicomm,
                                        const char *filename,
                                        char *user_string, size_t *len,
                                        sc_scda_fopen_options_t * opt,
                                        sc_scda_ferror_t * errcode);

/** Read the next file section header.
 *
 * This is a collective function.
 * This functions reads the next file section header and provides the user
 * information on the subsequent file section that can be used to read the
 * actual data in a next calling depending on the file section type one
 * (or for a variable-size array two) functions out of
 * \ref sc_scda_fread_block_data, \ref sc_scda_fread_array_data,
 * \ref sc_scda_fread_varray_sizes and \ref sc_scda_fread_varray_data.
 * \note
 * All parameters are collective.
 *
 * This function returns NULL on I/O errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_scda_fopen_read.
 * \param [out]  user_string    At least \ref SC_SCDA_USER_STRING_BYTES +1 bytes.
 *                              \b user_string is filled with the read user
 *                              string from file and is nul-terminated.
 * \param [out]    len          On output \b len is set to the number of bytes
 *                              written to \b user_string excluding the
 *                              terminating nul.
 * \param [out]     type        On output this char is set to
 *                              'I' (inline data), 'B' (block of given size),
 *                              'A' (fixed-size array) or 'V' (variable-size
 *                              array) depending on the file section type.
 * \param [out]     elem_count  On output set to the global number of array
 *                              elements if \b type equals 'A' or 'V'. For
 *                              'I' and 'B' as \b type, \b elem_count is set 0.
 * \param [out]     elem_size   On output set to the byte count of the array
 *                              elements if \b type is 'A' and for the \b type
 *                              'B' the number of bytes. Otherwise set to 0.
 * \param [in,out]  decode      On input a Boolean to decide whether the file
 *                              section shall possibly be interpreted as a
 *                              compressed section, i.e. they were written by a
 *                              sc_scda_fwrite_* function with encode set to
 *                              true. For \b decode true as input the file
 *                              section is interpreted as a compressed file
 *                              section if the type and user string of the first
 *                              raw file section satisfiy the compression
 *                              convention. If the compression convention is not
 *                              satisfied the data is read raw. For false as
 *                              input the data will be read raw by the
 *                              subsequent sc_scda_fread_* calls. The output
 *                              value is always false if the input was set to
 *                              false. Otherwise, the output is a Boolean that
 *                              indicates if the nex file section contains a
 *                              file section type and user string matching the
 *                              compression convention. The subsequent
 *                              sc_scda_fread_* calls do not require any
 *                              adjustment dependent on \b decode. See also
 *                              the 'Encoding' section in the detailed
 *                              description in this file.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_scda_ferror_string or mapped to an error class
 *                              by \ref sc_scda_ferror_class.
 * \return                      Return a pointer to the input
 *                              context \b fc on success.
 *                              The context is used to continue
 *                              reading and eventually closing the file.
 *                              In case of any error, attempt to close the
 *                              file and deallocate the context \b fc.
 */
sc_scda_fcontext_t *sc_scda_fread_section_header (sc_scda_fcontext_t * fc,
                                                  char *user_string,
                                                  size_t *len, char *type,
                                                  size_t *elem_count,
                                                  size_t *elem_size,
                                                  int *decode,
                                                  sc_scda_ferror_t * errcode);

/** Read the data of an inline data section.
 *
 * This is a collective function.
 * This function is only valid to call directly after a successful call of \ref
 * sc_scda_fread_section_header.
 * \note
 * All parameters except of \b data are collective.
 *
 * This function returns NULL on I/O errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_scda_fopen_read.
 * \param [out]     data        Exactly 32 bytes on the rank \b root or NULL
 *                              on \b root to not read the bytes. The parameter
 *                              is ignored on all ranks unequal to \b root.
 * \param [in]      root        An integer between 0 and mpisize exclusive of
 *                              the MPI communicator that was used to create
 *                              \b fc. \b root indicates the MPI rank on that
 *                              \b data is read from the file.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_scda_ferror_string or mapped to an error class
 *                              by \ref sc_scda_ferror_class.
 * \return                      Return a pointer to the input
 *                              context \b fc on success.
 *                              The context is used to continue
 *                              reading and eventually closing the file.
 *                              In case of any error, attempt to close the
 *                              file and deallocate the context \b fc.
 */
sc_scda_fcontext_t *sc_scda_fread_inline_data (sc_scda_fcontext_t * fc,
                                               sc_array_t * data, int root,
                                               sc_scda_ferror_t * errcode);

/** Read the data of a block of given size.
 *
 * This is a collective function.
 * This function is only valid to call directly after a successful call of \ref
 * sc_scda_fread_section_header. This preceding call gives also the required
 * \b block_size.
 * \note
 * All parameters except of \b data_block are collective.
 *
 * This function returns NULL on I/O errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_scda_fopen_read.
 * \param [out]     block_data  A sc_array with element count 1 and element
 *                              size \b block_size. On output the sc_array is
 *                              filled with the block data of the read block data
 *                              section.
 * \param [in]      block_size  The number of bytes of the block as retrieved
 *                              from the preceding call of \ref
 *                              sc_scda_fread_section_header.
 * \param [in]      root        An integer between 0 and mpisize exclusive of
 *                              the MPI communicator that was used to create
 *                              \b fc. \b root indicates the MPI rank on that
 *                              \b block_data is read from the file.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_scda_ferror_string or mapped to an error class
 *                              by \ref sc_scda_ferror_class.
 * \return                      Return a pointer to the input
 *                              context \b fc on success.
 *                              The context is used to continue
 *                              reading and eventually closing the file.
 *                              In case of any error, attempt to close the
 *                              file and deallocate the context \b fc.
 */
sc_scda_fcontext_t *sc_scda_fread_block_data (sc_scda_fcontext_t * fc,
                                              sc_array_t * block_data,
                                              size_t block_size, int root,
                                              sc_scda_ferror_t * errcode);

/** Read the data of a fixed-size array.
 *
 * This is a collective function.
 * This function is only valid to call directly after a successful call of \ref
 * sc_scda_fread_section_header. This preceding call gives also the required
 * \b elem_size and the global number of array elements. The user must pass
 * a parallel partition of the array elements by \b elem_counts.
 * \note
 * All parameters except of \b array_data are collective.
 *
 * This function returns NULL on I/O errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_scda_fopen_read.
 * \param [out]     array_data  If \b indirect is false, a sc_array with
 *                              element count equals to the p-th entry of
 *                              \b elem_counts for p being the calling rank.
 *                              The element size must be equal to \b elem_size.
 *                              If \b indirect is true, a sc_array with the
 *                              same element count as for \b indirect false
 *                              but with sizeof (sc_array_t) as element size.
 *                              Each array element is then again a sc_array but
 *                              with element count 1 and element size
 *                              \b elem_size.
 *                              The data can be skipped on each process by
 *                              locally passing NULL.
 * \param [in]      elem_counts An sc_array that must be equal on all
 *                              ranks. The element count of \b elem_counts
 *                              must be the mpisize of the MPI communicator
 *                              that was used to create \b fc. The element size
 *                              of the sc_array must be equal to sizeof (\ref
 *                              sc_scda_ulong). The sc_array must contain the
 *                              local array elements counts. That is why it
 *                              induces the partition that is used to read the
 *                              array data in parallel. The sum of all array
 *                              elements must be equal to elem_count as
 *                              retrieved from \ref sc_scda_fread_section_header.
 * \param [in]      elem_size   The element size of one array element on number
 *                              of bytes. Must be the same on all ranks and as
 *                              retrieved from \ref sc_scda_fread_section_header.
 * \param [in]      indirect    A Boolean to determine whether \b array_data
 *                              must be a sc_array of sc_arrays to read
 *                              indirectly and in particular to potentially
 *                              non-contigous memory. See the documentation of
 *                              the parameter \b array_data for more information.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_scda_ferror_string or mapped to an error class
 *                              by \ref sc_scda_ferror_class.
 * \return                      Return a pointer to the input
 *                              context \b fc on success.
 *                              The context is used to continue
 *                              reading and eventually closing the file.
 *                              In case of any error, attempt to close the
 *                              file and deallocate the context \b fc.
 */
sc_scda_fcontext_t *sc_scda_fread_array_data (sc_scda_fcontext_t * fc,
                                              sc_array_t * array_data,
                                              sc_array_t * elem_counts,
                                              size_t elem_size,
                                              int indirect,
                                              sc_scda_ferror_t * errcode);

/** Read the element sizes of a variable-size array.
 *
 * This is a collective function.
 * This function is only valid to call directly after a successful call of \ref
 * sc_scda_fread_section_header. This preceding call gives also the for
 * \b elem_counts required global number of array elements. The user must pass
 * a parallel partition of the array elements by \b elem_counts.
 * \note
 * All parameters except of \b elem_sizes are collective.
 *
 * This function returns NULL on I/O errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_scda_fopen_read.
 * \param [out]     elem_sizes  A sc_array with element count equals to
 *                              p-th entry of \b elem_counts for p being the
 *                              calling rank. The element size must be
 *                              sizeof (\ref sc_scda_ulong). On output the array is
 *                              filled with the local array element byte counts,
 *                              where locality is determined by \b elem_counts.
 *                              The element sizes can be skipped on each process
 *                              by locally passing NULL.
 * \param [in]      elem_counts An sc_array that must be equal on all
 *                              ranks. The element count of \b elem_counts
 *                              must be the mpisize of the MPI communicator
 *                              that was used to create \b fc. The element size
 *                              of the sc_array must be equal to sizeof (\ref
 *                              sc_scda_ulong). The sc_array must contain the
 *                              local array elements counts. That is why it induces
 *                              the partition that is used to read the array
 *                              data in parallel. The sum of all array elements
 *                              must be equal to elem_count as retrieved from
 *                              \ref sc_scda_fread_section_header.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_scda_ferror_string or mapped to an error class
 *                              by \ref sc_scda_ferror_class.
 * \return                      Return a pointer to the input
 *                              context \b fc on success.
 *                              The context is used to continue
 *                              reading and eventually closing the file.
 *                              In case of any error, attempt to close the
 *                              file and deallocate the context \b fc.
 */
sc_scda_fcontext_t *sc_scda_fread_varray_sizes (sc_scda_fcontext_t * fc,
                                                sc_array_t * elem_sizes,
                                                sc_array_t * elem_counts,
                                                sc_scda_ferror_t * errcode);

/** Read the data of a variable-size array.
 *
 * This is a collective function.
 * This function is only valid to call directly after a successful call of \ref
 * sc_scda_fread_varray_sizes. This preceding call gives also the required
 * \b elem_sizes.
 * \note
 * All parameters except of \b array_data and \b elem_sizes are collective.
 *
 * This function returns NULL on I/O errors.
 *
 * \param [in,out]  fc          File context previously opened by \ref
 *                              sc_scda_fopen_read.
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
 *                              The data can be skipped on each process by
 *                              locally passing NULL.
 * \param [in]    elem_counts   An sc_array that must be equal on all
 *                              ranks. The element count of \b elem_counts
 *                              must be the mpisize of the MPI communicator
 *                              that was used to create \b fc. The element size
 *                              of the sc_array must be equal to sizeof (\ref
 *                              sc_scda_ulong). The sc_array must contain the
 *                              local array elements counts. That is why it
 *                              induces the partition that is used to read the
 *                              array data in parallel. The sum of all array
 *                              elements must be equal to elem_count as retrieved
 *                              from \ref sc_scda_fread_section_header.
 * \param [in]    elem_sizes    The local element sizes conforming to the array
 *                              element partition \b elem_counts as retrieved
 *                              from \ref sc_scda_fread_varray_sizes.
 *                              This parameter is ignored for ranks to which
 *                              NULL for \b array_data was passed.
 * \param [in]      proc_sizes  An sc_array that must be equal on all
 *                              ranks. The element count and element size
 *                              must be the same as for \b elem_counts. The
 *                              array must contain the overall byte count per
 *                              rank conforming with the passed array element
 *                              partition \b elem_counts and the local array
 *                              element sizes in \b elem_sizes. This parameter
 *                              can be computed using \ref sc_scda_proc_sizes.
 * \param [in]      indirect    A Boolean to determine whether \b array_data
 *                              must be a sc_array of sc_arrays to read
 *                              indirectly and in particular to potentially
 *                              non-contigous memory. See the documentation of
 *                              the parameter \b array_data for more information.
 * \param [out]     errcode     An errcode that can be interpreted by \ref
 *                              sc_scda_ferror_string or mapped to an error class
 *                              by \ref sc_scda_ferror_class.
 * \return                      Return a pointer to the input
 *                              context \b fc on success.
 *                              The context is used to continue
 *                              reading and eventually closing the file.
 *                              In case of any error, attempt to close the
 *                              file and deallocate the context \b fc.
 */
sc_scda_fcontext_t *sc_scda_fread_varray_data (sc_scda_fcontext_t * fc,
                                               sc_array_t * array_data,
                                               sc_array_t * elem_counts,
                                               sc_array_t * elem_sizes,
                                               sc_array_t * proc_sizes,
                                               int indirect,
                                               sc_scda_ferror_t * errcode);

/** Translate a sc_scda error code to an error class.
 *
 * The semantic of error class and error code is the same as in the MPI standard,
 * i.e. all error classes are error codes but potentially there are more error
 * codes than error classes.
 *
 * If \b errcode is already an error class, \b errclass if filled with
 * \b errcode.
 *
 * This is a non-collective function.
 *
 * \param [in]    errcode       An errcode that is output by a sc_scda function.
 * \param [out]   errclass      On output filled with the error class that
 *                              corresponds to the given \b errcode.
 *                              See the function description above for more
 *                              information on error classes and error codes
 *                              in scda.
 * \return                      \ref SC_SCDA_FERR_SUCCESS on success or
 *                              something else on invalid arguments.
 */
int                 sc_scda_ferror_class (sc_scda_ferror_t errcode,
                                          sc_scda_ferror_t *errclass);

/** Check if a scda_errorcode_t encodes success.
 *
 * \param [in]    errcode       An errcode that is output by a sc_scda function.
 * \return                      1 if \b errcode encodes success and 0 otherwise.
 */
int sc_scda_is_success (const sc_scda_ferror_t * errorcode);

/** Translate a sc_scda error code/class to an error string.
 *
 * This is a non-collective function.
 *
 * \param [in]    errcode       An errcode that is output by a sc_scda function.
 * \param [out]   str           At least sc_MPI_MAX_ERROR_STRING bytes.
 * \param [out]   len           On output the length of string on return.
 * \return                      \ref SC_SCDA_FERR_SUCCESS on success or
 *                              something else on invalid arguments.
 */
int                 sc_scda_ferror_string (sc_scda_ferror_t errcode, char *str,
                                           int *len);

/** Close a file opened for parallel write/read and the free the file context.
 *
 * This is a collective function.
 * Every call of \ref sc_scda_fopen_write and \ref sc_scda_fopen_read must be
 * matched by a corresponding call of \ref sc_scda_fclose on the created file
 * context.
 * \note
 * All parameters are collective.
 *
 * This function returns NULL on I/O errors.
 *
 * \param [in,out]  fc        File context previously created by
 *                            \ref sc_scda_fopen_write or \ref
 *                            sc_scda_fopen_read. This file context is freed
 *                            after a call of this function.
 * \param [out]     errcode   An errcode that can be interpreted by \ref
 *                            sc_scda_ferror_string or mapped to an error class
 *                            by \ref sc_scda_ferror_class.
 * \return                    \ref SC_SCDA_FERR_SUCCESS for a successful call
 *                            and -1 in case a of an error.
 *                            See also \b errcode argument.
 */
int                 sc_scda_fclose (sc_scda_fcontext_t * fc,
                                    sc_scda_ferror_t * errcode);

SC_EXTERN_C_END;

#endif /* SC_SCDA_H */
