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

/** \file sc3_options.h \ingroup sc3
 *
 * We provide an object to collect command line options.
 * Multiple of instances may be used flexibly to parse command lines.
 */

#ifndef SC3_OPTIONS_H
#define SC3_OPTIONS_H

#include <sc3_alloc.h>

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** The options object is an opaque struct. */
typedef struct sc3_options sc3_options_t;

/** Query whether options object is not NULL and internally consistent.
 * The object may be valid in both its setup and usage phases.
 * \param [in] yy       Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if pointer is not NULL and object consistent.
 */
int                 sc3_options_is_valid (const sc3_options_t * yy,
                                          char *reason);

/** Query whether options object is not NULL, consistent and not setup.
 * This means that the object is not (yet) in its usage phase.
 * \param [in] yy       Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if pointer not NULL, object consistent, not setup.
 */
int                 sc3_options_is_new (const sc3_options_t * yy,
                                        char *reason);

/** Query whether options object is not NULL, internally consistent and setup.
 * This means that the object's parameterization is final and it is usable.
 * \param [in] yy       Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if pointer not NULL, object consistent and setup.
 */
int                 sc3_options_is_setup (const sc3_options_t * yy,
                                          char *reason);

#if 0

/** Query whether an object is setup and has the dummy parameter set.
 * \param [in] yy       Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if pointer not NULL, object setup and resizable.
 */
int                 sc3_options_is_dummy (const sc3_options_t * yy,
                                          char *reason);

#endif

/** Create a new options object in its setup phase.
 * It begins with default parameters that can be overridden explicitly.
 * Setting and modifying parameters is only allowed in the setup phase.
 * Call \ref sc3_options_setup to change the object into its usage phase.
 * The defaults are documented in the sc3_options_set_* calls.
 * \param [in,out] alloc    An allocator that is setup, or NULL.
 *                          If NULL, we call \ref sc3_allocator_new_static.
 *                          The allocator is refd and remembered internally
 *                          and will be unrefd on object destruction.
 * \param [out] yyp     Pointer must not be NULL.
 *                      If the function returns an error, value set to NULL.
 *                      Otherwise, value set to an object with default values.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_new (sc3_allocator_t * alloc,
                                     sc3_options_t ** yyp);

#if 0

/** Set the dummy parameter (boolean) of options object.
 * Default is false.
 * \param [in,out] yy   The object must not be setup.
 * \param [in] dummy    New value for dummy variable.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_set_dummy (sc3_options_t * yy, int dummy);

#endif

/** Setup an object and change it into its usable phase.
 * \param [in,out] yy   This object must not yet be setup.  Setup on output.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_setup (sc3_options_t * yy);

/** Increase the reference count on options object by 1.
 * This is only allowed after the object has been setup.
 * \param [in,out] yy   Object must be setup.  Its refcount is increased.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_ref (sc3_options_t * yy);

/** Decrease the reference count on options object by one.
 * If the reference count drops to zero, the object is deallocated.
 * \param [in,out] yyp  The pointer must not be NULL and the object valid.
 *                      Its refcount is decreased.  If it reaches zero,
 *                      the object is destroyed and the value set to NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_unref (sc3_options_t ** yyp);

/** Destroy options object with a reference count of one.
 * It is a fatal error to destroy an object that is multiply referenced. We
 * unref the allocator that has been passed and refd in \ref sc3_options_new.
 * \param [in,out] yyp  This object must be valid and have a refcount of 1.
 *                      On output, value is set to NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_destroy (sc3_options_t ** yyp);

#if 0

/** Return dummy parameter of an object that is setup.
 * \param [in] yy       Object must be setup.
 * \param [out] dummy   Dummy parameter.  Pointer must not be NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_get_dummy (const sc3_options_t * yy,
                                           int *dummy);

#endif

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_OPTIONS_H */
