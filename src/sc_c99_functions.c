/* Implementation of various C99 functions 
   Copyright (C) 2004 Free Software Foundation, Inc.

This file is part of the GNU Fortran 95 runtime library (libgfortran).

Libgfortran is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

In addition to the permissions in the GNU General Public License, the
Free Software Foundation gives you unlimited permission to link the
compiled version of this file into combinations with other programs,
and to distribute those combinations without any restriction coming
from the use of this file.  (The General Public License restrictions
do apply in other respects; for example, they cover modification of
the file, and distribution when not linked into a combine
executable.)

Libgfortran is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public
License along with libgfortran; see the file COPYING.  If not,
write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.  */

/* This function was taken from c99_functions.c in libfortran, see the
 * license above
 */

#include <sc_config.h>

#if !defined(SC_HAVE_TGAMMA)
#define SC_HAVE_TGAMMA 1

extern double tgamma (double); 

/* Fallback tgamma() function. Uses the algorithm from
   http://www.netlib.org/specfun/gamma and references therein.  */

#undef SQRTPI
#define SQRTPI 0.9189385332046727417803297

#undef PI
#define PI 3.1415926535897932384626434

double
tgamma (double x)
{
  int i, n, parity;
  double fact, res, sum, xden, xnum, y, y1, ysq, z;

  static double p[8] = {
    -1.71618513886549492533811e0,  2.47656508055759199108314e1,
    -3.79804256470945635097577e2,  6.29331155312818442661052e2,
     8.66966202790413211295064e2, -3.14512729688483675254357e4,
    -3.61444134186911729807069e4,  6.64561438202405440627855e4 };

  static double q[8] = {
    -3.08402300119738975254353e1,  3.15350626979604161529144e2,
    -1.01515636749021914166146e3, -3.10777167157231109440444e3,
     2.25381184209801510330112e4,  4.75584627752788110767815e3,
    -1.34659959864969306392456e5, -1.15132259675553483497211e5 };

  static double c[7] = {             -1.910444077728e-03,
     8.4171387781295e-04,            -5.952379913043012e-04,
     7.93650793500350248e-04,        -2.777777777777681622553e-03,
     8.333333333333333331554247e-02,  5.7083835261e-03 };

  static const double xminin = 2.23e-308;
  static const double xbig = 171.624;
  static const double xnan = __builtin_nan ("0x0"), xinf = __builtin_inf ();
  static double eps = 0;
  
  if (eps == 0)
    eps = nextafter(1., 2.) - 1.;

  parity = 0;
  fact = 1;
  n = 0;
  y = x;

  if (__builtin_isnan (x))
    return x;

  if (y <= 0)
    {
      y = -x;
      y1 = trunc(y);
      res = y - y1;

      if (res != 0)
	{
	  if (y1 != trunc(y1*0.5l)*2)
	    parity = 1;
	  fact = -PI / sin(PI*res);
	  y = y + 1;
	}
      else
	return x == 0 ? copysign (xinf, x) : xnan;
    }

  if (y < eps)
    {
      if (y >= xminin)
        res = 1 / y;
      else
	return xinf;
    }
  else if (y < 13)
    {
      y1 = y;
      if (y < 1)
	{
	  z = y;
	  y = y + 1;
	}
      else
	{
	  n = (int)y - 1;
	  y = y - n;
	  z = y - 1;
	}

      xnum = 0;
      xden = 1;
      for (i = 0; i < 8; i++)
	{
	  xnum = (xnum + p[i]) * z;
	  xden = xden * z + q[i];
	}

      res = xnum / xden + 1;

      if (y1 < y)
        res = res / y1;
      else if (y1 > y)
	for (i = 1; i <= n; i++)
	  {
	    res = res * y;
	    y = y + 1;
	  }
    }
  else
    {
      if (y < xbig)
	{
	  ysq = y * y;
	  sum = c[6];
	  for (i = 0; i < 6; i++)
	    sum = sum / ysq + c[i];

	  sum = sum/y - y + SQRTPI;
	  sum = sum + (y - 0.5) * log(y);
	  res = exp(sum);
	}
      else
	return x < 0 ? xnan : xinf;
    }

  if (parity)
    res = -res;
  if (fact != 1)
    res = fact / res;

  return res;
}
#endif

/* *INDENT-ON* */
