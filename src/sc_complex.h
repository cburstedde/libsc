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
  02110-1301 USA.
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

/* Using cabs on a double does NOT work right */
/* float */
#define cabsf(x)    (abs(x))
#define crealf(x)   (real(x))
#define cimagf(x)   (imag(x))
#define cargf(x)    (arg(x))
#define csqrtf(x)   (sqrt(x))
#define cpowf(x,e)  (pow(x,e))
/* double */
#define cabs(x)     (abs(x))
#define creal(x)    (real(x))
#define cimag(x)    (imag(x))
#define carg(x)     (arg(x))
#define csqrt(x)    (sqrt(x))
#define cpow(x,e)   (pow(x,e))
/* long double */
#define cabsl(x)    (abs(x))
#define creall(x)   (real(x))
#define cimagl(x)   (imag(x))
#define cargl(x)    (arg(x))
#define csqrtl(x)   (sqrt(x))
#define cpowl(x,e)  (pow(x,e))

#else

/* Splint doesn't know about these types */
#ifdef SC_SPLINT

typedef float       sc_float_complex_t;
typedef double      sc_double_complex_t;
typedef long double sc_long_double_complex_t;

#define I                       (0.)
#define cabs(x)                 (0.)
#define creal(x)                (0.)
#define cimag(x)                (0.)
#define carg(x)                 (0.)
#define csqrt(x)                (0.)
#define cpow(x,e)               (0.)

#else

#include <complex.h>

typedef float complex sc_float_complex_t;
typedef double complex sc_double_complex_t;
typedef long double complex sc_long_double_complex_t;

#endif /* !SC_SPLINT */

#endif /* !__cplusplus */

#endif /* !SC_COMPLEX_H */
