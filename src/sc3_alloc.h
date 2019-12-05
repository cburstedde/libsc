/*
  This file is part of the SC Library, version 3.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

  The SC Library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with the SC Library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

/** \file sc3_alloc.h
 */

#ifndef SC3_ALLOC_H
#define SC3_ALLOC_H

#include <sc3.h>

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

extern sc3_allocator_t const *sc3_nocount_allocator;

/* TODO: refcounting the arguments? */

sc3_error_t        *sc3_allocator_args_new (sc3_allocator_t * oa,
                                            sc3_allocator_args_t ** aap);
sc3_error_t        *sc3_allocator_args_destroy (sc3_allocator_args_t ** aap);

sc3_error_t        *sc3_allocator_args_set_align (sc3_allocator_args_t * aa,
                                                  int align);

/** Creates a new allocator from arguments.
 * \param [in,out] aa   We call \ref sc3_allocator_args_destroy on it.
 */
sc3_error_t        *sc3_allocator_new (sc3_allocator_args_t * aa,
                                       sc3_allocator_t ** ap);
sc3_error_t        *sc3_allocator_ref (sc3_allocator_t * a);
sc3_error_t        *sc3_allocator_unref (sc3_allocator_t ** ap);
sc3_error_t        *sc3_allocator_destroy (sc3_allocator_t ** ap);

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
