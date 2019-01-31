/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System
  Additional copyright (C) 2011 individual authors

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

#include <sc_random.h>

#define SC_RANDOM_ITER 4
static const uint32_t sc_rand_rc1[SC_RANDOM_ITER] =
  { 0xbaa96887U, 0x1e17d32cU, 0x03bcdc3cU, 0x0f33d1b2U };
static const uint32_t sc_rand_rc2[SC_RANDOM_ITER] =
  { 0x4b0f3b58U, 0xe874f0c3U, 0x6955c5a6U, 0x55a7ca46U };
static const double iump = 1. / (UINT32_MAX + 1.);

/** Run different versions of Poisson PRNG and compare them.
 * Going through several different mean values as specified.
 * This function returns nothing, just outputs by sc_infof.
 * \param [in,out] state        Internal state of random number generator.
 * \param [in] mean_min         Minimum mean value to be tested.
 * \param [in] mean_max         Maximum mean value to be tested.
 * \param [in] mean_steps       Varying mean in so many subintervals.
 * \param [in] n                Number of draws.
 */
void                sc_rand_test_poisson (sc_rand_state_t * state,
                                          double mean_min,
                                          double mean_max,
                                          int mean_steps, int n);

double
sc_rand (sc_rand_state_t * state)
{
  int                 i;
  uint32_t            a, b, c;
  uint32_t            lword, rword;
  uint32_t            ltemp, htemp;
  uint32_t            swap;

  SC_ASSERT (state != NULL);

  lword = *state >> 32;
  rword = *state & 0xffffffff;
  for (i = 0; i < SC_RANDOM_ITER; ++i) {
    a = (swap = rword) ^ sc_rand_rc1[i];
    htemp = a >> 16;
    ltemp = a & 0xffff;
    b = ~(htemp * htemp) + ltemp * ltemp;
    c = (((b & 0xffff) << 16) | (b >> 16)) ^ sc_rand_rc2[i];
    rword = lword ^ (htemp * ltemp + c);
    lword = swap;
  }
  ++(*state);

  return rword * iump;
}

int
sc_rand_small (sc_rand_state_t * state, double d)
{
  /*
     We have a positive double variable potentially very close to zero.
     We want to draw random variables that are 1 with this probability.
   */

  const double        rfac = 13.;
  const double        frac = 1. / rfac;

  if (d <= 0.) {
    return 0;
  }

  while (d < frac) {
    if (sc_rand (state) >= frac) {
      return 0;
    }
    d *= rfac;
  }
  return sc_rand (state) < d;
}

static int
sc_rand_poisson_knuth (sc_rand_state_t * state, double mean)
{
  int                 n;
  double              expmm;
  double              p;

  expmm = exp (-mean);
  n = -1;
  p = 1.;
  do {
    ++n;
    p *= sc_rand (state);
  }
  while (p > expmm);
  return n;
}

int
sc_rand_poisson (sc_rand_state_t * state, double mean)
{
  double              sq, lnmean, correct;
  double              p, t, x;

  /* use Knuth's method for not-so-large mean values */
  if (mean < 12.) {
    return sc_rand_poisson_knuth (state, mean);
  }

  /* use rejection method */
  sq = sqrt (2. * mean);
  lnmean = log (mean);
  correct = mean * lnmean - lgamma (mean + 1.);
  do {
    /* draw from the majorant distribution */
    do {
      t = tan (M_PI * sc_rand (state));
      x = sq * t + mean;
    }
    while (x < 0.);
    x = floor (x);
    p = .9 * (1. + t * t) * exp (x * lnmean - lgamma (x + 1.) - correct);
    SC_ASSERT (p < 1.);
  }
  while (sc_rand (state) > p);
  return (int) x;
}

static int
draw_poisson_cumulative (sc_rand_state_t * state, double *cumud, int ncumu)
{
  int                 lo, hi, guess;
  double              p = sc_rand (state);

  /* find guess such that cumud[guess] <= p < cumud[guess + 1] */

  SC_ASSERT (ncumu >= 2);
  lo = 0;
  hi = ncumu - 2;
  for (;;) {
    SC_ASSERT (0 <= lo && lo <= hi && hi < ncumu - 1);
    guess = (lo + hi) / 2;
    if (p < cumud[guess]) {
      hi = guess - 1;
      continue;
    }
    if (p >= cumud[guess + 1]) {
      lo = guess + 1;
      continue;
    }
    SC_ASSERT (0 <= guess && guess < ncumu - 1);
    SC_ASSERT (cumud[guess] <= p && p < cumud[guess + 1]);
    return guess;
  }
}

static void
test_poisson_mean (sc_rand_state_t * state, double mean, int n)
{
  int                 i, k;
  int                 ncumu;
  int                 draw[3];
  double              p, cp;
  double             *cumud;
  double              sumsv[3], sumsq[3];
  double              meanv[3], varia[3];

  SC_INFOF ("Computing Poisson test for mean %g and %d draws\n", mean, n);

  /* go out five standard deviations */
  ncumu = (int) ceil (mean + 5. * sqrt (mean));
  ncumu = SC_MAX (ncumu, 2);
  SC_INFOF ("Computing %d cumulative terms\n", ncumu);

  /* explicitly compute the cumulative Poisson distribution */
  cumud = SC_ALLOC (double, ncumu);
  cumud[0] = 0.;
  cp = p = exp (-mean);
  for (i = 1; i < ncumu - 1; ++i) {
    cumud[i] = cp;
#if 0
    if (i < 10) {
      SC_INFOF ("Mean %g i %d p %g cumu %g\n", mean, i, p, cp);
    }
#endif
    cp += (p *= mean / i);
  }
  SC_ASSERT (cumud[i - 1] < 1.);
  cumud[i] = 1.;

  /* draw n times */
  for (k = 0; k < 3; ++k) {
    sumsv[k] = sumsq[k] = 0.;
  }
  for (i = 0; i < n; ++i) {
    draw[0] = draw_poisson_cumulative (state, cumud, ncumu);
    draw[1] = sc_rand_poisson_knuth (state, mean);
    draw[2] = sc_rand_poisson (state, mean);
    for (k = 0; k < 3; ++k) {
      p = (double) draw[k];
      sumsv[k] += p;
      sumsq[k] += p * p;
    }
  }
  for (k = 0; k < 3; ++k) {
    /* compute sampled mean and variance */
    meanv[k] = sumsv[k] / n;
    varia[k] = sumsq[k] / n - meanv[k] * meanv[k];

    /* subtract the known mean and variance, relative */
    meanv[k] = meanv[k] / mean - 1.;
    varia[k] = varia[k] / mean - 1.;
  }

  /* print results */
  for (k = 0; k < 3; ++k) {
    SC_INFOF ("Method %d dev mean %g variance %g\n", k, meanv[k], varia[k]);
  }

  /* cleanup */
  SC_FREE (cumud);
}

void
sc_rand_test_poisson (sc_rand_state_t * state, double mean_min,
                      double mean_max, int mean_steps, int n)
{
  int                 i;
  double              mh;

  SC_ASSERT (0. < mean_min && mean_min <= mean_max);
  SC_ASSERT (mean_steps > 0);
  SC_ASSERT (n > 0);
  mh = (mean_max - mean_min) / mean_steps;

  /* test a series of mean values */
  for (i = 0; i <= mean_steps; ++i) {
    test_poisson_mean (state, mean_min + i * mh, n);
  }
}
