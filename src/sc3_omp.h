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
 * If we configure without --enable-openmp, we report a single thread.
 *
 * \todo Check error synchronization again and TODO notes in the .c file.
 */

#ifndef SC3_OMP_H
#define SC3_OMP_H

#include <sc3_error.h>

/** Collect error synchronization information in the master thread.
 * Typically, it is initialized by \ref sc3_omp_esync_init *before*
 * a #`pragma omp parallel`.
 * Inside the parallel region, a thread may create an \ref sc3_error_t.
 * It may call \ref sc3_omp_esync on it, which is procected internally
 * by #`pragma omp critical`, to globally synchronize the error status.
 * Synchronization means that individual per-thread errors are deallocated
 * until only one remains.
 * *After* the parallel region, the function \ref sc3_omp_esync_summary
 * reports the remaining error, or NULL if there was none.
 */
typedef struct sc3_omp_esync
{
  int                 rcount;   /**< Count problems freeing errors.
                                 * These *should* not occur. */
  int                 ecount;   /**< Count the errors among the threads. */
  int                 error_tid;        /**< Thread number of the remaining
                                         * error object. */
  sc3_error_t        *shared_error;     /**< Remaining error object. */
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

/** Query a synchronization struct to hold no error.
 * \param [in] s    Pointer to initialized \ref sc3_omp_esync_t struct.
 * \return          True iff \a s is not NULL and its shared error is NULL.
 */
int                 sc3_omp_esync_is_clean (sc3_omp_esync_t * s);

/** Initialize OpenMP error synchronization context.
 * Must be called before the OpenMP parallel construct.
 * \param [out] s   This pointer to existing memory must not be NULL.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_omp_esync_init (sc3_omp_esync_t * s);

/** This version of \ref sc3_omp_esync must be called inside
 * a #`pragma omp critical` region to avoid data corruption.
 * \param [in,out] s    Initialized by \ref sc3_omp_esync_init.
 * \param [in,out] e    On input, error encountered in thread.
 *                      The pointer to the pointer must not be NULL.
 *                      The pointed-to error may be NULL or not.
 *                      If not NULL, then this function takes ownership and
 *                      integrates it into \a s.  Becomes NULL on output.
 */
void                sc3_omp_esync_in_critical (sc3_omp_esync_t * s,
                                               sc3_error_t ** e);

/** This function establishes a #`pragma omp critical` region
 * and collects the error reported by the present thread.
 * \param [in,out] s    Initialized by \ref sc3_omp_esync_init.
 * \param [in,out] e    On input, error encountered in thread.
 *                      This function takes ownership and integrates it
 *                      into \a s.  Becomes NULL on output.
 */
void                sc3_omp_esync (sc3_omp_esync_t * s, sc3_error_t ** e);

/** Return the error collected in the synchronization stuct.
 * Ownership of the error is transferred to the caller.
 * Call this function *after* the end of the parallel region, exactly once.
 * \param [in] s        Initialized synchronization struct.
 * \return              Possible assertion on bad usage,
 *                      otherwise the error in \a s, be it NULL or not.
 */
sc3_error_t        *sc3_omp_esync_summary (sc3_omp_esync_t * s);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_OMP_H */
