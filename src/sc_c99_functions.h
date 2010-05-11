/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System

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

#ifndef SC_C99_FUNCTIONS_H
#define SC_C99_FUNCTIONS_H

#include <sc.h>

/* Supply some C99 prototypes */
#ifndef __cplusplus

/* File function prototypes */
int                 fsync (int fd);
int                 fileno (FILE * stream);
FILE               *fopen (const char *path, const char *mode);
FILE               *fdopen (int fd, const char *mode);
int                 mkstemp (char *template);

/* Long size integer support */
intmax_t            imaxabs (intmax_t);

#endif /* !__cplusplus */

/* Supply defines and prototypes that splint doesn't know about */
#ifdef SC_SPLINT

#ifndef UINT32_MAX
#define UINT32_MAX  (4294967295U)
#endif
#define restrict

long long int       strtoll (const char *nptr, char **endptr, int base);
intmax_t            imaxabs (intmax_t a);

#endif /* SC_SPLINT */

#endif /* SC_C99_FUNCTIONS_H  */
