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

/** \file sc3_log.h
 */

#ifndef SC3_LOG_H
#define SC3_LOG_H

#include <sc3_mpi.h>

typedef struct sc3_log sc3_log_t;

typedef enum sc3_log_role
{
  SC3_LOG_ANY,
  SC3_LOG_THREAD0,
  SC3_LOG_PROCESS0,
  SC3_LOG_ROLE_LAST
}
sc3_log_role_t;

typedef enum sc3_log_level
{
  SC3_LOG_NOISE,
  SC3_LOG_DEBUG,
  SC3_LOG_INFO,
  SC3_LOG_PROGRAM,
  SC3_LOG_ERROR,
  SC3_LOG_SILENT,
  SC3_LOG_LEVEL_LAST
}
sc3_log_level_t;

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

int                 sc3_log_is_valid (sc3_log_t * log, char *reason);
int                 sc3_log_is_new (sc3_log_t * log, char *reason);
int                 sc3_log_is_setup (sc3_log_t * log, char *reason);

sc3_error_t        *sc3_log_new (sc3_allocator_t * lator, sc3_log_t ** logp);
sc3_error_t        *sc3_log_set_level (sc3_log_t * log,
                                       sc3_log_level_t level);
sc3_error_t        *sc3_log_set_comm (sc3_log_t * log,
                                      sc3_MPI_Comm_t mpicomm);
sc3_error_t        *sc3_log_set_file (sc3_log_t * log,
                                      FILE * file, int call_fclose);
sc3_error_t        *sc3_log_set_indent (sc3_log_t * log, int indent);

sc3_error_t        *sc3_log_setup (sc3_log_t * log);
sc3_error_t        *sc3_log_unref (sc3_log_t ** logp);
sc3_error_t        *sc3_log_destroy (sc3_log_t ** logp);

void                sc3_log (sc3_log_t * log, int depth,
                             sc3_log_role_t role, sc3_log_level_t level,
                             const char *msg);
void                sc3_logf (sc3_log_t * log, int depth,
                              sc3_log_role_t role, sc3_log_level_t level,
                              const char *fmt, ...)
  __attribute__ ((format (printf, 5, 6)));
void                sc3_logv (sc3_log_t * log, int depth,
                              sc3_log_role_t role, sc3_log_level_t level,
                              const char *fmt, va_list ap);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_LOG_H */
