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

/** \file sc3_alloc.h \ingroup sc3
 * The allocator is a fundamental object to allocate memory on the heap.
 *
 * An allocator provides malloc and free equivalents with counters.
 * It keeps track of the number of allocs and frees to aid in dedugging.
 * Furthermore, it is refd by every allocation and unrefd by deallocation.
 * The latter feature avoids error returns on dropping the last reference
 * to an allocator, and in turn any other object, with live allocations.
 * These counting mechanisms can be disabled.
 *
 * Different allocators are independent objects with independent counters.
 * This feature is useful for example to isolate memory between threads:
 * Each thread may create its own allocator, derived from the result of \ref
 * sc3_allocator_new_static, and then use the new allocator without locking.
 * Generally, sc3 objects are safe to use in threads as long as no object
 * is accessed simultaneously by different threads.
 *
 * Most sc3_object_new functions take an allocator as argument.
 * It will be used for allocations throughout the lifetime of the object.
 * They would also accept a NULL allocator, which defaults to using \ref
 * sc3_allocator_new_static internally, which does not count or align,
 * for example to bootstrap a forest-type graph of derived allocators.
 * Having different allocators for different functionalities or algorithms
 * can improve modularity of allocation and the associated debugging.
 * Each allocator can be configured with its own alignment requirements.
 * Each allocator is counting its allocations by default for leak checking.
 *
 * Allocators can be refd and unrefd (even the static one, which noops).
 * Dropping the last reference deallocates the allocator (if not static).
 * The function \ref sc3_allocator_destroy must only be called when it
 * is known that the allocator has only one reference to it (ok for static).
 * With counting enabled (only when non-static), dropping the last reference
 * of an allocator will fail fatally when it still has live allocations.
 */

#ifndef SC3_ALLOC_H
#define SC3_ALLOC_H

#include <sc3_error.h>

/** The allocator object is an opaque struct. */
typedef struct sc3_allocator sc3_allocator_t;

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** Check whether an allocator is not NULL and internally consistent.
 * The allocator may be valid in both its setup and usage phases.
 * Any allocation by \ref sc3_allocator_malloc or \ref sc3_allocator_calloc
 * may be followed by an arbitrary number of \ref sc3_allocator_realloc calls
 * and must then be followed by \ref sc3_allocator_free.
 * The rules for zero input sizes and NULL input pointers are the same
 * as those in the C standard library.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if pointer is not NULL and allocator consistent.
 */
int                 sc3_allocator_is_valid (const sc3_allocator_t * a,
                                            char *reason);

/** Check whether an allocator is not NULL, consistent and not setup.
 * This means that the allocator is not (yet) in its usage phase.
 * \param [in] a    Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return          True if pointer not NULL, allocator consistent, not setup.
 */
int                 sc3_allocator_is_new (const sc3_allocator_t * a,
                                          char *reason);

/** Check whether an allocator is not NULL, internally consistent and setup.
 * This means that the allocator is in its usage phase.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return          True if pointer not NULL, allocator consistent and setup.
 */
int                 sc3_allocator_is_setup (const sc3_allocator_t * a,
                                            char *reason);

/** Return whether a setup allocator does not hold any allocations.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return          True if pointer not NULL, allocator setup
 *                  and not holding any allocations.
 */
int                 sc3_allocator_is_free (const sc3_allocator_t * a,
                                           char *reason);

/** Create a new allocator object in its setup phase.
 * It begins with default parameters that can be overridden explicitly.
 * Default alignment is sizeof (void *); see \ref sc3_allocator_set_align.
 * Setting and modifying parameters is only allowed in the setup phase.
 * Call \ref sc3_allocator_setup to change it into its usage phase.
 * After that, no more parameters may be set.
 * \param [in,out] oa   Either NULL or an allocator that is setup.
 *                      In the latter case, the allocator is refd and
 *                      remembered and will be unrefd on destruction.
 *                      This argument (or a static internal one when
 *                      NULL) is used to allocate the output object.
 * \param [out] ap      Pointer must not be NULL.
 *                      If the function returns an error, value set to NULL.
 *                      Otherwise, value set to allocator with default values.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_allocator_new (sc3_allocator_t * oa,
                                       sc3_allocator_t ** ap);

/** Set byte alignment followed by the allocator.
 * Default is at least sizeof (void *).
 * \param [in,out] a    Valid allocator not setup.
 * \param [in] align    Power of two designating byte alignment of memory,
 *                      or zero for system default alignment.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_allocator_set_align (sc3_allocator_t * a,
                                             size_t align);

/** Set whether the allocator keeps track of malloc and free counts.
 * If true, it requires the count to be zero when its last reference drops,
 * otherwise exiting with a fatal error out of that \ref sc3_allocator_unref.
 * The count status can be queried by \ref sc3_allocator_is_free either way.
 * \param [in,out] a    Valid allocator not setup.
 * \param [in] counting Boolean to enable counting.  Default is true.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_allocator_set_counting (sc3_allocator_t * a,
                                                int counting);

/** Setup an allocator and put it into its usable phase.
 * \param [in,out] a    This allocator must not yet be setup.
 *                      Internal storage is allocated, the setup phase ends,
 *                      and the allocator is put into its usable phase.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_allocator_setup (sc3_allocator_t * a);

/** Increase the reference count on an allocator by 1.
 * This is only allowed after the allocator has been setup.
 * \param [in,out] a    Allocator must be setup.  Its refcount is increased.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_allocator_ref (sc3_allocator_t * a);

/** Decrease the reference count on an allocator by 1.
 * If the reference count drops to zero, the allocator is deallocated.
 * \param [in,out] ap   The pointer must not be NULL and the allocator valid.
 *                      Its refcount is decreased.  If it reaches zero,
 *                      the allocator is destroyed and the value set to NULL.
 * \return              NULL on success, fatal error object otherwise.
 *                      If the reference count drops to zero while counting
 *                      and holding memory, return kind \ref SC3_ERROR_LEAK.
 */
sc3_error_t        *sc3_allocator_unref (sc3_allocator_t ** ap);

/** Destroy an allocator with a reference count of 1.
 * It is a leak error to destroy when multiply refd or with live allocations.
 * \param [in,out] ap   This allocator must be valid.
 *                      On output, value is set to NULL.
 * \return              NULL on success, error object otherwise.
 *                      When the allocator has more than one reference to it,
 *                      or when the allocater is counting and has allocations,
 *                      return a fatal error of kind \ref SC3_ERROR_LEAK.
 */
sc3_error_t        *sc3_allocator_destroy (sc3_allocator_t ** ap);

/** Return a static allocator that is safe to use from multiple threads.
 * The allocator does not align and does not count its allocations.
 * It can be refd, unrefd and destroyed as usual (even though this noops).
 * The result it is less valuable for debugging, but carefree to use.
 * Contrary to most libsc functions, function's return value is its output.
 * \return              A static allocator without alignment or counting.
 */
sc3_allocator_t    *sc3_allocator_new_static (void);

/** Query internal allocation overhead in bytes.
 * When the allocator is counting or aligning, we allocate a slightly
 * larger amount of bytes than specified for internal bookkeeping.
 * This function queries this value such that calling code can optimize
 * the actual number of bytes it allocates to fit the page size, etc.
 * \param [in] a        Allocator must be setup.
 * \param [out] oh      Pointer must not be NULL.  On output
 *                      the overhead of each allocation in bytes.
 * \return              NULL on success, fatal error object otherwise.
 */
sc3_error_t        *sc3_allocator_get_overhead (sc3_allocator_t * a,
                                                size_t *oh);

/** Allocate memory and copy a Nul-terminated string into it.
 * Unlike strdup (3), memory is passed by a reference argument.
 * \param [in,out] a    The allocator must be setup.
 * \param [in] src      Nul-terminated string.
 * \param [out] dest    Non-NULL pointer to the address of output,
 *                      memory with the input string copied into it.
 *                      May be passed to \ref sc3_allocator_realloc
 *                      any number of times and must eventually be
 *                      released with \ref sc3_allocator_free.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_allocator_strdup (sc3_allocator_t * a,
                                          const char *src, char **dest);

/** Allocate memory that is not initialized.
 * Unlike malloc (3), memory is passed by a reference argument.
 * \param [in,out] a    The allocator must be setup.
 * \param [in] size     Bytes to allocate, zero is legal.
 *                      Behavior is same as malloc (3).
 * \param [out] ptr     Non-NULL address of a pointer.
 *                      Contains newly allocated pointer on output.
 *                      When size is zero, assigns NULL or a pointer suitable
 *                      for \ref sc3_allocator_realloc and/or \ref
 *                      sc3_allocator_free to the address.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_allocator_malloc (sc3_allocator_t * a, size_t size,
                                          void *ptr);

/** Allocate memory that is initialized to zero.
 * Unlike calloc (3), memory is passed by a reference argument.
 * \param [in,out] a    The allocator must be setup.
 * \param [in] nmemb    Number of items to allocate, zero is legal.
 * \param [in] size     Bytes to allocate for each item, zero is legal.
 * \param [out] ptr     Non-NULL address of a pointer.
 *                      Contains newly allocated pointer on output.
 *                      When nmemb * size is zero, assigns NULL or a pointer
 *                      suitable for \ref sc3_allocator_realloc and/or \ref
 *                      sc3_allocator_free to the address, like calloc (3).
 *                      If nmemb * size is greater zero, memory is zeroed.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_allocator_calloc (sc3_allocator_t * a,
                                          size_t nmemb, size_t size,
                                          void *ptr);

/** Change the allocated size of a previously allocated pointer.
 * Unlike realloc (3), memory is passed by a reference argument.
 * \param [in,out] a    Allocator must be setup.  If input is non-NULL, must
 *                      be the same as used on (re-)allocation.
 * \param [in,out] ptr  Non-NULL address of pointer.
 *                      On input, value is NULL or pointer created by \ref
 *                      sc3_allocator_malloc, \ref sc3_allocator_calloc or \ref
 *                      sc3_allocator_realloc.  Reallocated memory on output.
 *                      If input is NULL, the call is equivalent to \ref
 *                      sc3_allocator_malloc.  If input is not NULL but new_size
 *                      is zero, equivalent to \ref sc3_allocator_free.
 *                      The memory content is unchanged to the minimum of
 *                      the old and new sizes.
 * \param [in] new_size New byte allocation for pointer, zero is legal.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_allocator_realloc (sc3_allocator_t * a,
                                           void *ptr, size_t new_size);

/** Free previously allocated memory.
 * Unlike free (3), memory is passed by a reference argument.
 * \param [in,out] a    Allocator must be setup.  If input is non-NULL, must
 *                      be the same as used on (re-)allocation.
 * \param [in,out] ptr  Non-NULL address of pointer previously allocated by
 *                      this allocator by \ref sc3_allocator_malloc, \ref
 *                      sc3_allocator_calloc or \ref sc3_allocator_realloc.
 *                      Output value is set to NULL on success.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_allocator_free (sc3_allocator_t * a, void *ptr);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_ALLOC_H */
