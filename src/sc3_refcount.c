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

#include <sc3_refcount_internal.h>

int
sc3_refcount_is_valid (sc3_refcount_t * r)
{
  return r != NULL && r->magic == SC3_REFCOUNT_MAGIC && r->rc >= 1;
}

int
sc3_refcount_is_last (sc3_refcount_t * r)
{
  return r != NULL && r->magic == SC3_REFCOUNT_MAGIC && r->rc == 1;
}

sc3_error_t        *
sc3_refcount_init_invalid (sc3_refcount_t * r)
{
  SC3A_CHECK (r != NULL);

  r->magic = 0;
  r->rc = 0;
  return NULL;
}

sc3_error_t        *
sc3_refcount_init (sc3_refcount_t * r)
{
  SC3A_CHECK (r != NULL);

  r->magic = SC3_REFCOUNT_MAGIC;
  r->rc = 1;
  return NULL;
}

sc3_error_t        *
sc3_refcount_ref (sc3_refcount_t * r)
{
  SC3A_CHECK (r != NULL && r->magic == SC3_REFCOUNT_MAGIC);
  SC3E_DEMAND (r->rc >= 1);

  ++r->rc;
  return NULL;
}

sc3_error_t        *
sc3_refcount_unref (sc3_refcount_t * r, int *waslast)
{
  SC3A_CHECK (r != NULL && r->magic == SC3_REFCOUNT_MAGIC);
  SC3E_DEMAND (r->rc >= 1);
  SC3E_RETVAL (waslast, 0);

  if (--r->rc == 0) {
    r->magic = 0;
    *waslast = 1;
  }
  return NULL;
}
