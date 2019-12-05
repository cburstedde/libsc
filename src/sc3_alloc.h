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

typedef struct sc3_allocator_args sc3_allocator_args_t;
typedef struct sc3_allocator sc3_allocator_t;

#include <sc3_error.h>

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

sc3_error_t        *sc3_allocator_args_new (sc3_allocator_args_t ** aar);
sc3_error_t        *sc3_allocator_args_destroy (sc3_allocator_args_t * aa);

sc3_error_t        *sc3_allocator_args_set_align (sc3_allocator_args_t * aa,
                                                  int align);

/* TODO error-ize below functions */

sc3_allocator_t    *sc3_allocator_new (sc3_allocator_args_t * aa);
void                sc3_allocator_destroy (sc3_allocator_t * a);

void               *sc3_allocator_malloc (sc3_allocator_t * a, size_t size);
void               *sc3_allocator_calloc (sc3_allocator_t * a,
                                          size_t nmemb, size_t size);
void                sc3_allocator_free (sc3_allocator_t * a, void *ptr);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_ALLOC_H */
