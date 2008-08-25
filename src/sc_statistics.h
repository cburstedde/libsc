/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2008 Carsten Burstedde, Lucas Wilcox.

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

#ifndef SC_STATISTICS_H
#define SC_STATISTICS_H

#ifndef SC_H
#error "sc.h should be included before this header file"
#endif

typedef struct sc_statinfo
{
  bool                dirty;    /* only update stats if this is true */
  long                count;    /* inout, global count is 52bit accurate */
  double              sum_values, sum_squares, min, max;        /* inout */
  int                 min_at_rank, max_at_rank; /* out */
  double              average, variance, standev;       /* out */
  double              variance_mean, standev_mean;      /* out */
  const char         *variable; /* name of the variable for output */
}
sc_statinfo_t;

typedef struct sc_flopinfo
{
  double              seconds;  /* time from MPI_Wtime() */

  float               irtime;   /* input real time */
  float               iptime;   /* input process time */
  long long           iflpops;  /* input floating point operations */
  float               imflops;  /* input MFlop/s rate */

  float               rtime;    /* real time */
  float               ptime;    /* process time */
  long long           flpops;   /* floating point operations */
  float               mflops;   /* MFlop/s rate */
}
sc_flopinfo_t;

/**
 * Populate a sc_statinfo_t structure assuming count=1 and mark it dirty.
 */
void                sc_statinfo_set1 (sc_statinfo_t * stats,
                                      double value, const char *variable);

/**
 * Compute global average and standard deviation.
 * Only updates dirty variables. Then removes the dirty flag.
 * \param [in]     mpicomm   MPI communicator to use.
 * \param [in]     nvars     Number of variables to be examined.
 * \param [in,out] stats     Set of statisitcs for each variable.
 * On input, set the following fields for each variable separately.
 *    count         Number of values for each process.
 *    sum_values    Sum of values for each process.
 *    sum_squares   Sum of squares for each process.
 *    min, max      Minimum and maximum of values for each process.
 *    variable      String describing the variable, or NULL.
 * On output, the fields have the following meaning.
 *    count                        Global number of values.
 *    sum_values                   Global sum of values.
 *    sum_squares                  Global sum of squares.
 *    min, max                     Global minimum and maximum values.
 *    min_at_rank, max_at_rank     The ranks that attain min and max.
 *    average, variance, standev   Global statistical measures.
 *    variance_mean, standev_mean  Statistical measures of the mean.
 */
void                sc_statinfo_compute (MPI_Comm mpicomm, int nvars,
                                         sc_statinfo_t * stats);

/**
 * Version of sc_statistics_statistics that assumes count=1.
 * On input, the field sum_values needs to be set to the value
 * and the field variable must contain a valid string or NULL.
 * Only updates dirty variables. Then removes the dirty flag.
 */
void                sc_statinfo_compute1 (MPI_Comm mpicomm, int nvars,
                                          sc_statinfo_t * stats);

/**
 * Print measured statistics. Should be called on 1 core only.
 * \param [in] package_id       Registered package id or -1.
 * \param [in] log_priority     Log priority for output according to sc.h.
 * \param [in] full             Print full information for every variable.
 * \param [in] summary          Print summary information all on 1 line.
 */
void                sc_statinfo_print (int package_id, int log_priority,
                                       int nvars, sc_statinfo_t * stats,
                                       bool full, bool summary);

/**
 * Start counting times and flops.
 * The semantic for sc_flopinfo_start and sc_flopinfo_stop
 * are changed so that
 *  sc_flopinfo_start (fi);
 *  ... flop work
 *  sc_flopinfo_stop (fi);
 * will only time the work in between start and stop.
 * \param [out] irtime    Not defined
 * \param [out] iptime    Not defined
 * \param [out] iflpops   Not defined
 * \param [out] imflops   Not defined
 */
void                sc_papi_start (float *irtime, float *iptime,
                                   long long *iflpops, float *imflops);

/**
 * Start counting times and flops.
 * \param [out] fi   Members will be initialized.
 */
void                sc_flopinfo_start (sc_flopinfo_t * fi);

/**
 * Compute the times, flops and flop rate since the corresponding _start call.
 * The semantic for sc_flopinfo_start and sc_flopinfo_stop
 * are changed so that
 *  sc_flopinfo_start (fi);
 *  ... flop work
 *  sc_flopinfo_stop (fi);
 * will only time the work in between start and stop.
 * \param [out]    rtime    Real time.
 * \param [out]    ptime    Process time.
 * \param [out]    flpops   Floating point operations.
 * \param [out]    mflops   Flop/s rate.
 */
void                sc_papi_stop (float *rtime, float *ptime,
                                  long long *flpops, float *mflops);

/**
 * Compute the times, flops and flop rate since the corresponding _start call.
 * \param [in,out] fi   Flop info structure.
 */
void                sc_flopinfo_stop (sc_flopinfo_t * fi);

#endif /* !SC_STATISTICS_H */
