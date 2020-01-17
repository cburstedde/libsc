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

/** \file sc3_base.h
 */

#ifndef SC3_BASE_H
#define SC3_BASE_H

#include <sc3_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SC3_BUFSIZE 512
#define SC3_BUFZERO(b) do { memset (b, 0, SC3_BUFSIZE); } while (0)
#define SC3_BUFCOPY(b,s) \
          do { (void) snprintf (b, SC3_BUFSIZE, "%s", s); } while (0)

#define SC3_MALLOC(typ,nmemb) ((typ *) malloc ((nmemb) * sizeof (typ)))
#define SC3_FREE(p) do { free (p); } while (0)

#define SC3_ISPOWOF2(a) ((a) > 0 && ((a) & ((a) - 1)) == 0)

#define SC3_MIN(in1,in2) ((in1) <= (in2) ? (in1) : (in2))
#define SC3_MAX(in1,in2) ((in1) >= (in2) ? (in1) : (in2))

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

char               *sc3_basename (char *path);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_BASE_H */
