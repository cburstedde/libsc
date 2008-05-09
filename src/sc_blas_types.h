/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2007,2008 Carsten Burstedde, Lucas Wilcox.

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

#ifndef SC_BLAS_TYPES_H
#define SC_BLAS_TYPES_H

typedef int         sc_bint_t;  /* Integer type for all of the blas calls */

typedef enum sc_trans
{
  SC_NO_TRANS,
  SC_TRANS,
  SC_TRANS_ANCHOR
}
sc_trans_t;

typedef enum sc_uplo
{
  SC_UPPER,
  SC_LOWER,
  SC_UPLO_ANCHOR
}
sc_uplo_t;

typedef enum sc_cmach
{
  SC_CMACH_EPS,                 /* relative machine precision */
  SC_CMACH_SFMIN,               /* safe minimum, such that 1/sfmin does not
                                 * overflow
                                 */
  SC_CMACH_BASE,                /* base of the machine */
  SC_CMACH_PREC,                /* eps*base */
  SC_CMACH_T,                   /* number of (base) digits in the mantissa */
  SC_CMACH_RND,                 /* 1.0 when rounding occurs in addition,
                                 * 0.0 otherwise
                                 */
  SC_CMACH_EMIN,                /* minimum exponent before (gradual)
                                 * underflow
                                 */
  SC_CMACH_RMIN,                /* underflow threshold - base**(emin-1) */
  SC_CMACH_EMAX,                /* largest exponent before overflow */
  SC_CMACH_RMAX,                /* overflow threshold  - (base**emax)*(1-eps) */
  SC_CMACH_ANCHOR
}
sc_cmach_t;

#endif /* !SC_BLAS_TYPES_H */
