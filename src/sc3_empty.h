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

/** \file sc3_empty.h \ingroup sc3
 *
 * The empty object serves as a template for new functionality.
 * This is as far as we're willing to go towards an object system.
 *
 * In the setup phase, we may set a dummy boolean parameter.
 * After setup is completed, we may query the dummy parameter.
 * The usual ref-, unref- and destroy semantics hold.
 */

#ifndef SC3_EMPTY_H
#define SC3_EMPTY_H

#include <sc3_alloc.h>

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** The empty template object is an opaque struct. */
typedef struct sc3_empty sc3_empty_t;

/** Query whether an object is not NULL and internally consistent.
 * The object may be valid in both its setup and usage phases.
 * \param [in] y        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if pointer is not NULL and object consistent.
 */
int                 sc3_empty_is_valid (const sc3_empty_t * y, char *reason);

/** Query whether an object is not NULL, consistent and not setup.
 * This means that the object is not (yet) in its usage phase.
 * \param [in] y        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if pointer not NULL, object consistent, not setup.
 */
int                 sc3_empty_is_new (const sc3_empty_t * y, char *reason);

/** Query whether an object is not NULL, internally consistent and setup.
 * This means that the object's parameterization is final and it is usable.
 * \param [in] y        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if pointer not NULL, object consistent and setup.
 */
int                 sc3_empty_is_setup (const sc3_empty_t * y, char *reason);

/** Query whether an object is setup and has the dummy parameter set.
 * \param [in] y        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if pointer not NULL, object setup and resizable.
 */
int                 sc3_empty_is_dummy (const sc3_empty_t * y, char *reason);

/** Create a new empty object in its setup phase.
 * It begins with default parameters that can be overridden explicitly.
 * Setting and modifying parameters is only allowed in the setup phase.
 * Call \ref sc3_empty_setup to change the object into its usage phase.
 * The defaults are documented in the sc3_empty_set_* calls.
 * \param [in,out] alloc    An allocator that is setup, or NULL.
 *                          If NULL, we call \ref sc3_allocator_new_static.
 *                          The allocator is refd and remembered internally
 *                          and will be unrefd on object destruction.
 * \param [out] yp      Pointer must not be NULL.
 *                      If the function returns an error, value set to NULL.
 *                      Otherwise, value set to an object with default values.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_empty_new (sc3_allocator_t * alloc,
                                   sc3_empty_t ** yp);

/** Set the dummy parameter (boolean) of an object.
 * Default is false.
 * \param [in,out] y    The object must not be setup.
 * \param [in] dummy    New value for dummy variable.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_empty_set_dummy (sc3_empty_t * y, int dummy);

/** Setup an object and change it into its usable phase.
 * \param [in,out] y    This object must not yet be setup.  Setup on output.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_empty_setup (sc3_empty_t * y);

/** Increase the reference count on an object by 1.
 * This is only allowed after the object has been setup.
 * \param [in,out] y    Object must be setup.  Its refcount is increased.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_empty_ref (sc3_empty_t * y);

/** Decrease the reference count on an object by one.
 * If the reference count drops to zero, the object is deallocated.
 * \param [in,out] yp   The pointer must not be NULL and the object valid.
 *                      Its refcount is decreased.  If it reaches zero,
 *                      the object is destroyed and the value set to NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_empty_unref (sc3_empty_t ** yp);

/** Destroy an object with a reference count of one.
 * It is a fatal error to destroy an object that is multiply referenced. We
 * unref the allocator that has been passed and refd in \ref sc3_empty_new.
 * \param [in,out] yp   This object must be valid and have a refcount of 1.
 *                      On output, value is set to NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_empty_destroy (sc3_empty_t ** yp);

/** Return dummy parameter of an object that is setup.
 * \param [in] y        Object must be setup.
 * \param [out] dummy   Dummy parameter.  Pointer must not be NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_empty_get_dummy (const sc3_empty_t * y, int *dummy);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_EMPTY_H */
