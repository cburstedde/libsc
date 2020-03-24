/*
  This file is part of the SC Library, version 3.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

/** \file sc3_omp.h \ingroup sc3
 * We provide support functions to work with OpenMP.
 *
 * We provide simple wrappers to OpenMP functions as well as a synchronization
 * mechanism for \ref sc3_error_t objects encountered in parallel threads.
 *
 * \todo Check error synchronization again and TODO notes in the .c file.
 */

#ifndef SC3_OMP_H
#define SC3_OMP_H

#include <sc3_error.h>

typedef struct sc3_omp_esync
{
  int                 rcount;
  int                 ecount;
  int                 error_tid;
  sc3_error_t        *shared_error;
}
sc3_omp_esync_t;

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** Wrap the omp_get_max_threads function.
 * \return          The maximum number of threads that may be spawned.
 */
int                 sc3_omp_max_threads (void);

/** Wrap the omp_get_num_threads function.
 * \return          The current number of parallel threads.
 */
int                 sc3_omp_num_threads (void);

/** Wrap the omp_get_thread_num function.
 * \return          The number of the calling thread.
 */
int                 sc3_omp_thread_num (void);

/** Divide a contiguous range of numbers into subranges by thread.
 * Often, each MPI process works on a range within a global number of tasks.
 * The threads in each MPI process can subdivide the range among them.
 * This function computes this subrange based on the current number of threads.
 * We guarantee that the subranges are contiguous and ascending among threads.
 * They are disjoint and their union is onto the input range.
 * \param [in,out] beginr   On input, start index of the range to subdivide.
 *                          On output, start index of this thread's subrange.
 *                          If \a beginr is NULL, the function noops.
 * \param [in,out] endr     On input, end index (exclusive) of the range.
 *                          On output, end index (exclusive) of thread range.
 *                          If \a endr is NULL, the function noops.
 */
void                sc3_omp_thread_intrange (int *beginr, int *endr);

int                 sc3_omp_esync_is_clean (sc3_omp_esync_t * s);

/** Initialize OpenMP error synchronization context.
 * Must be called outside of the OpenMP parallel construct.
 */
sc3_error_t        *sc3_omp_esync_init (sc3_omp_esync_t * s);
void                sc3_omp_esync_in_critical (sc3_omp_esync_t * s,
                                               sc3_error_t ** e);
void                sc3_omp_esync (sc3_omp_esync_t * s, sc3_error_t ** e);
sc3_error_t        *sc3_omp_esync_summary (sc3_omp_esync_t * s);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_OMP_H */
