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

/** We may log per root MPI rank or for each MPI process. */
typedef enum sc3_log_role
{
  SC3_LOG_LOCAL,       /**< Log on all processes */
  SC3_LOG_GLOBAL,      /**< Log only on root process */
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
  SC3_LOG_STATISTICS,   /**< Major diagnostics and statistical summaries */
  SC3_LOG_PRODUCTION,   /**< Sparse flow logging for toplevel functions */
  SC3_LOG_ESSENTIAL,    /**< A few lines per program: version, options */
  SC3_LOG_ERROR,        /**< Errors by misusage, internal bugs, I/O */
  SC3_LOG_SILENT,       /**< This log level will not print anything */
  SC3_LOG_LEVEL_LAST
}
sc3_log_level_t;

/* *INDENT-OFF* */
/** Prototype for the user-selectable log output function.
 * This function does not need to decide whether to log based on role or level.
 * This is done before calling this function, or not calling it depending.
 * This function is just responsible for formatting and writing the message.
 */
typedef void (*sc3_log_function_t) (void *user, const char *msg,
                                    sc3_log_role_t role, int rank,
                                    sc3_log_level_t level, FILE *outfile);
/* *INDENT-ON* */

#ifndef SC3_LOG_LEVEL
#ifdef SC_ENABLE_DEBUG
#define SC3_LOG_LEVEL SC3_LOG_DEBUG
#else
#define SC3_LOG_LEVEL SC3_LOG_INFO
#endif
#endif

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
   sc3_log_role_t role, int rank,
   sc3_log_level_t level, FILE * outfile);

/** Log function that adds rank/thread information and indent spacing. */
void                sc3_log_function_default
  (void *user, const char *msg,
   sc3_log_role_t role, int rank,
   sc3_log_level_t level, FILE * outfile);

int                 sc3_log_is_valid (const sc3_log_t * log, char *reason);
int                 sc3_log_is_new (const sc3_log_t * log, char *reason);
int                 sc3_log_is_setup (const sc3_log_t * log, char *reason);

/** Create new logging object.
 */
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

/** Return a predefined static logger that uses MPI_COMM_WORLD.
 * \return          Valid and setup logger object prints to stderr.
 */
sc3_log_t          *sc3_log_new_static (void);

/* Right now, they try to do the right thing always.
   If log == NULL, fprintf to stderr */

/** Log a message depending on selection criteria.
 * This function does not return any error status.
 * If parameters passed in are illegal or the logger NULL, output to stderr.
 * \param [in] log       If NULL, write a message to stderr.
 *                       Otherwise, logger must be setup and will be queried
 *                       for log level and format options, etc.
 * \param [in] role      See \ref sc3_log_role_t for legal values.
 * \param [in] level     See \ref sc3_log_level_t for legal values.
 * \param [in] msg       If NULL, print "NULL message," otherwise \a msg.
 */
void                sc3_log (sc3_log_t * log,
                             sc3_log_role_t role, sc3_log_level_t level,
                             const char *msg);

/** See \ref sc3_log. */
void                sc3_logf (sc3_log_t * log,
                              sc3_log_role_t role, sc3_log_level_t level,
                              const char *fmt, ...)
  __attribute__((format (printf, 4, 5)));

/** See \ref sc3_log. */
void                sc3_logv (sc3_log_t * log,
                              sc3_log_role_t role, sc3_log_level_t level,
                              const char *fmt, va_list ap);

/** Log the location and message of an error object and its history stack.
 * We call the log function once for each stack entry.
 * \param [in,out] log  Logger must be setup.
 * \param [in] depth    Number if indentation levels.
 * \param [in] role     Parallelism: root rank, per process, per thread.
 * \param [in] level    Log level (priority).
 * \param [in,out] e    This error is not modified.
 *                      It is an in-out argument since we access and restore
 *                      some of its resources.
 */
sc3_error_t        *sc3_log_error (sc3_log_t * log,
                                   sc3_log_role_t role, sc3_log_level_t level,
                                   sc3_error_t * e);

#define _SC3_LOG_LEVEL(barelevel) SC3_LOG_ ## barelevel

#define _SC3_LOG_PROT                                                   \
  (const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

#define _SC3_LOG_BODY(localglobal,loglevel) (const char *fmt, ...) {    \
  if (SC3_LOG_LEVEL <= loglevel) {                                      \
    va_list            ap;                                              \
    va_start (ap, fmt);                                                 \
    sc3_logv (sc3_log_new_static (), localglobal, loglevel, fmt, ap);   \
    va_end (ap);                                                        \
  }}

#define _SC3_LOG_FUNCTIONS(barelevel)                                   \
static inline void SC3_ ## barelevel ## F _SC3_LOG_PROT                 \
static inline void SC3_ ## barelevel ## F                               \
  _SC3_LOG_BODY (SC3_LOG_LOCAL, _SC3_LOG_LEVEL(barelevel))              \
static inline void SC3_GLOBAL_ ## barelevel ## F _SC3_LOG_PROT          \
static inline void SC3_GLOBAL_ ## barelevel ## F                        \
  _SC3_LOG_BODY (SC3_LOG_GLOBAL, _SC3_LOG_LEVEL(barelevel))

_SC3_LOG_FUNCTIONS (NOISE)
#define SC3_NOISES(s) SC3_NOISEF("%s", s);
_SC3_LOG_FUNCTIONS (DEBUG)
#define SC3_DEBUGS(s) SC3_DEBUGF("%s", s);
_SC3_LOG_FUNCTIONS (INFO)
#define SC3_INFOS(s) SC3_INFOF("%s", s);
_SC3_LOG_FUNCTIONS (STATISTICS)
#define SC3_STATISTICS(s) SC3_STATISTICSF("%s", s);
_SC3_LOG_FUNCTIONS (PRODUCTION)
#define SC3_PRODUCTIONS(s) SC3_PRODUCTIONF("%s", s);
_SC3_LOG_FUNCTIONS (ESSENTIAL)
#define SC3_ESSENTIALS(s) SC3_ESSENTIALF("%s", s);
_SC3_LOG_FUNCTIONS (ERROR)
#define SC3_ERRORS(s) SC3_ERRORF("%s", s);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_LOG_H */
