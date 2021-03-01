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
    const int           tnum = sc3_omp_num_threads ();
    const int           tid = sc3_omp_thread_num ();
    const int           ranger = *endr - *beginr;

    *endr = *beginr + sc3_intcut (ranger, tnum, tid + 1);
    *beginr += sc3_intcut (ranger, tnum, tid);
  }
}

int
sc3_omp_esync_is_clean (sc3_omp_esync_t * s)
{
  return s != NULL && s->shared_error == NULL;
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

void
sc3_omp_esync_in_critical (sc3_omp_esync_t * s, sc3_error_t ** e)
{
  if (s == NULL) {
    /* survive NULL input parameters */
    if (e != NULL && *e != NULL) {
      sc3_error_destroy (e);
    }
  }
  else if (e == NULL) {
    /* this is not the expected call convention */
    ++s->rcount;
  }
  else if (*e != NULL) {
    /* we have been called as expected */
    const int           tid = sc3_omp_thread_num ();
    char                tprefix[SC3_BUFSIZE];
    sc3_error_t        *res;

    if (s->error_tid > tid) {
      /* we are the first or lowest numbered thread to encounter an error */
      s->error_tid = tid;
    }

    /* use the incoming error as new top of the shared error stack */
    sc3_snprintf (tprefix, SC3_BUFSIZE, "T%02d", tid);
    res = sc3_error_accumulate (sc3_allocator_nocount (), &s->shared_error, e,
                                __FILE__, __LINE__, tprefix);

    /* On fatal error return, we have an internal bug and return that instead. */
    if (res != NULL) {
      ++s->rcount;
      if (sc3_error_destroy (&s->shared_error) != NULL) {
        ++s->rcount;
      }
      s->shared_error = res;
    }

    /* count a proper error added */
    ++s->ecount;
  }
}

void
sc3_omp_esync (sc3_omp_esync_t * s, sc3_error_t ** e)
{
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma omp critical (sc3_omp_esync)
  sc3_omp_esync_in_critical (s, e);
}

sc3_error_t        *
sc3_omp_esync_summary (sc3_omp_esync_t * s)
{
  sc3_error_t        *res;
  SC3A_CHECK (s != NULL);
  if (s->rcount > 0) {
    char                srcount[SC3_BUFSIZE];

    /* Unexpected (buggy) behavior is reported in addition */
    sc3_snprintf (srcount, SC3_BUFSIZE, "s->count: %d", s->rcount);
    SC3E (sc3_error_accum_kind (sc3_allocator_nocount (),
                                &s->shared_error, SC3_ERROR_FATAL,
                                __FILE__, __LINE__, srcount));
    s->rcount = 0;
  }

  /* Return the accumulated error only once */
  res = s->shared_error;
  s->ecount = 0;
  s->error_tid = sc3_omp_max_threads ();
  s->shared_error = NULL;

  /* Recommend to \ref sc3_omp_esync_init before using the struct again */
  return res;
}
