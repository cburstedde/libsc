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
 *
 * The array container stores any number of fixed-size elements.
 * The length of the array (accessible number of elements) can be changed.
 * If needed, memory is reallocated internally.
 * The array can serve as a push-and-pop stack.
 *
 * We use standard `int` types for indexing.
 * The total array memory size may be greater than 2 GB depending
 * on the size of each element.
 *
 * In the setup phase, we set the size of the array and optionally
 * an initial count, an initzero property, and some more.
 * The caller may also decide during setup if the array is resizable.
 * After setup, the array may be resized while it is resizable.
 * Resizability can be terminated by calling \ref sc3_array_freeze
 * (only after \ref sc3_array_setup).  The freeze is not reversible.
 * Freeze only applies to the length of the array, not its contents.
 * Indexing into the current length of the array is always possible.
 * Indexing allows for both read and write access.
 *
 * We allow for arrays to be views on other arrays or plain data.
 * This provides for one unified way of indexing into any memory.
 * Views can be renewed quickly to optimize for this common case.
 *
 * The usual ref-, unref- and destroy semantics hold, except
 * an array can only be refd if it is setup and non-resizable.
 */

#ifndef SC3_ARRAY_H
#define SC3_ARRAY_H

#include <sc3_alloc.h>

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
 * This applies to both arrays and views right after being setup.
 * Resizable arrays can be passed to \ref sc3_array_resize, while
 * resizable array views can be passed to \ref sc3_array_renew_view
 * and resizable data views can be passed to \ref sc3_array_renew_data.
 * Any resizable array becomes non-resizable by \ref sc3_array_freeze.
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

/** Query whether an array holds allocated memory, thus is not a view.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff array not NULL and a view onto an array.
 */
int                 sc3_array_is_alloced (const sc3_array_t * a,
                                          char *reason);

/** Query whether an array is a view on another array.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff array not NULL and a view onto an array.
 */
int                 sc3_array_is_view (const sc3_array_t * a, char *reason);

/** Query whether an array is a view on plain data.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff array not NULL and a view onto plain data.
 */
int                 sc3_array_is_data (const sc3_array_t * a, char *reason);

/** Check whether an array is sorted non-descending wrt. a comparison function.
 * \param [in] a        Any pointer.  For a true result must exist and be setup.
 * \param [in] compar   The comparison function to be used as with qsort (3).
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if array is sorted and no errors occur,
 *                      false otherwise.
 */
int                 sc3_array_is_sorted (const sc3_array_t * a,
                                         int (*compar) (const void *,
                                                        const void *),
                                         char *reason);

#ifndef SC_HAVE_BSD_QSORT_R

/** Check whether an array is sorted non-descending wrt. a comparison function.
 * \param [in] a        Any pointer.  For a true result must exist and be setup.
 * \param [in] compar   The comparison function to be used as with qsort_r.
 * \param [in] user     A pointer to arbitrary user data that is passed to
 *                      the comparison function \a compar for context.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if array is sorted and no errors occur,
 *                      false otherwise.  False also if defined neither
 *                      SC_HAVE_BSD_QSORT_R nor S_HAVE_GNU_QSORT_R.
 */
int                 sc3_array_is_sorted_r (const sc3_array_t * a,
                                           int (*compar) (const void *,
                                                          const void *,
                                                          void *),
                                           void *user, char *reason);

#else

int                 sc3_array_is_sorted_r (const sc3_array_t * a,
                                           int (*compar) (void *,
                                                          const void *,
                                                          const void *),
                                           void *user, char *reason);

#endif /* SC_HAVE_BSD_QSORT_R */

/** Create a new array object in its setup phase.
 * It begins with default parameters that can be overridden explicitly.
 * Setting and modifying parameters is only allowed in the setup phase.
 * Call \ref sc3_array_setup to change the array into its usage phase.
 * After that, no more parameters may be set.
 * The defaults are documented in the sc3_array_set_* calls.
 * \param [in,out] alloc    An allocator that is setup, or NULL.
 *                          If NULL, we use \ref sc3_allocator_new_static.
 *                          The allocator is refd and remembered internally
 *                          and will be unrefd on array destruction.
 * \param [out] ap      Pointer must not be NULL.
 *                      If the function returns an error, value set to NULL.
 *                      Otherwise, value set to an array with default values.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_new (sc3_allocator_t * alloc,
                                   sc3_array_t ** ap);

/** Set the size of each array element in bytes.
 * Default is 1.
 * \param [in,out] a    The array must not be setup.
 * \param [in] esize    Element size in bytes.  Zero is legal, one the default.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_set_elem_size (sc3_array_t * a, size_t esize);

/** Set the initial number of array elements.
 * Default is 0.
 * \param [in,out] a    The array must not be setup.
 * \param [in] ecount   Element count on setup.  Zero is legal and default.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_set_elem_count (sc3_array_t * a, int ecount);

/** Set the minimum number of array elements to pre-allocate on setup.
 * This may be useful to grow an array avoiding frequent reallocation.
 * Default is a small reasonable number.
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
 * Default is false.
 * \param [in,out] a        The array must not be setup.
 * \param [in] initzero     Boolean; default is false.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_set_initzero (sc3_array_t * a, int initzero);

/** Set the resizable property of an array.
 * It determines whether the array may be resized after setup.
 * Default is true.
 * \param [in,out] a        The array must not be setup.
 * \param [in] resizable    Boolean; default is true.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_set_resizable (sc3_array_t * a, int resizable);

/** Set the tightening property of an array.
 * Default is false.
 * If set to true, the internal array memory is shrunk, occasionally on
 * \ref sc3_array_resize, and on \ref sc3_array_freeze.
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
 * This means that views can only be refd after freezing them.
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
 */
sc3_error_t        *sc3_array_unref (sc3_array_t ** ap);

/** Destroy an array with a reference count of one.
 * It is a fatal error to destroy an array that is multiply referenced.
 * We unref its internal allocator, which may cause a leak error if that
 * allocator has one reference and live allocations elsewhere in the code,
 * which can only occur when mismanaging the allocator in unrelated code.
 * \param [in,out] ap   This array must be valid and have a refcount of 1.
 *                      On output, value is set to NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_destroy (sc3_array_t ** ap);

/** Resize an array, reallocating internally as needed.
 * Only allocated arrays may be resized, views may not.
 * The array elements are preserved to the minimum of old and new counts.
 * Their addresses may change, however, as with standard realloc (3).
 * \param [in,out] a        The array must be resizable.
 * \param [in] new_ecount   The new element count.  0 is legal.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_resize (sc3_array_t * a, int new_ecount);

/** Sort the elements of an array with qsort (3).
 * The comparison callback does not allow for error return.
 * A possible solution is longjmp (3) with one jump buffer per thread.
 * \param [in,out] a    Array must be setup.
 * \param [in] compar   Comparison function as with qsort (3).
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_sort (sc3_array_t * a,
                                    int (*compar) (const void *,
                                                   const void *));

#ifndef SC_HAVE_BSD_QSORT_R

/** Sort the elements of an array with qsort_r (3).
 * The comparison callback does not allow for error return.
 * However, it is passed context data that may be used directly for
 * this purpose or combined with a thread-local longjmp (3) buffer.
 * \param [in,out] a    Array must be setup.
 * \param [in] compar   Comparison function as with qsort (3).
 *                      There are two differing conventions regarding
 *                      its parameters.  We document the GNU version.
 * \param [in] data     Passed on to the \a compar function.
 * \return              NULL on success, error object otherwise.
 *                      Returns fatal error if defined neither
 *                      SC_HAVE_BSD_QSORT_R nor S_HAVE_GNU_QSORT_R.
 */
sc3_error_t        *sc3_array_sort_r (sc3_array_t * a,
                                      int (*compar) (const void *,
                                                     const void *,
                                                     void * data),
                                      void *data);

#else

sc3_error_t        *sc3_array_sort_r (sc3_array_t * a,
                                      int (*compar) (void * data,
                                                     const void *,
                                                     const void *),
                                      void *data);

#endif /* SC_HAVE_BSD_QSORT_R */

/** Binary search in a sorted array with bsearch (3).
 * The comparison callback does not allow for error return.
 * A possible solution is longjmp (3) with one jump buffer per thread.
 * \param [in] a        Array must be setup.
 * \param [in] key      Pointer to array contents searched.  Not NULL.
 * \param [in] compar   Comparison function as wiith bsearch (3);
 * \param [out] ptr     Pointer to address must not be NULL.
 *                      Value set to address of found object inside
 *                      the array, or NULL if no match found.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_bsearch (sc3_array_t * a, const void *key,
                                       int (*compar) (const void *,
                                                      const void *),
                                       void *ptr);

/** Function to determine the enumerable type of an object in an array.
 * \param [in] array   Array containing the object.
 * \param [in] index   The location of the object.
 * \param [in] data    Arbitrary user data.
 * \param [out] type   Returned enumerable type of an object.
 * \return             NULL on success, error object otherwise.
 */
typedef sc3_error_t *(*sc3_array_type_t) (sc3_array_t * array,
                                          int index, void *data, int *type);

/** Compute the offsets of groups of enumerable types in an array.
 * \param [in] a             Array that is sorted in ascending order by type.
 *                           If k indexes \a a, then
 *                           0 <= \a type_fn (\a a, k, \a data) <
 *                           \a num_types.
 * \param [in,out] offsets   An initialized a of type size_t that is
 *                           resized to \a num_types + 1 entries.  The indices
 *                           j of \a a that contain objects of type k are
 *                           \a offsets[k] <= j < \a offsets[k + 1].
 *                           If there are no objects of type k, then
 *                           \a offsets[k] = \a offset[k + 1].
 * \param [in] num_types     The number of possible types of objects in
 *                           \a a.
 * \param [in] type_fn       Returns the type of an object in the a.
 * \param [in] data          Arbitrary user data passed to \a type_fn.
 * \return                   NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_split (sc3_array_t * a, sc3_array_t * offsets,
                                     int num_types, sc3_array_type_t type_fn,
                                     void *data);

/** Enlarge an array by a number of elements.
 * The array must be resizable.
 * The output points to the beginning of the memory for the new elements.
 * The pointer to the array element is passed by a reference argument.
 * \param [in,out] a    The array must be setup and resizable.
 * \param [in] n        Non-negative number.  If 0, do nothing.
 * \param [out] ptr     Optional address of pointer.
 *                      On output the array element at previously last index,
 *                      or NULL if n is zero.
 *                      This argument may be NULL for no assignment.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_push_count (sc3_array_t * a, int n, void *ptr);

/** Enlarge an array by one element.
 * The array must be resizable.
 * The output points to the beginning of the memory for the new element.
 * Equivalent to \ref sc3_array_push_count (a, 1, ptr).
 * The pointer to the array element is passed by a reference argument.
 * \param [in,out] a    The array must be setup and resizable.  Enlarged by 1.
 * \param [out] ptr     Optional address of pointer.
 *                      On output the array element at previously last index.
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
 * If the array is not resizable already, do nothing.
 * If the array is initially resizable, this call is required to allow refing.
 * This call is not reversible.
 * \param [in,out] a    The array must be setup.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_freeze (sc3_array_t * a);

/** Index an array element.
 * The pointer to the array element is passed by a reference argument.
 * \param [in] a        The array must be setup.
 * \param [in] i        Index must be in [0, element count).
 * \param [out] ptr     Address of a pointer.
 *                      Assigned address of array element at index \a i.
 *                      The pointer may be used for reading and writing.
 *                      Not to be dereferenced when element size is 0.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_index (sc3_array_t * a, int i, void *ptr);

/** Index an array element for reading without returning an error object.
 *
 * With --enable-debug, \ref sc3_array_index checks for sanity and bounds.
 * Without --enable-debug, \ref sc3_array_index is a fast function and it
 * always returns NULL.  In this case, checking its return value is redundant.
 * Only if this check, usually effected by the \ref SC3E macro, would make a
 * measurable difference in performance, it might be advised to consider
 * using this replacement function.
 *
 * This function is optimized for speed, not safety, thus risky.
 * Using it with a non-setup array or wrong indexing is undefined.
 * The idea is not to verify its return pointer at all.
 *
 * \param [in] a        Const pointer to the array, must be setup.
 * \param [in] i        Index must be in [0, element count).
 * \return              Address of array element at index \a i.
 *                      When configured with --enable-debug, returns NULL
 *                      if index is out of bounds, element size is zero,
 *                      or the array is not setup.  Without --enable-debug,
 *                      passing a non-setup array or invalid indices is
 *                      undefined, and a segmentation fault is likely.
 */
const void         *sc3_array_index_noerr (const sc3_array_t * a, int i);

/** Create a view array pointing into the same memory as a given array.
 * Inherit the data element size from the given array.  The view refs the
 * viewed array, which must thus not be destroyed before this view.
 * The view starts out as resizable, permitting future calls to \ref
 * sc3_array_renew_view as long as \ref sc3_array_freeze is not called.
 * This does not mean that \ref sc3_array_resize would be legal on a view.
 * \param [in,out] alloc    An allocator that is setup, or NULL.
 *                          If NULL, we use \ref sc3_allocator_new_static.
 *                          The allocator is refd and remembered internally
 *                          and will be unrefd on view destruction.
 * \param [out] view    Pointer to the view array to be created.
 *                      This array will refer to the memory of
 *                      the referenced array \a a corresponding to
 *                      indices [offset, offset + length).
 *                      The view is setup and not resizable.
 * \param [in] a        The viewed array must be setup and not resizable.
 * \param [in] offset   The offset of the viewed section in element units.
 *                      The offset + length must be within length of \a a.
 * \param [in] length   The length of the viewed section in element units.
 *                      The offset + length must be within length of \a a.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_new_view (sc3_allocator_t * alloc,
                                        sc3_array_t ** view,
                                        sc3_array_t * a,
                                        int offset, int length);

/** Create a view array pointing to some given memory fragment.
 * We have no means to verify that the fragment exists and stays alive.
 * The view starts out as resizable, permitting future calls to \ref
 * sc3_array_renew_data as long as \ref sc3_array_freeze is not called.
 * \note Potentially dangerous function.  Ensure that the array's length
 * from start offset is supported by the length of the given fragment.
 * Otherwise the behavior is undefined.
 * \param [in,out] alloc    An allocator that is setup, or NULL.
 *                          If NULL, we use \ref sc3_allocator_new_static.
 *                          The allocator is refd and remembered internally
 *                          and will be unrefd on view destruction.
 * \param [out] view    Pointer to the view array to be created.
 *                      It will be returned as a view on the fragment \a data.
 *                      The view is setup and not resizable.
 * \param [in] data     The data must not be moved while view is alive.
 * \param [in] esize    Size of one viewed element in bytes.
 * \param [in] offset   The offset of the viewed data section in element units.
 *                      This offset cannot be changed unless the view is renewed.
 * \param [in] length   The length of the viewed section in element units.
 *                      The view cannot be resized unless it is renewed.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_new_data (sc3_allocator_t * alloc,
                                        sc3_array_t ** view,
                                        void *data, size_t esize,
                                        int offset, int length);

/** Adjust an existing view array for a different window and/or viewed array.
 * It is not possible to renew an array view to view data, or vice versa.
 * Inherit the data element size from the given array.  The view unrefs the
 * previously viewed array and refs the new viewed array, which must thus not
 * be destroyed before this view.  The view must be resizable (the default).
 * \param [in,out] view     Existing view array to be adjusted.
 *                      This array will refer to the memory of
 *                      the referenced array \a a corresponding to
 *                      indices [offset, offset + length).
 *                      Input view must not have been frozen.
 * \param [in] a        The array must be setup and not resizable.
 *                      Its element size must be identical to the view's.
 * \param [in] offset   The offset of the viewed section in element units.
 *                      The offset + length must be within length of \a a.
 * \param [in] length   The length of the viewed section in element units.
 *                      The offset + length must be within length of \a a.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_renew_view (sc3_array_t * view,
                                          sc3_array_t * a,
                                          int offset, int length);

/** Adjust an existing view on data for tracking some given memory fragment.
 * It is not possible to renew a data view to view an array, or vice versa.
 * The viewed memory may change, but not the size of one data element.
 * We have no means to verify that the fragment exists and stays alive.
 * The view must be resizable (which is the default).
 * \note Potentially dangerous function.  Ensure that the array's length
 * from start offset is supported by the length of the given fragment.
 * Otherwise the behavior is undefined.
 * \param [in,out] view     Existing view on data to be adjusted.
 *                      The element size of the view remains the same.
 *                      Input view must not have been frozen.
 * \param [in] data     The data must not be moved while view is alive.
 * \param [in] esize    Size of one viewed element in bytes, same as view's!
 *                      This parameter is added for redundancy and safety.
 * \param [in] offset   The offset of the viewed section in element units.
 *                      This offset cannot be changed unless the view is renewed.
 * \param [in] length   The length of the viewed section in element units.
 *                      The view cannot be resized resized unless it is renewed.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_renew_data (sc3_array_t * view, void *data,
                                          size_t esize, int offset,
                                          int length);

/** Return element size of an array that is setup.
 * \param [in] a        Array must be setup.
 * \param [out] esize   Element size of the array in bytes, or 0 on error.
 *                      Pointer must not be NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_get_elem_size (const sc3_array_t * a,
                                             size_t *esize);

/** Return element count of an array that is setup.
 * \param [in] a        Array must be setup.
 * \param [out] ecount  Element count of the array, or 0 on error.
 *                      Pointer must not be NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_array_get_elem_count (const sc3_array_t * a,
                                              int *ecount);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_ARRAY_H */
