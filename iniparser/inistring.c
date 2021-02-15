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

#include "inistring.h"
#include <stdarg.h>

void
ini_strcopy (char *dest, size_t size, const char *src)
{
  ini_snprintf (dest, size, "%s", src);
}

void
ini_snprintf (char *str, size_t size, const char *fmt, ...)
{
  int                 retval;
  va_list             ap;

  /* If there is no space, we do not access the buffer at all.
     Further down we expect it to be at least 1 byte wide */
  if (str == NULL || size == 0) {
    return;
  }

  /* Writing this function just to catch the return value.
     Avoiding -Wnoformat-truncation gcc option this way */
  va_start (ap, fmt);
  retval = vsnprintf (str, size, fmt, ap);
  if (retval < 0) {
    str[0] = '\0';
  }
  /* We do not handle truncation, since it is expected in our design. */
  va_end (ap);
}
