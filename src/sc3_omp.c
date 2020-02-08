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

#include <sc3_omp.h>
#ifdef SC_ENABLE_OPENMP
#include <omp.h>
#endif

int
sc3_omp_max_threads (void)
{
#ifndef SC_ENABLE_OPENMP
  return 1;
#else
  return omp_get_max_threads ();
#endif
}

int
sc3_omp_num_threads (void)
{
#ifndef SC_ENABLE_OPENMP
  return 1;
#else
  return omp_get_num_threads ();
#endif
}

int
sc3_omp_thread_num (void)
{
#ifndef SC_ENABLE_OPENMP
  return 0;
#else
  return omp_get_thread_num ();
#endif
}

void
sc3_omp_thread_intrange (int *beginr, int *endr)
{
  if (beginr != NULL && endr != NULL) {
    const int              tnum = sc3_omp_num_threads ();
    const int              tid = sc3_omp_thread_num ();
    const int              ranger = *endr - *beginr;

    *endr = *beginr + sc3_intcut (ranger, tnum, tid + 1);
    *beginr += sc3_intcut (ranger, tnum, tid);
  }
}

sc3_error_t        *
sc3_omp_esync_init (sc3_omp_esync_t * s)
{
  SC3A_CHECK (s != NULL);
  s->rcount = 0;
  s->ecount = 0;
  s->error_tid = sc3_omp_max_threads ();
  s->shared_error = NULL;
  return NULL;
}

sc3_error_t        *
sc3_omp_esync_critical (sc3_omp_esync_t * s, sc3_error_t ** e)
{
  /* this function is written to survive NULL input parameters */
  if (s == NULL) {
    if (e != NULL && *e != NULL) {
      sc3_error_t        *reterr = *e;
      *e = NULL;
      return reterr;
    }
    return NULL;
  }

  /* the error synchronization context exists */
  if (e == NULL) {
    /* this is not the expected call convention */
    ++s->rcount;
  }
  else if (*e != NULL) {
    int                 tid = sc3_omp_thread_num ();
    /* we have been called as expected */

    if (s->shared_error == NULL) {
      /* we are the first thread to encounter an error */
      s->error_tid = tid;
      s->shared_error = *e;
    }
    else {
      /* some other thread had set an error */
      if (s->error_tid > tid) {
        /* we are now the lowest numbered sane error thread */

        /* TODO stack error instead of destroying the previous one */
        if (sc3_error_destroy (&s->shared_error) != NULL) {
          ++s->rcount;
        }
        s->error_tid = tid;
        s->shared_error = *e;
      }
      else {
        /* the other thread has higher priority so we remove our error */

        /* TODO stack this thread's error instead of destroying it */
        if (sc3_error_destroy (e) != NULL) {
          ++s->rcount;
        }
      }
    }
    *e = NULL;
    ++s->ecount;
  }
  return s->shared_error;
}
