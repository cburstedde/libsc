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

/** \file sc3_refcount_internal.h
 */

#ifndef SC3_REFCOUNT_INTERNAL_H
#define SC3_REFCOUNT_INTERNAL_H

#include <sc3_refcount.h>

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** Query a reference counter for validity.
 * \param [in] r        NULL or existing reference counter.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL and refcounter valid.
 */
int                 sc3_refcount_is_valid (sc3_refcount_t * r, char *reason);

/** Query a reference counter for validity and holding exactly one reference.
 * \param [in] r        NULL or existing reference counter.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL and refcounter valid
 *                      with exactly one reference.
 */
int                 sc3_refcount_is_last (sc3_refcount_t * r, char *reason);

/** Initialize reference counter to be invalid (thus unusable).
 * \param [out] r       Existing reference counter memory.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_refcount_init_invalid (sc3_refcount_t * r);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_REFCOUNT_INTERNAL_H */
