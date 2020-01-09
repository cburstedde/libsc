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

/** \file sc3_refcount.h
 */

#ifndef SC3_REFCOUNT_H
#define SC3_REFCOUNT_H

#include <sc3_error.h>

#define SC3_REFCOUNT_MAGIC 0x6CA9EFC08917AF1C

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

typedef struct sc3_refcount
{
  long                magic;
  long                rc;
}
sc3_refcount_t;

sc3_error_t        *sc3_refcount_init (sc3_refcount_t * r);
sc3_error_t        *sc3_refcount_ref (sc3_refcount_t * r);
sc3_error_t        *sc3_refcount_unref (sc3_refcount_t * r, int *waslast);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_REFCOUNT_H */
