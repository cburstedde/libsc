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

/** \file sc3_memstamp.h \ingroup sc3
 * A data container to create memory items of the same size.
 * Allocations are bundled so it's fast for small memory sizes.
 * The items created will remain valid until the container is destroyed.
 * It is possible to return allocated items to the container for reuse.
 * 
 * The memstamp container stores any number of fixed-size items within a new
 * allocation, or stamp. If needed, memory is reallocated internally.
 *
 * We use standard `int` types for indexing.
 *
 * In the setup phase, we set the size of the element and stamp,
 * a number of items in stamp, an initzero property, and some more.
 *
 * An memstamp container can only be refd if it is setup.
 * Otherwise, the usual ref-, unref- and destroy semantics hold.
 */

#ifndef SC3_MEMSTAMP_H
#define SC3_MEMSTAMP_H

#include <sc3_error.h>

/** The memstamp container is an opaque struct. */
typedef struct sc3_mstamp sc3_mstamp_t;

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** Query whether a memory stamp container is not NULL and internally
 * consistent.
 * The container may be valid in both its setup and usage phases.
 * \param [in] mst      Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer is not NULL and container consistent.
 */
int                 sc3_mstamp_is_valid (const sc3_mstamp_t * mst,
                                         char *reason);

/** Query whether a memory stamp is not NULL, consistent and not setup.
 * This means that the memory stamp is not (yet) in its usage phase.
 * \param [in] mst      Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL, memory stamp consistent,
 *                      not setup.
 */
int                 sc3_mstamp_is_new (const sc3_mstamp_t * mst,
                                       char *reason);

/** Query whether a memory stamp is not NULL, internally consistent and setup.
 * This means that the memory stamp is in its usage phase.
 * We provide allocation of fixed-size memory items
 * without allocating new memory in every request.
 * Instead we block the allocations in what we call a stamp of multiple items.
 * Even if no allocations are done, the container's internal memory
 * must be freed eventually by \ref sc3_mstamp_destroy.
 * \param [in] mst      Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL, memory stamp consistent and
 *                      setup.
 */
int                 sc3_mstamp_is_setup (const sc3_mstamp_t * mst,
                                         char *reason);

/** Create a new memory stamp container in its setup phase.
 * It begins with default parameters that can be overridden explicitly.
 * Setting and modifying parameters is only allowed in the setup phase.
 * Call \ref sc3_mstamp_setup to change the memory stamp into its usage phase.
 * After that, no more parameters may be set.
 * \param [in,out] aator    An allocator that is setup.
 *                          The allocator is refd and remembered internally
 *                          and will be unrefd on memory stamp container
 *                          destruction.
 * \param [out] mstp    Pointer must not be NULL.
 *                      If the function returns an error, value set to NULL.
 *                      Otherwise, value set to a memory stamp with default
 *                      values.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mstamp_new (sc3_allocator_t * aator,
                                    sc3_mstamp_t ** mstp);

/** Set the size of each memory stamp element in bytes.
 * \param [in,out] mst  The memory stamp must not be setup.
 * \param [in] esize    Element size in bytes. Zero is legal, in that case
 *                      \ref sc3_mstamp_alloc output is NULL. One is default.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mstamp_set_elem_size (sc3_mstamp_t * mst,
                                              size_t esize);

/** Set the size of each memory stamp in bytes.
 * \param [in,out] mst  The memory stamp must not be setup.
 * \param [in] ssize    Size in bytes of each memory block that we allocate.
 *                      If it is larger than the element size,
 *                      we may place more than one element in it.
 *                      Passing 0 is legal and forces
 *                      stamps that hold one item each.  Default is 4096.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mstamp_set_stamp_size (sc3_mstamp_t * mst,
                                               size_t ssize);

/** Set the initzero property of a memory stamp container.
 * If set to true, every new stamp will be initialized with zeros.
 * \param [in,out] mst      The memory stamp container must not be setup.
 * \param [in] initzero     Boolean; default is false.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mstamp_set_initzero (sc3_mstamp_t * mst,
                                             int initzero);

/** Setup a memory stamp container and change it into its usable phase.
 * We provide allocation of fixed-size memory items without allocating
 * new memory in every request.
 * Instead we block the allocations in what we call a stamp of multiple items.
 * Even if no allocations are done, the container's internal memory
 * must be freed eventually by \ref sc3_mstamp_destroy.
 * \param [in,out] mst  This memory stamp container must not yet be setup.
 *                      Internal storage is allocated, the setup phase ends,
 *                      and the container is put into its usable phase.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mstamp_setup (sc3_mstamp_t * mst);

/** Increase the reference count on a memory stamp container by 1.
 * This is only allowed after the memory stamp container has been setup.
 * \param [in,out] mst  Its refcount is increased.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mstamp_ref (sc3_mstamp_t * mst);

/** Decrease the reference count on a memory stamp container by 1.
 * If the reference count drops to zero, the container is deallocated.
 * \param [in,out] mstp The pointer must not be NULL and the container valid.
 *                      Its refcount is decreased.  If it reaches zero,
 *                      the memory stamp container is destroyed and
 *                      the value set to NULL.
 * \return              NULL on success, error object otherwise.
 *                      We return a leak if we find one.
 *
 * \todo Think about returning a NULL value in unref in all cases.
 */
sc3_error_t        *sc3_mstamp_unref (sc3_mstamp_t ** mstp);

/** Destroy a memory stamp container by freeing all memory
 * in a stamps structure.
 * It is a leak error to destroy a memory stamp container that is
 * multiply referenced. We unref its internal allocator, which may cause a
 * fatal error if that allocator has been overly unrefd elsewhere in the code.
 * \param [in,out] mstp This container must be valid and have a refcount of 1.
 *                      On output, value is set to NULL.
 * \return              NULL on success, error object otherwise.
 *                      When the memory stamp container had more than one
 *                      reference, return an error of kind \ref SC3_ERROR_LEAK.
 */
sc3_error_t        *sc3_mstamp_destroy (sc3_mstamp_t ** mstp);

/** Return a new item.
 * The memory returned will stay legal until container is destroyed,
 * or equivalently, its reference count drops to zero.
 * \param [in,out] mst  Memory stamp container must be setup.
 * \param [out] itemp   Pointer to an item ready to use.
 *                      Legal until \ref sc3_mstamp_destroy or
 *                      \ref sc3_mstamp_unref (with one ref)
 *                      is called on \a mst.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mstamp_alloc (sc3_mstamp_t * mst, void **itemp);

/** Return a previously allocated element to the container.
 * \param [in] mst      Memory stamp container must be setup.
 * \param [in] elem     Pointer to an element to be returned to the pool.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mstamp_free (sc3_mstamp_t * mst, void *elem);

/** Return element size of a memory stamp container that is setup.
 * \param [in] mst      Memory stamp container must be setup.
 * \param [out] esize   Element size of the container in bytes, or 0 on error.
 *                      Pointer may be NULL, then we do nothing.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mstamp_get_elem_size (sc3_mstamp_t * mst,
                                              size_t *esize);

/** Return number of valid elements of a memory stamp container that is setup.
 * \param [in] mst      Memory stamp container must be setup.
 * \param [out] scount  The number of stamps in the container or 0 on error.
 *                      Pointer may be NULL, then we do nothing.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mstamp_get_elem_count (sc3_mstamp_t * mst,
                                               int *scount);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_MEMSTAMP_H */
