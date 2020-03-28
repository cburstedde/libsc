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

/** \file sc3_alloc_internal.h \ingroup sc3_internal
 * We document auxiliary allocation functions used by the library.
 */

#ifndef SC3_ALLOC_INTERNAL_H
#define SC3_ALLOC_INTERNAL_H

#include <sc3_alloc.h>

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** Allocate memory without alignment and internal referencing.
 * It is legal to call \ref sc3_allocator_realloc and \ref
 * sc3_allocator_free on this memory with the allocator passed here.
 * Effectively, we call the system malloc function.
 * \param [in,out] a    Allocator \a a must be setup, have zero alignment
 *                      and not be set to keepalive.  One is obtained,
 *                      e.g., by \ref sc3_allocator_nocount.
 *                      Return NULL if conditions are not met.
 * \param [in] size     Amount of memory to allocate; 0 is legal.
 * \return          Allocated memory on success, NULL otherwise.
 */
void               *sc3_allocator_malloc_noerr (sc3_allocator_t * a,
                                                size_t size);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_ALLOC_INTERNAL_H */
