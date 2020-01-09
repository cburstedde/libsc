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
  SC3E (sc3_allocator_calloc (a, n, sizeof (t), &_ptr);                 \
  (p) = (t *) _ptr; } while (0)
#define SC3E_ALLOCATOR_FREE(a,t,p) do {                                 \
  SC3E (sc3_allocator_free (a, p));                                     \
  (p) = (t *) 0; } while (0)

typedef struct sc3_allocator_args sc3_allocator_args_t;

sc3_allocator_t    *sc3_allocator_nocount (void);

/* TODO: refcounting the arguments? */

sc3_error_t        *sc3_allocator_args_new (sc3_allocator_t * oa,
                                            sc3_allocator_args_t ** aap);
sc3_error_t        *sc3_allocator_args_destroy (sc3_allocator_args_t ** aap);

sc3_error_t        *sc3_allocator_args_set_align (sc3_allocator_args_t * aa,
                                                  int align);

/** Creates a new allocator from arguments.
 * \param [in,out] aa   We call \ref sc3_allocator_args_destroy on it.
 * \param [out] ap      On output, the initialized allocator with refcount 1.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_allocator_new (sc3_allocator_args_t ** aap,
                                       sc3_allocator_t ** ap);
sc3_error_t        *sc3_allocator_ref (sc3_allocator_t * a);
sc3_error_t        *sc3_allocator_unref (sc3_allocator_t ** ap);

/** It is an error to destroy an allocator that is not allocated. */
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
