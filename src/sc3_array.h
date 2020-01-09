/*
  This file is part of the SC Library, version 3.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

/** \file sc3_array.h
 */

#ifndef SC3_ARRAY_H
#define SC3_ARRAY_H

#include <sc3_error.h>

typedef struct sc3_array_args sc3_array_args_t;
typedef struct sc3_array sc3_array_t;

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** Create a new array argument object.
 * \param [in] aator    A valid allocator.
 *                      The allocator is refd and remembered internally
 *                      and will be unrefd on destruction.
 * \param [out] aap     Pointer must not be NULL.
 *                      If the function returns an error, value set to NULL.
 *                      Otherwise, value set to arguments with default values.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_array_args_new (sc3_allocator_t * aator,
                                        sc3_array_args_t ** aap);

/** Destroy an array argument object and unrefs its allocator.
 * \param [in,out] aap  Pointer and value must not be NULL.
 *                      Value is set to NULL.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_array_args_destroy (sc3_array_args_t ** aap);

sc3_error_t        *sc3_array_args_set_elem_size (sc3_array_args_t * aa,
                                                  size_t esize);
sc3_error_t        *sc3_array_args_set_elem_count (sc3_array_args_t * aa,
                                                   size_t ecount);
sc3_error_t        *sc3_array_args_set_elem_alloc (sc3_array_args_t * aa,
                                                   size_t ealloc);
sc3_error_t        *sc3_array_args_set_resizable (sc3_array_args_t * aa,
                                                  int resizable);

/** Check whether an array is not NULL and internally consistent.
 * \param [in] a        Any pointer to an array.
 * \return              True iff pointer is not NULL and array consistent.
 */
int                 sc3_array_is_valid (sc3_array_t * a);

/** Creates a new array from arguments.
 * \param [in,out] aap  Properly initialized array arguments.
 *                      Its allocator is passed into the output array.
 *                      We call \ref sc3_array_args_destroy on the arguments.
 *                      Their value is set to NULL on output.
 * \param [out] ap      On output, the initialized array with refcount 1
 *                      unless there is an internal error, then set to NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_new (sc3_array_args_t ** aap,
                                   sc3_array_t ** ap);

/** Increase the reference count on an array by 1.
 * \param [in] a        This array must be valid.  Its refcount is increased.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_ref (sc3_array_t * a);

/** Decrease the reference count on an array by 1.
 * \param [in] ap       The pointer must not be NULL and the array valid.
 *                      Its refcount is decreased.  If it reaches zero,
 *                      the array is destroyed and the value set to NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_unref (sc3_array_t ** ap);

/** Destroy an array with a reference count of 1.
 * \param [in,out] ap   This array must be valid and have a refcount of 1.
 *                      On output, value is set to NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_destroy (sc3_array_t ** ap);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_ARRAY_H */
