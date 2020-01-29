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

typedef struct sc3_array sc3_array_t;

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** Check whether an array is not NULL and internally consistent.
 * The array may be valid in both its setup and usage phases.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer is not NULL and array consistent.
 */
int                 sc3_array_is_valid (sc3_array_t * a, char *reason);

/** Check whether an array is not NULL, consistent and not setup.
 * This means that the array is not in its usage phase.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL, array consistent, not setup.
 */
int                 sc3_array_is_new (sc3_array_t * a, char *reason);

/** Check whether an array is not NULL, internally consistent and setup.
 * This means that the array is in its usage phase.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL, array consistent and setup.
 */
int                 sc3_array_is_setup (sc3_array_t * a, char *reason);

/** Check whether an array is setup and resizable.
 * A resizable array becomes non-resizable by \ref sc3_array_freeze.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL, array setup and resizable.
 */
int                 sc3_array_is_resizable (sc3_array_t * a, char *reason);

/** Create a new array object in its setup phase.
 * It begins with default parameters that can be overridden explicitly.
 * Setting and modifying parameters is only allowed in the setup phase.
 * Call \ref sc3_array_setup to change the array into its usage phase.
 * After that, no more parameters may be set.
 * \param [in,out] aator    An allocator that is setup.
 *                          The allocator is refd and remembered internally
 *                          and will be unrefd on array destruction.
 * \param [out] ap      Pointer must not be NULL.
 *                      If the function returns an error, value set to NULL.
 *                      Otherwise, value set to an array with default values.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_new (sc3_allocator_t * aator,
                                   sc3_array_t ** ap);

/** Set the size of each array element in bytes.
 * \param [in,out] a    The array must not be setup.
 * \param [in] esize    Element size in bytes.  Zero is legal, one the default.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_set_elem_size (sc3_array_t * a, size_t esize);

/** Set the initial number of array elements.
 * \param [in,out] a    The array must not be setup.
 * \param [in] ecount   Element count on setup.  Zero is legal and default.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_set_elem_count (sc3_array_t * a, int ecount);

/** Set the minimum required number of array elements to allocate on setup.
 * \param [in,out] a    The array must not be setup.
 * \param [in] ealloc   Minimum number of elements initially allocated.
 *                      Legal if this is smaller than the initial count.
 *                      Must be non-negative; default is 8.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_set_elem_alloc (sc3_array_t * a, int ealloc);

/** Set the initzero property of an array.
 * If set to true, array memory for initial count is zeroed during setup.
 * This does *not* mean that new space after resize is initialized.
 * \param [in,out] a        The array must not be setup.
 * \param [in] initzero     Boolean; default is false.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_set_initzero (sc3_array_t * a, int initzero);

/** Set the resizable property of an array.
 * It determines whether the array may be resized after setup.
 * \param [in,out] a        The array must not be setup.
 * \param [in] resizable    Boolean; default is false.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_set_resizable (sc3_array_t * a, int resizable);

/** Set the tighten property of an array.
 * If set to true, the array memory is shrunk on resize and freeze.
 * \param [in,out] a        The array must not be setup.
 * \param [in] initzero     Boolean; default is false.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_set_tighten (sc3_array_t * a, int tighten);

/** Setup an array and put it into its usable phase.
 * \param [in,out] a    This array must not yet be setup.
 *                      Internal storage is allocated, the setup phase ends,
 *                      and the array is put into its usable phase.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_setup (sc3_array_t * a);

/** Increase the reference count on an array by 1.
 * This is only allowed after the array has been setup.  The array must not
 * be resizable, by initialization or by calling \ref sc3_array_freeze.
 * \param [in,out] a    This array must be setup.  Its refcount is increased.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_ref (sc3_array_t * a);

/** Decrease the reference count on an array by 1.
 * If the reference count drops to zero, the allocator is deallocated.
 * \param [in,out] ap   The pointer must not be NULL and the array valid.
 *                      Its refcount is decreased.  If it reaches zero,
 *                      the array is destroyed and the value set to NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_unref (sc3_array_t ** ap);

/** Destroy an array with a reference count of 1.
 * It is an error to destroy an array that is multiply refd.
 * \param [in,out] ap   This array must be valid and have a refcount of 1.
 *                      On output, value is set to NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_destroy (sc3_array_t ** ap);

/** Resize an array, reallocating internally as needed.
 * The array elements are preserved to the minimum of old and new counts.
 * \param [in] a            The array must be resizable.
 * \parma [in] new_ecount   The new element count.  0 is legal.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_resize (sc3_array_t * a, int new_ecount);

/** Enlarge an array by a number of elements and provide a pointer.
 * \param [in] a        The array must be resizable.
 * \param [in] n        Non-negative number.  If n == 0, do nothing.
 * \param [out] p       Address of array element at previously last index,
 *                      or NULL if n == 0, which is explicitly allowed.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_push_count (sc3_array_t * a, int n, void **p);

/** Enlarge an array by one element and provide a pointer to it.
 * \param [in] a        The array must be resizable.
 * \param [out] p       Address of array element at previously last index.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_push (sc3_array_t * a, void **p);

/** Enlarge an array by one element without returning an error object.
 * \param [in] a        The array must be resizable.
 * \return              Address of array element at previously last index.
 *                      With SC_ENABLE_DEBUG, returns NULL if the array
 *                      is not resizable or the allocation fails.
 *                      Otherwise it may crash on error.
 */
void               *sc3_array_push_noerr (sc3_array_t * a);

/** Set array to non-resizable after it has been setup.
 * \param [in] a        The array must be setup.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_freeze (sc3_array_t * a);

/** Index an array element.
 * \param [in] a        The array must be setup.
 * \param [in] i        Index must be in [0, element count).
 * \param [out] p       Address of array element at index \b i.
 *                      If the array element size is zero, the pointer
 *                      must not be dereferenced.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_index (sc3_array_t * a, int i, void **p);

/** Index an array element without returning an error object.
 * \param [in] a        The array must be setup.
 * \param [in] i        Index must be in [0, element count).
 * \return              Address of array element at index \b i.
 *                      If the array element size is zero, the pointer
 *                      must not be dereferenced.
 *                      With SC_ENABLE_DEBUG, returns NULL if index
 *                      is out of bounds or the array is not setup.
 *                      Otherwise it may crash on error.
 */
void               *sc3_array_index_noerr (sc3_array_t * a, int i);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_ARRAY_H */
