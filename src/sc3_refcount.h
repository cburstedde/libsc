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

sc3_error_t        *sc3_refcount_init_invalid (sc3_refcount_t * r);
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
