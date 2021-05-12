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

/** \file sc3_array.h \ingroup sc3
 * We provide an array of a variable count of same-size items.
 *
 * The array container stores any number of fixed-size items.
 * The length of the array (allocated number of items) can be changed.
 * If needed, memory is reallocated internally.
 * The array can serve as a push-and-pop stack.
 *
 * We use standard `int` types for indexing.
 * The total array memory size may be greater than 2GB depending on the
 * size of each element.
 *
 * In the setup phase, we set the size of the array and optionally
 * an initial count, an initzero property, and some more.
 * The caller may also decide during setup if the array is resizable.
 * After setup, the array may be resized while it is resizable.
 * Resizability can be terminated by calling \ref sc3_array_freeze
 * (only after \ref sc3_array_setup).  The freeze is not reversible.
 *
 * An array can only be refd if it is setup and non-resizable.
 * Otherwise, the usual ref-, unref- and destroy semantics hold.
 */

#ifndef SC3_ARRAY_H
#define SC3_ARRAY_H

#include <sc3_error.h>

/** The array container is an opaque struct. */
typedef struct sc3_array sc3_array_t;

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** Query whether an array is not NULL and internally consistent.
 * The array may be valid in both its setup and usage phases.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer is not NULL and array consistent.
 */
int                 sc3_array_is_valid (const sc3_array_t * a, char *reason);

/** Query whether an array is not NULL, consistent and not setup.
 * This means that the array is not (yet) in its usage phase.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL, array consistent, not setup.
 */
int                 sc3_array_is_new (const sc3_array_t * a, char *reason);

/** Query whether an array is not NULL, internally consistent and setup.
 * This means that the array is in its usage phase (resizable or not).
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL, array consistent and setup.
 */
int                 sc3_array_is_setup (const sc3_array_t * a, char *reason);

/** Query whether an array is setup and resizable.
 * A resizable array becomes non-resizable by \ref sc3_array_freeze.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL, array setup and resizable.
 */
int                 sc3_array_is_resizable (const sc3_array_t * a,
                                            char *reason);

/** Query whether an array is setup and not (or no longer) resizable.
 * A resizable array becomes non-resizable by \ref sc3_array_freeze.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff array not NULL, setup, and not resizable.
 */
int                 sc3_array_is_unresizable (const sc3_array_t * a,
                                              char *reason);

/** Create a new array object in its setup phase.
 * It begins with default parameters that can be overridden explicitly.
 * Setting and modifying parameters is only allowed in the setup phase.
 * Call \ref sc3_array_setup to change the array into its usage phase.
 * After that, no more parameters may be set.
 * The defaults are documented in the sc3_array_set_* calls.
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
 * This may be useful to grow an array avoiding frequent reallocation.
 * \param [in,out] a    The array must not be setup.
 * \param [in] ealloc   Minimum number of elements initially allocated.
 *                      Legal if this is smaller than the initial count,
 *                      in which case we ignore it.
 *                      Must be non-negative; default is 8.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_set_elem_alloc (sc3_array_t * a, int ealloc);

/** Set the initzero property of an array.
 * If set to true, array memory for initial count is zeroed during setup.
 * This does *not* mean that new space after resize will be initialized.
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
 * If set to true, the array memory is shrunk occasionally on
 * \ref sc3_array_resize and \ref sc3_array_freeze.
 * \param [in,out] a        The array must not be setup.
 * \param [in] tighten      Boolean; default is false.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_set_tighten (sc3_array_t * a, int tighten);

/** Setup an array and change it into its usable phase.
 * Whether it is resizable depends on calling \ref sc3_array_set_resizable.
 * \param [in,out] a    This array must not yet be setup.
 *                      Internal storage is allocated, the setup phase ends,
 *                      and the array is put into its usable phase.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_setup (sc3_array_t * a);

/** Increase the reference count on an array by 1.
 * This is only allowed after the array has been setup.  The array must not
 * be resizable, by initialization or by calling \ref sc3_array_freeze.
 * \param [in,out] a    Must not be resizable.  Its refcount is increased.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_ref (sc3_array_t * a);

/** Decrease the reference count on an array by one.
 * If the reference count drops to zero, the array is deallocated.
 * \param [in,out] ap   The pointer must not be NULL and the array valid.
 *                      Its refcount is decreased.  If it reaches zero,
 *                      the array is destroyed and the value set to NULL.
 * \return              NULL on success, error object otherwise.
 *                      We return a leak if we find one.
 */
sc3_error_t        *sc3_array_unref (sc3_array_t ** ap);

/** Destroy an array with a reference count of one.
 * It is a leak error to destroy an array that is multiply referenced.
 * We unref its internal allocator, which may cause a leak error if that
 * allocator has been used against specification elsewhere in the code.
 * \param [in,out] ap   This array must be valid and have a refcount of 1.
 *                      On output, value is set to NULL.
 * \return              NULL on success, error object otherwise.
 *                      When the array had more than one reference,
 *                      return an error of kind \ref SC3_ERROR_LEAK.
 */
sc3_error_t        *sc3_array_destroy (sc3_array_t ** ap);

/** Resize an array, reallocating internally as needed.
 * The array elements are preserved to the minimum of old and new counts.
 * \param [in,out] a        The array must be resizable.
 * \param [in] new_ecount   The new element count.  0 is legal.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_resize (sc3_array_t * a, int new_ecount);

/** Enlarge an array by a number of elements.
 * The output points to the beginning of the memory for the new elements.
 * \param [in,out] a    The array must be resizable.
 * \param [in] n        Non-negative number.  If n == 0, do nothing.
 * \param [out] ptr     Address of pointer.
 *                      On output set to array element at previously last index,
 *                      or NULL if n == 0, which is explicitly allowed.
 *                      This argument may be NULL for no assignment.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_push_count (sc3_array_t * a, int n, void *ptr);

/** Enlarge an array by one element.
 * The output points to the beginning of the memory for the new element.
 * Equivalent to \ref sc3_array_push_count (a, 1, ptr).
 * \param [in,out] a    The array must be resizable.
 * \param [out] ptr     Address of pointer.
 *                      On output set to array element at previously last index.
 *                      This argument may be NULL for no assignment.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_push (sc3_array_t * a, void *ptr);

/** Reduce array size by one element.
 * \param [in,out] a    The array must be resizable and have > 0 elements.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_pop (sc3_array_t * a);

/** Set array to non-resizable after it has been setup.
 * If the array is initially resizable, this call is required to allow refing.
 * \param [in,out] a    The array must be setup.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_freeze (sc3_array_t * a);

/** Index an array element.
 * \param [in] a        The array must be setup.
 * \param [in] i        Index must be in [0, element count).
 * \param [out] ptr     Address of a pointer.
 *                      Assigned address of array element at index \b i.
 *                      If the array element size is zero, the pointer
 *                      must not be dereferenced.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_index (sc3_array_t * a, int i, void *ptr);

/** Index an array element without returning an error object.
 * \param [in] a        The array must be setup.
 * \param [in] i        Index must be in [0, element count).
 * \return              Address of array element at index \b i.
 *                      If the array element size is zero, the pointer
 *                      must not be dereferenced.
 *                      With --enable-debug, returns NULL if index
 *                      is out of bounds or the array is not setup.
 *                      Without, the program may crash here or later.
 */
void               *sc3_array_index_noerr (const sc3_array_t * a, int i);

/** Create a subarray sa pointing to the same memory as an array a.
 * \param [in] a        The array must be setup.
 * \param [in] idx      Index of the array a that points to a beginning of
 *                      the subarray sa.
 * \param [in, out] sa  The array must not be setup. Size of elements must
 *                      be equal to elements in a. The number of elements
 *                      must be less of equal than (a->ecount - idx).
 *                      This subarray will contain all the memory of
 *                      the array a corresponding indices
 *                      [idx, sa->ecount + idx).
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_subarray (sc3_array_t * a, int idx,
                                        sc3_array_t * sa);

/** Create an array view sa pointing to the allocated memory fragment a.
 * Warning! Potentially dangerous function. Make sure, that array's length
 * adjusted for idx shift is covered by the length of the allocated fragment.
 * \param [in] a        The allocated memory fragment.
 * \param [in] idx      Index pointing to allocated memory and is considered
 *                      to be the beginning of the subarray sa.
 * \param [in] n        The number of allocated bits.
 * \param [in, out] sa  The array must not be setup. The number of elements
 *                      as->ecount must be such that
 *                      (as->ecount + idx) * as->esize <= |alloc memory of a|.
 *                      (Sub)array sa will contain all the memory of
 *                      a corresponding indices [idx, sa->ecount + idx).
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_view (char * a, int idx, int n,
                                    sc3_array_t * sa);

/** Return element size of an array that is setup.
 * \param [in] a        Array must be setup.
 * \param [out] esize   Element size of the array in bytes, or 0 on error.
 *                      Pointer must not be NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_get_elem_size (sc3_array_t * a, size_t *esize);

/** Return element count of an array that is setup.
 * \param [in] a        Array must be setup.
 * \param [out] ecount  Element count of the array, or 0 on error.
 *                      Pointer must not be NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_get_elem_count (sc3_array_t * a, int *ecount);

/** Return the array's element count without creating error objects.
 * \param [in] a        The array must be setup.  Otherwise, return 0
 *                      with --enable-debug, or junk or crash without.
 * \return              The array's element count.
 */
int                 sc3_array_elem_count_noerr (const sc3_array_t * a);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_ARRAY_H */
