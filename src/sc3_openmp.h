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

/** \file sc3_openmp.h
 */

#ifndef SC3_OPENMP_H
#define SC3_OPENMP_H

#include <sc3_error.h>

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

int                 sc3_openmp_get_max_threads (void);
int                 sc3_openmp_get_num_threads (void);
int                 sc3_openmp_get_thread_num (void);

sc3_error_t        *sc3_openmp_esync_pre_critical
  (int *rcount, int *ecount, int *error_tid, sc3_error_t ** shared_error);

void                sc3_openmp_esync_in_critical
  (sc3_error_t * e, int *rcount, int *ecount,
   int *error_tid, sc3_error_t ** shared_error);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_OPENMP_H */
