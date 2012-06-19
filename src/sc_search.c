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

#include <sc_search.h>

int
sc_search_bias (int maxlevel, int level, int interval, int target)
{
  const int           left = interval << (maxlevel - level);
  const int           right = left + (1 << (maxlevel - level));
  int                 result;

  SC_ASSERT (0 <= level && level <= maxlevel);
  SC_ASSERT (0 <= interval && interval < 1 << level);
  SC_ASSERT (0 <= left && left < right && right <= 1 << maxlevel);

  result = target < left ? left : target >= right ? right - 1 :
    left + (target & ((1 << (maxlevel - level)) - 1));

  SC_ASSERT (left <= result && result < right);
  SC_ASSERT (0 <= result && result < 1 << maxlevel);

  return result;
}

ssize_t
sc_search_lower_bound64 (int64_t target, const int64_t * array,
                         size_t nmemb, size_t guess)
{
  size_t              k_low, k_high;
  int64_t             cur;

  if (nmemb == 0) {
    return -1;
  }

  k_low = 0;
  k_high = nmemb - 1;
  for (;;) {
    SC_ASSERT (k_low <= k_high);
    SC_ASSERT (k_low < nmemb && k_high < nmemb);
    SC_ASSERT (k_low <= guess && guess <= k_high);

    /* compare two quadrants */
    cur = array[guess];

    /* check if guess is higher or equal target and there's room below it */
    if (target <= cur && (guess > 0 && target <= array[guess - 1])) {
      k_high = guess - 1;
      guess = (k_low + k_high + 1) / 2;
      continue;
    }

    /* check if guess is lower than target */
    if (target > cur) {
      k_low = guess + 1;
      if (k_low > k_high) {
        return -1;
      }
      guess = (k_low + k_high) / 2;
      continue;
    }

    /* otherwise guess is the correct position */
    break;
  }

  SC_ASSERT (0 <= guess && guess < nmemb);
  SC_ASSERT (array[guess] >= target);
  SC_ASSERT (guess == 0 || array[guess - 1] < target);
  return (ssize_t) guess;
}
