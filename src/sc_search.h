/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2010 Carsten Burstedde, Lucas Wilcox.

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

#ifndef SC_SEARCH_H
#define SC_SEARCH_H

#include <sc.h>

SC_EXTERN_C_BEGIN;

/** Find the branch of a tree that is biased towards a target.
 * We assume a binary tree of depth maxlevel and 0 <= target < 2**maxlevel.
 * We search the branch towards the target on 0 <= level <= maxlevel.
 * The branch number on level is specified by 0 <= interval < 2**level.
 *
 * \return          Branch position with 0 <= position <= 2**maxlevel.
 */
int                 sc_search_bias (int maxlevel, int level,
				    int interval, int target);

/** Find lowest position k in a sorted array such that array[k] >= target.
 * \param [in]  target  The target lower bound to binary search for.
 * \param [in]  array   The 64bit integer array to binary search in.
 * \param [in]  size    The number of int64_t's in the array.
 * \param [in]  guess   Initial array position to look at.
 * \return  Returns the matching position
 *          or -1 if array[size-1] < target or if size == 0.
 */
ssize_t             sc_search_lower_bound64 (int64_t target,
                                             const int64_t * array,
                                             size_t size, size_t guess);

SC_EXTERN_C_END;

#endif /* !SC_SEARCH_H */
