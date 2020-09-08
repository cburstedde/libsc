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

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** Opaque object to encapsulate options to the logging mechanism. */
typedef struct sc3_log sc3_log_t;

/** We may log per root MPI rank, for each MPI process, or for each thread. */
typedef enum sc3_log_role
{
  SC3_LOG_ANY,
  SC3_LOG_THREAD0,
  SC3_LOG_PROCESS0,
  SC3_LOG_ROLE_LAST
}
sc3_log_role_t;

/** Log level or priority.  Used to ignore messages of low priority. */
typedef enum sc3_log_level
{
  SC3_LOG_NOISE,        /**< Anything at all and all sorts of nonsense */
  SC3_LOG_DEBUG,        /**< Information only useful for debugging.
                             Too much to be acceptable for production runs */
  SC3_LOG_INFO,         /**< Detailed, but still acceptable for production */
  SC3_LOG_TOP,          /**< Sparse flow logging, for toplevel functions */
  SC3_LOG_ESSENTIAL,    /**< Couple lines per program: options, stats */
  SC3_LOG_ERROR,        /**< Errors by misusage, internal bugs, I/O */
  SC3_LOG_SILENT,       /**< This log level will not print anything */
  SC3_LOG_LEVEL_LAST
}
sc3_log_level_t;

/* *INDENT-OFF* */
/** Prototype for the user-selectable output function.
 * This function does not need to decide whether to log based on role or level.
 * This is done before calling this function, or not calling it depending.
 * This function is just responsible for formatting, if desired, and output.
 */
typedef void (*sc3_log_function_t) (void *user, const char *msg,
                                    sc3_log_role_t role, int rank, int tid,
                                    sc3_log_level_t level, int spaces,
                                    FILE *outfile);
/* *INDENT-ON* */

/** Log function that prints the incoming message without formatting.
 * \param [in,out] user    This function ignores the user-defined context
 *                         which may be passed to \ref sc3_log_set_function.
 * \param [in] msg     This function adds a newline to the end of the message.
 * \param [in] role    Used for deciding whether to log, not used in formatting.
 * \param [in] rank    Used for deciding whether to log, not used in formatting.
 * \param [in] tid     Used for deciding whether to log, not used in formatting.
 * \param [in] level   Used for deciding whether to log, not used in formatting.
 * \param [in] spaces  Ignored.
 * \param [in,out] outfile      File printed to.
 */
void                sc3_log_function_bare
  (void *user, const char *msg,
   sc3_log_role_t role, int rank, int tid,
   sc3_log_level_t level, int spaces, FILE * outfile);

/** Log function that adds rank/thread information and indent spacing. */
void                sc3_log_function_default
  (void *user, const char *msg,
   sc3_log_role_t role, int rank, int tid,
   sc3_log_level_t level, int spaces, FILE * outfile);

int                 sc3_log_is_valid (const sc3_log_t * log, char *reason);
int                 sc3_log_is_new (const sc3_log_t * log, char *reason);
int                 sc3_log_is_setup (const sc3_log_t * log, char *reason);

/** Return a predefined static logger that has no concept of MPI.
 * \return          Valid and setup logger object.
 *                  It will respect the indent spacing but not the role.
 *                  It is always registered with rank and thread id 0.
 *                  Its log level is fixed at \ref SC3_LOG_TOP,
 *                  and it prints to stderr.
 */
sc3_log_t          *sc3_log_predef (void);

sc3_error_t        *sc3_log_new (sc3_allocator_t * lator, sc3_log_t ** logp);

/** Default with --enable-debug SC3_LOG_DEBUG, otherwise SC3_LOG_TOP */
sc3_error_t        *sc3_log_set_level (sc3_log_t * log,
                                       sc3_log_level_t level);

/** Default SC3_COMM_NULL */
sc3_error_t        *sc3_log_set_comm (sc3_log_t * log,
                                      sc3_MPI_Comm_t mpicomm);

/** Default stderr */
sc3_error_t        *sc3_log_set_file (sc3_log_t * log,
                                      FILE * file, int call_fclose);

/** Set function that effectively formats and outputs the log message.
 * It default to \ref sc3_log_function_default.
 * \param [in,out] log  Logger must not yet be setup.
 * \param [in] func     Non-NULL function; see \ref sc3_log_function_t.
 * \param [in] user     Pointer is passed through to log function \a func.
 * \return              NULL on success, fatal error otherwise.
 */
sc3_error_t        *sc3_log_set_function (sc3_log_t * log,
                                          sc3_log_function_t func,
                                          void *user);

/** Set number of spaces to indent each depth level.
 * \param [in,out] log  Logger must not yet be setup.
 * \param [in] indent   Non-negative number, default 0.
 * \return              NULL on success, fatal error otherwise.
 */
sc3_error_t        *sc3_log_set_indent (sc3_log_t * log, int indent);

sc3_error_t        *sc3_log_setup (sc3_log_t * log);
sc3_error_t        *sc3_log_ref (sc3_log_t * log);
sc3_error_t        *sc3_log_unref (sc3_log_t ** logp);
sc3_error_t        *sc3_log_destroy (sc3_log_t ** logp);

/* Right now, they try to do the right thing always.
   If log == NULL, fprintf to stderr */

/** Log a message depending on selection criteria.
 * This function does not return any error status.
 * If parameters passed in are illegal or the logger NULL, output to stderr.
 * \param [in] log       If NULL, print a simple message to stderr.
 *                       Otherwise, logger must be setup and will be queried
 *                       for log level and format options, etc.
 * \param [in] depth     Number of indentation steps to use.  Non-negative.
 * \param [in] role      See \ref sc3_log_role_t for legal values.
 * \param [in] level     See \ref sc3_log_level_t for legal values.
 * \param [in] msg       If NULL, print "NULL message," otherwise \a msg.
 */
void                sc3_log (sc3_log_t * log, int depth,
                             sc3_log_role_t role, sc3_log_level_t level,
                             const char *msg);

/** See \ref sc3_log. */
void                sc3_logf (sc3_log_t * log, int depth,
                              sc3_log_role_t role, sc3_log_level_t level,
                              const char *fmt, ...)
  __attribute__ ((format (printf, 5, 6)));

/** See \ref sc3_log. */
void                sc3_logv (sc3_log_t * log, int depth,
                              sc3_log_role_t role, sc3_log_level_t level,
                              const char *fmt, va_list ap);

sc3_error_t        *sc3_log_error (sc3_log_t * log, int depth,
                                   sc3_log_role_t role,
                                   sc3_log_level_t level, sc3_error_t * e);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_LOG_H */
