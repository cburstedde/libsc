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

/** \file sc3_refcount.h \ingroup sc3
 * We implement a minimal reference counting mechanism.
 *
 * The reference counter is a public struct with an \a rc member variable.
 * Its value begins at 1 and may be arbitrarily incremented.
 * It may also be decremented.
 * When it reaches 0, the object that contains the reference counter is
 * considered expired.
 * Values below zero cannot occur within this convention.
 *
 * Except for the query functions, the reference counter functions return an
 * \ref sc3_error_t object.
 * This is NULL if the function executed successfully.
 * The functions may return a non-NULL object if an assertion fails,
 * such as passing a NULL reference counter argument to \ref sc3_refcount_init
 * or an invalid object pointer or NULL \a waslast parameter to \ref
 * sc3_refcount_unref.
 *
 * We do not lock the \ref sc3_refcount_t struct.
 * It is the application's responsibility to ensure thread safety.
 */

#ifndef SC3_REFCOUNT_H
#define SC3_REFCOUNT_H

#include <sc3_error.h>

/** Arbitrarily chosen number to catch uninitialized objects. */
#define SC3_REFCOUNT_MAGIC 0x6CA9EFC08917AF1C

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** The reference counter is a public struct.
 * We count references from 1 upwards.
 * If a counter is decremented to zero, the object's life is over.
 */
typedef struct sc3_refcount
{
  /** This structure is only valid if the value is \ref SC3_REFCOUNT_MAGIC. */
  long                magic;

  /** The reference count is 1 or higher for a valid object. */
  long                rc;
}
sc3_refcount_t;

/** Query a reference counter for validity.
 * \param [in] r        NULL or existing reference counter.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL and refcounter valid.
 */
int                 sc3_refcount_is_valid (const sc3_refcount_t * r,
                                           char *reason);

/** Query a reference counter for validity and holding exactly one reference.
 * \param [in] r        NULL or existing reference counter.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL and refcounter valid
 *                      with exactly one reference.
 */
int                 sc3_refcount_is_last (const sc3_refcount_t * r,
                                          char *reason);

/** Initialize reference counter to be invalid (thus unusable).
 * \param [out] r       Existing reference counter memory.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_refcount_init_invalid (sc3_refcount_t * r);

/** Initialize reference counter to be valid and have a count of one.
 * \param [out] r       Existing reference counter memory.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_refcount_init (sc3_refcount_t * r);

/** Increase the reference count of a valid counter by one.
 * \param [in,out] r    Valid reference counter.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_refcount_ref (sc3_refcount_t * r);

/** Decrease the reference count of a valid counter by one.
 * If the count drops to zero, the counter is invalidated,
 * which is also considered a success of the function.
 * \param [in,out] r    Valid reference counter.
 * \param [out] waslast Must not be NULL.  Becomes 0 on error.
 *                      Otherwise true iff the count drops to zero.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_refcount_unref (sc3_refcount_t * r, int *waslast);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_REFCOUNT_H */
