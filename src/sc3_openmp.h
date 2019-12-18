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

/** \file sc3_openmp.h
 */

#ifndef SC3_OPENMP_H
#define SC3_OPENMP_H

#include <sc3.h>

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

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_OPENMP_H */
