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

/** \file sc3_error.h
 */

#ifndef SC3_ERROR_H
#define SC3_ERROR_H

#include <sc3_base.h>

typedef struct sc3_error sc3_error_t;

#include <sc3_alloc.h>

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/*** DEBUG macros do nothing unless configured with --enable-debug. ***/
#ifndef SC_ENABLE_DEBUG
#define SC3A_CHECK(x) do { ; } while (0)
#define SC3A_STACK(f) do { ; } while (0)
#else
#define SC3A_CHECK(x) do {                                              \
  if (!(x)) {                                                           \
    return sc3_error_new_fatal (__FILE__, __LINE__, #x);                \
  }} while (0)
#define SC3A_STACK(f) do {                                              \
  sc3_error_t *_e = (f);                                                \
  if (_e != NULL) {                                                     \
    return sc3_error_new_stack (&_e, __FILE__, __LINE__, #f);           \
  }} while (0)
#endif

/*** ERROR macros.  They are always active and create fatal errors. ***/
#define SC3E(f) do {                                                    \
  sc3_error_t *_e = (f);                                                \
  if (_e != NULL) {                                                     \
    return sc3_error_new_stack (&_e, __FILE__, __LINE__, #f);           \
  }} while (0)
#define SC3E_DEMAND(x) do {                                             \
  if (!(x)) {                                                           \
    return sc3_error_new_fatal (__FILE__, __LINE__, #x);                \
  }} while (0)
#define SC3E_NONNEG(r) SC3E_DEMAND ((r) >= 0)
#define SC3E_UNREACH(s) do {                                            \
  char _errmsg[SC3_BUFSIZE];                                            \
  (void) snprintf (_errmsg, SC3_BUFSIZE, "Unreachable: %s", s);         \
  return sc3_error_new_fatal (__FILE__, __LINE__, _errmsg);             \
  } while (0)
#define SC3E_RETVAL(r,v) do {                                           \
  SC3A_CHECK ((r) != NULL);                                             \
    *(r) = (v);                                                         \
  } while (0)
#define SC3E_INOUTP(pp,p) do {                                          \
  SC3A_CHECK ((pp) != NULL && *(pp) != NULL);                           \
  (p) = *(pp);                                                          \
  } while (0)
#define SC3E_INULLP(pp,p) do {                                          \
  SC3A_CHECK ((pp) != NULL && *(pp) != NULL);                           \
  (p) = *(pp);                                                          \
  *(pp) = NULL;                                                         \
  } while (0)

/*** ERROR macros.  Do not call return and inherit the severity. ***/
#define SC3E_SET(e,f) do {                                              \
  sc3_error_t *_e = (f);                                                \
  if (_e != NULL) {                                                     \
    (e) = sc3_error_new_inherit (&_e, __FILE__, __LINE__, #f);          \
  } else { (e) = NULL; }                                                \
  } while (0)
#define SC3E_NULL_SET(e,f) do {                                         \
  if ((e) == NULL) SC3E_SET(e,f);                                       \
  } while (0)

typedef enum sc3_error_severity
{
  SC3_ERROR_RUNTIME,
  SC3_ERROR_WARNING,
  SC3_ERROR_FATAL,
  SC3_ERROR_SEVERITY_LAST
}
sc3_error_severity_t;

typedef enum sc3_error_sync
{
  SC3_ERROR_LOCAL,
  SC3_ERROR_SYNCED,
  SC3_ERROR_DISAGREE,
  SC3_ERROR_SYNC_LAST
}
sc3_error_sync_t;

typedef struct sc3_error_args sc3_error_args_t;

/*** TODO error functions shall not throw new errors themselves? ***/

/** Create a new error argument object.
 * \param [in] eator    An existing allocator or NULL.
 *                      The allocator is refd and remembered internally
 *                      and will be unrefd on destruction.
 *                      If NULL, calls \ref sc3_allocator_nocount.
 * \param [out] eap     Pointer must not be NULL.
 *                      If the function returns an error, value set to NULL.
 *                      Otherwise, value set to arguments with default values.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_args_new (sc3_allocator_t * eator,
                                        sc3_error_args_t ** eap);

/** Destroy an allocator argument object and unrefs its allocator.
 * \param [in,out] eap  Pointer and value must not be NULL.
 *                      Value is set to NULL.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_args_destroy (sc3_error_args_t ** eap);

/** Takes ownership of stack (i.e. does not ref it), stack is NULLed.
 * If called multiple times, the stack passed earlier is unref'd.
 */
sc3_error_t        *sc3_error_args_set_stack (sc3_error_args_t * ea,
                                              sc3_error_t ** stack);
sc3_error_t        *sc3_error_args_set_location (sc3_error_args_t * ea,
                                                 const char *filename,
                                                 int line);
sc3_error_t        *sc3_error_args_set_message (sc3_error_args_t * ea,
                                                const char *errmsg);
sc3_error_t        *sc3_error_args_set_severity (sc3_error_args_t * ea,
                                                 sc3_error_severity_t sev);
#if 0
void                sc3_error_args_set_sync (sc3_error_args_t * ea,
                                             sc3_error_sync_t syn);
sc3_error_t        *sc3_error_args_set_msgf (sc3_error_args_t * ea,
                                             const char *errfmt, ...)
  __attribute__ ((format (printf, 2, 3)));
#endif

/** Create a new allocated error object from an argument structure.
 * \param [in,out] ea   The argument structure is destroyed and set to NULL.
 * \param [out] ep      If the function returns an error, value set to NULL.
 *                      Otherwise, value set to new error object.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_new (sc3_error_args_t ** ea, sc3_error_t ** ep);

/** Add a reference to an error object.
 * \param [in] e        Pointer must not be NULL.
 *                      If the error is allocated, its refcount is increased.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_ref (sc3_error_t * e);

/** Remove a reference from an error object and deallocate if it was the last.
 * \param [in,out] e    Pointer must not be NULL.
 *                      If the error is allocated, its refcount is decreased
 *                      and it is checked if it reaches zero.
 *                      In this case, the error is deallocated and set to NULL.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_unref (sc3_error_t ** ep);

/** Takes an error object with one remaining reference and deallocates it.
 * It is an error to destroy an error that is multiply refd or not allocated.
 * \param [in,out] ep   Allocated error with one reference.  NULL on output.
 * \return              0 if error object is cleanly deallocated, -1 otherwise.
 */
int                 sc3_error_destroy (sc3_error_t ** ep);

/** Frees the top object on an error stack and returns the next deepest.
 * \param [in,out] ep   Pointer and value must not be NULL.
 *                      When output is 0, set to next deepest error object.
 *                      Otherwise, value set to NULL.
 * \return              0 if it is possible to return the next deepest error
 *                      in the stack, -1 otherwise.
 */
int                 sc3_error_pop (sc3_error_t ** ep);

sc3_error_t        *sc3_error_new_ssm (sc3_allocator_t * alloc,
                                       sc3_error_severity_t sev,
                                       sc3_error_sync_t syn,
                                       const char *errmsg);

/* TODO: new_fatal, new_stack, new_inherit always return consistent results.
         They must not lead to an infinite loop (e.g. when out of memory). */
/* TODO: shall we pass an allocator parameter to new_fatal and new_stack? */

sc3_error_t        *sc3_error_new_fatal (const char *filename,
                                         int line, const char *errmsg);

/** Takes owership of stack (i.e. does not ref it and NULLs the pointer).
 * The new error severity is set to SC3_ERROR_FATAL. */
sc3_error_t        *sc3_error_new_stack (sc3_error_t ** stack,
                                         const char *filename,
                                         int line, const char *errmsg);

/** Takes owership of stack (i.e. does not ref it and NULLs the pointer).
 * The new error inherits the severity of the old. */
sc3_error_t        *sc3_error_new_inherit (sc3_error_t ** stack,
                                           const char *filename,
                                           int line, const char *errmsg);

/*** TODO need a bunch of _get_ and/or _is_ functions ***/

/** Check an error object to be valid and fatal.
 * \return              true if e is not NULL and has severity
 *                      SC3_ERROR_FATAL, false otherwise.
 */
int                 sc3_error_is_fatal (sc3_error_t * e);

/*** Choose simplicity over export/release and error checking. ***/

/** Return pointer to the filename in an error object.
 * The filename output pointer is only valid as long as the error is alive.
 * It is ok to call this function on NULL or non-filenamed errors.
 * \param [in] e        Error object.
 * \param [out] filename    Set to error's filename if it is not NULL.
 *                          the empty string otherwise.
 * \param [out] line        Set to error's line number if it is not NULL.
 */
void                sc3_error_get_location (sc3_error_t * e,
                                            const char **filename, int *line);

/** Return pointer to the message string in an error object.
 * The message output pointer is only valid as long as the error is alive.
 * It is ok to call this function on NULL or non-messaged errors.
 * \param [in] e        Error object.
 * \param [out] errmsg  Set to error's message if it is not NULL,
 *                      the empty string otherwise.
 */
void                sc3_error_get_message (sc3_error_t * e,
                                           const char **errmsg);

/** Return the severity of an error object.
 * It is ok to call this function on NULL errors.
 * \param [in] e        Error object or NULL.
 * \param [out] sev     Set to error's severity if the error and \b sev
 *                      are not NULL, SC3_ERROR_FATAL otherwise.
 */
void                sc3_error_get_severity (sc3_error_t * e,
                                            sc3_error_severity_t * sev);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_ERROR_H */
