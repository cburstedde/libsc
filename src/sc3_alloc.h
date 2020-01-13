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

/** \file sc3_alloc.h
 */

#ifndef SC3_ALLOC_H
#define SC3_ALLOC_H

#include <sc3_base.h>

typedef struct sc3_allocator sc3_allocator_t;

#include <sc3_error.h>

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

#define SC3E_ALLOCATOR_MALLOC(a,t,n,p) do {                             \
  void *_ptr;                                                           \
  SC3E (sc3_allocator_malloc (a, (n) * sizeof (t), &_ptr));             \
  (p) = (t *) _ptr; } while (0)
#define SC3E_ALLOCATOR_CALLOC(a,t,n,p) do {                             \
  void *_ptr;                                                           \
  SC3E (sc3_allocator_calloc (a, n, sizeof (t), &_ptr));                \
  (p) = (t *) _ptr; } while (0)
#define SC3E_ALLOCATOR_FREE(a,t,p) do {                                 \
  SC3E (sc3_allocator_free (a, p));                                     \
  (p) = (t *) 0; } while (0)

/** Check whether an allocator is not NULL and internally consistent.
 * The allocator may be valid in both its setup and usage phases.
 * \param [in] a        Any pointer.
 * \return              True iff pointer is not NULL and allocator consistent.
 */
int                 sc3_allocator_is_valid (sc3_allocator_t * a);

/** Check whether an allocator is not NULL, consistent and not setup.
 * This means that the allocator is not in its usage phase.
 * \param [in] a    Any pointer.
 * \return          True iff pointer not NULL, allocator consistent, not setup.
 */
int                 sc3_allocator_is_new (sc3_allocator_t * a);

/** Check whether an allocator is not NULL, internally consistent and setup.
 * This means that the allocator is in its usage phase.
 * \param [in] a    Any pointer.
 * \return          True iff pointer not NULL, allocator consistent and setup.
 */
int                 sc3_allocator_is_setup (sc3_allocator_t * a);

/** Return whether a setup allocator does not hold any allocations.
 * \param [in]      Any pointer.
 * \return          True iff pointer not NULL, allocator setup
 *                  and not holding any allocations.
 */
int                 sc3_allocator_is_free (sc3_allocator_t * a);

/** Return a non-counting allocator setup and safe to use in threads.
 * This allocator thus does not check for matched alloc/free calls.
 * It can be arbitrarily refd and unrefd but must not be destroyed.
 * Use only if there is no other option.
 * \return              Allocator that does not count its allocations.
 *                      It is not allowed to destroy this allocator.
 */
sc3_allocator_t    *sc3_allocator_nocount (void);

/** Return a counting allocator setup and not protected from threads.
 * This allocator is not safe to use concurrently from multiple threads.
 * It can be arbitrarily refd and unrefd but must not be destroyed.
 * Use this function to create the first allocator in main ().
 * Use only if there is no other option.
 * \return              Allocator unprotected from concurrent access.
 *                      It is not allowed to destroy this allocator.
 */
sc3_allocator_t    *sc3_allocator_nothread (void);

/** Create a new allocator object in its setup phase.
 * It begins with default parameters that can be overridden explicitly.
 * Setting and modifying parameters is only allowed in the setup phase.
 * Call \ref sc3_allocator_setup to change it into its usage phase.
 * After that, no more parameters may be set.
 * \param [in] oa       An allocator that is setup.
 *                      The allocator is refd and remembered internally
 *                      and will be unrefd on destruction.
 * \param [out] ap      Pointer must not be NULL.
 *                      If the function returns an error, value set to NULL.
 *                      Otherwise, value set to allocator with default values.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_allocator_new (sc3_allocator_t * oa,
                                       sc3_allocator_t ** ap);

/** Set byte alignment followed by the allocator.
 * \param [in] a        Valid allocator not setup.
 * \param [in] align    Power of two designating byte alignment of memory,
 *                      or zero for default alignment.
 */
sc3_error_t        *sc3_allocator_set_align (sc3_allocator_t * a, int align);

/** Setup an allocator and put it into its usable phase.
 * \param [in] a        This allocator must not yet be setup.
 *                      Internal storage is allocated, the setup phase ends,
 *                      and the allocator is put into its usable phase.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_allocator_setup (sc3_allocator_t * a);

/** Increase the reference count on an allocator by 1.
 * This is only allowed after the allocator has been setup.
 * \param [in] a        Allocator must be setup.  Its refcount is increased.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_allocator_ref (sc3_allocator_t * a);

/** Decrease the reference count on an allocator by 1.
 * If the reference count drops to zero, the allocator is deallocated.
 * \param [in] ap       The pointer must not be NULL and the allocator valid.
 *                      Its refcount is decreased.  If it reaches zero,
 *                      the allocator is destroyed and the value set to NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_allocator_unref (sc3_allocator_t ** ap);

/** Destroy an allocator with a reference count of 1.
 * Do not destroy an allocator that is multiply refd or not allocated.
 * \param [in,out] ap   This allocator must be valid and have a refcount of 1.
 *                      On output, value is set to NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_allocator_destroy (sc3_allocator_t ** ap);

sc3_error_t        *sc3_allocator_strdup (sc3_allocator_t * a,
                                          const char *src, char **dest);
sc3_error_t        *sc3_allocator_malloc (sc3_allocator_t * a, size_t size,
                                          void **ptr);
sc3_error_t        *sc3_allocator_calloc (sc3_allocator_t * a,
                                          size_t nmemb, size_t size,
                                          void **ptr);
sc3_error_t        *sc3_allocator_free (sc3_allocator_t * a, void *ptr);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_ALLOC_H */
