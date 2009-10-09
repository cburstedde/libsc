/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2009 Carsten Burstedde, Lucas Wilcox.

  The SC Library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with the SC Library.  If not, see <http://www.gnu.org/licenses/>.
*/

/* This header can be included by either C99 or ANSI C++ programs to allow
 * complex arithmetic to be written in a common subset.
 *
 * This header is a modififed version of the one found at
 * http://www.ddj.com/cpp/184401628?pgno=2.
 */

#ifndef SC_COMPLEX_H
#define SC_COMPLEX_H

#include <sc.h>

#ifdef __cplusplus

#include <cmath>
#include <complex>

/* *INDENT-OFF* */
typedef std::complex<float>          sc_float_complex_t;
typedef std::complex<double>         sc_double_complex_t;
typedef std::complex<long double>    sc_long_double_complex_t;
/* *INDENT-ON* */

#define fabs(x) abs(x)

#else

#include <tgmath.h>

/* Splint doesn't know about these types */
#ifdef SC_SPLINT
typedef float       sc_float_complex_t;
typedef double      sc_double_complex_t;
typedef long double sc_long_double_complex_t;
#define I                       (0.)
#define creal(x)                (0.)
#define cimag(x)                (0.)
#define carg(x)                 (0.)
#else
typedef float complex sc_float_complex_t;
typedef double complex sc_double_complex_t;
typedef long double complex sc_long_double_complex_t;
#endif

#define sc_float_complex_t(r,i) ((float)(r) + ((float)(i))*I)
#define sc_double_complex_t(r,i) ((double)(r) + ((double)(i))*I)
#define sc_long_double_complex_t(r,i) ((long double)(r) + ((long double)(i))*I)

#define real(x) creal(x)
#define imag(x) cimag(x)
#define arg(x) carg(x)

#endif

#endif /* !SC_COMPLEX_H */
