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

#include <sc_warp.h>

sc_warp_interval_t *
sc_warp_new (double r_low, double r_high)
{
  sc_warp_interval_t *root;

  SC_ASSERT (r_low <= r_high);

  root = SC_ALLOC (sc_warp_interval_t, 1);
  root->level = 0;
  root->r_low = r_low;
  root->r_high = r_high;
  root->left = root->right = NULL;

  return root;
}

void
sc_warp_destroy (sc_warp_interval_t * root)
{
  if (root->left != NULL)
    sc_warp_destroy (root->left);
  if (root->right != NULL)
    sc_warp_destroy (root->right);
  SC_FREE (root);
}

static void
sc_warp_update_interval (sc_warp_interval_t * iv,
                         int start, int end, double *r_points,
                         double r_tol, int rem_levels)
{
  int                 i_low, i_high, i_guess, i_best;
  int                 i_left_end, i_right_start;
  double              r, r_best, r_dist, r_sign, r_mid;

  SC_ASSERT (0 <= start && start < end);
  SC_ASSERT (r_points[start] >= iv->r_low);
  SC_ASSERT (r_points[end - 1] <= iv->r_high);

  SC_LDEBUGF ("Level %d interval %g %g with %d %d\n",
              rem_levels, iv->r_low, iv->r_high, start, end);

  while (start < end && r_points[start] <= iv->r_low)
    ++start;
  while (start < end && r_points[end - 1] >= iv->r_high)
    --end;
  if (start >= end || rem_levels == 0) {
    return;
  }

  if (iv->left != NULL) {
    SC_ASSERT (iv->right != NULL);
    SC_ASSERT (iv->left->r_high == iv->right->r_low);

    /* find highest point with r < r_mid, which need not exist */
    r_mid = iv->left->r_high;
    i_low = start;
    i_high = end - 1;
    while (i_low < i_high) {
      i_guess = (i_low + i_high + 1) / 2;
      r = r_points[i_guess];
      if (r < r_mid) {
        i_low = i_guess;
      }
      else {
        i_high = i_guess - 1;
      }
    }
    SC_ASSERT (i_low == i_high);
    SC_LDEBUGF ("Searched low %d %g\n", i_low, r_points[i_low]);
    if (r_points[i_low] >= r_mid) {
      /* left interval is empty */
      i_left_end = start;
    }
    else {
      i_left_end = i_low + 1;
    }
    while (i_high < end && r_points[i_high] <= r_mid) {
      ++i_high;
    }
    i_right_start = i_high;
  }
  else {
    /* find closest point to mid-interval */
    r_sign = iv->r_high - iv->r_low;
    r_best = r_mid = .5 * (iv->r_low + iv->r_high);
    i_low = start;
    i_high = end - 1;
    i_guess = i_best = -1;
    while (i_low <= i_high) {
      i_guess = (i_low + i_high + 1) / 2;
      r = r_points[i_guess];
      r_dist = r - r_mid;
      SC_LDEBUGF ("Search now %d %d with %d %g %g\n",
                  i_low, i_high, i_guess, r, r_dist);
      if (fabs (r_dist) < fabs (r_sign)) {
        r_sign = r_dist;
        r_best = r;
        i_best = i_guess;
      }
      if (r_dist < 0.) {
        i_low = i_guess + 1;
      }
      else if (r_dist > 0.) {
        i_high = i_guess - 1;
      }
      else
        break;
    }
    SC_ASSERT (i_guess >= start && i_guess < end);
    SC_ASSERT (i_best >= start && i_best < end);
    SC_LDEBUGF ("Searched %d %d with %d %g %g\n",
                i_low, i_high, i_best, r_best, r_sign);

    iv->left = SC_ALLOC (sc_warp_interval_t, 1);
    iv->left->r_low = iv->r_low;
    iv->left->level = iv->level + 1;
    iv->left->left = iv->left->right = NULL;
    iv->right = SC_ALLOC (sc_warp_interval_t, 1);
    iv->right->r_high = iv->r_high;
    iv->right->level = iv->level + 1;
    iv->right->left = iv->right->right = NULL;

    r_dist = r_tol * (iv->r_high - iv->r_low);
    if (fabs (r_sign) < r_dist) {
      SC_LDEBUG ("New matching point\n");

      iv->left->r_high = iv->right->r_low = r_best;
      i_left_end = i_best;
      i_right_start = i_best + 1;
    }
    else {
      SC_LDEBUGF ("No matching point error %g %g\n", fabs (r_sign), r_dist);

      if (r_sign < 0) {
        i_left_end = i_right_start = i_best + 1;
        iv->left->r_high = iv->right->r_low = r_mid;    /* - r_dist; */
      }
      else {
        i_left_end = i_right_start = i_best;
        iv->left->r_high = iv->right->r_low = r_mid;    /* + r_dist; */
      }
    }
  }

  if (start < i_left_end)
    sc_warp_update_interval (iv->left, start, i_left_end, r_points,
                             r_tol, rem_levels - 1);
  if (i_right_start < end)
    sc_warp_update_interval (iv->right, i_right_start, end, r_points,
                             r_tol, rem_levels - 1);
}

void
sc_warp_update (sc_warp_interval_t * root, int num_points, double *r_points,
                double r_tol, int max_level)
{
  if (num_points <= 0)
    return;

  SC_ASSERT (r_points != NULL);
  SC_ASSERT (0 <= r_tol && r_tol <= 1.);

  /* call interval recursion */
  sc_warp_update_interval (root, 0, num_points, r_points, r_tol, max_level);
}

void
sc_warp_write (sc_warp_interval_t * root, FILE * nout)
{
  if (root->left == NULL) {
    SC_ASSERT (root->right == NULL);
    fprintf (nout, "Warp interval level %d [%g %g] length %g\n",
             root->level, root->r_low, root->r_high,
             root->r_high - root->r_low);
  }
  else {
    sc_warp_write (root->left, nout);
    sc_warp_write (root->right, nout);
  }
}
