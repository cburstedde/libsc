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

#include <sc3_trace.h>

void
sc3_trace_init (sc3_trace_t * t, const char *func, void *user)
{
  if (t != NULL) {
    t->magic = SC3_TRACE_MAGIC;
    t->user = user;
    t->parent = NULL;
    t->func = func;
    t->depth = 0;
  }
}

sc3_error_t        *
sc3_trace_push (sc3_trace_t ** t, sc3_trace_t * stackvar,
                const char *func, void *user)
{
  /* catch invalid calls */
  SC3A_CHECK (t != NULL);
  if (*t != NULL) {
    SC3A_CHECK ((*t)->magic == SC3_TRACE_MAGIC);
    SC3A_CHECK ((*t)->depth >= 0);
  }
  SC3A_CHECK (stackvar != NULL);

  /* initialize new top object */
  sc3_trace_init (stackvar, func, user);

  /* link new top object to previous stack */
  if ((*t) != NULL) {
    stackvar->parent = *t;
    stackvar->depth = (*t)->depth + 1;
  }
  *t = stackvar;

  /* successful return */
  return NULL;
}
