/* Declarations of various C99 functions
   Copyright (C) 2004, 2006, 2007 Free Software Foundation, Inc.

This file is part of the GNU Fortran 95 runtime library (libgfortran).

Libgfortran is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

Libgfortran is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with libgfortran; see the file COPYING.LIB.  If not,
write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.  */

/* As a special exception, if you link this library with other files,
   some of which are compiled with GCC, to produce an executable,
   this library does not by itself cause the resulting executable
   to be covered by the GNU General Public License.
   This exception does not however invalidate any other reasons why
   the executable file might be covered by the GNU General Public License.  */

/* made some changes to preprocessor macros for libsc */

#ifndef SC_C99_FUNCTIONS_H
#define SC_C99_FUNCTIONS_H

#ifndef SC_H
#error "sc.h should be included before this header file"
#endif

/* Supply some C99 prototypes */
#ifndef __cplusplus

/* Gamma-related prototypes */
double              tgamma (double);
double              trunc (double);

#endif /* !__cplusplus */

/* Supply defines and prototypes that splint doesn't know about */
#ifdef ACX_SPLINT

#ifndef UINT32_MAX
#define UINT32_MAX  (4294967295U)
#endif

long long int       strtoll (const char *nptr, char **endptr, int base);

#endif /* ACX_SPLINT */

#endif /* SC_C99_FUNCTIONS_H  */
