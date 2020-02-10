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
#define SC3A_IS(f,o) SC3_NOOP
#define SC3A_CHECK(x) SC3_NOOP
#define SC3A_STACK(f) SC3_NOOP
#define SC3A_ONULL(r) SC3_NOOP
#else
#define SC3A_IS(f,o) SC3E_DEMIS(f,o)
#define SC3A_CHECK(x) do {                                              \
  if (!(x)) {                                                           \
    return sc3_error_new_fatal (__FILE__, __LINE__, #x);                \
  }} while (0)
#define SC3A_STACK(f) do {                                              \
  sc3_error_t *_e = (f);                                                \
  if (_e != NULL) {                                                     \
    return sc3_error_new_stack (&_e, __FILE__, __LINE__, #f);           \
  }} while (0)
#define SC3A_ONULL(r) do {                                              \
  SC3A_CHECK ((r) != NULL);                                             \
  *(r) = NULL;                                                          \
  } while (0)
#endif

/*** ERROR macros.  They are always active and create fatal errors. ***/
#define SC3E(f) do {                                                    \
  sc3_error_t *_e = (f);                                                \
  if (_e != NULL) {                                                     \
    return sc3_error_new_stack (&_e, __FILE__, __LINE__, #f);           \
  }} while (0)
#define SC3E_DEMAND(x,s) do {                                           \
  if (!(x)) {                                                           \
    char _errmsg[SC3_BUFSIZE];                                          \
    snprintf (_errmsg, SC3_BUFSIZE, "%s: %s", #x, (s));                 \
    return sc3_error_new_fatal (__FILE__, __LINE__, _errmsg);           \
  }} while (0)
#define SC3E_DEMIS(f,o) do {                                            \
  char _r[SC3_BUFSIZE];                                                 \
  if (!(f ((o), _r))) {                                                 \
    char _errmsg[SC3_BUFSIZE];                                          \
    snprintf (_errmsg, SC3_BUFSIZE, "%s(%s): %s", #f, #o, _r);          \
    return sc3_error_new_fatal (__FILE__, __LINE__, _errmsg);           \
  }} while (0)
#define SC3E_UNREACH(s) do {                                            \
  char _errmsg[SC3_BUFSIZE];                                            \
  snprintf (_errmsg, SC3_BUFSIZE, "Unreachable: %s", (s));              \
  return sc3_error_new_fatal (__FILE__, __LINE__, _errmsg);             \
  } while (0)
#define SC3E_RETVAL(r,v) do {                                           \
  SC3A_CHECK ((r) != NULL);                                             \
  *(r) = (v);                                                           \
  } while (0)
#define SC3E_RETOPT(r,v) do {                                           \
  if ((r) != NULL) *(r) = (v);                                          \
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
#define SC3E_ONULLP(pp,p) do {                                          \
  SC3A_CHECK ((pp) != NULL);                                            \
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
#define SC3E_NULL_REQ(e,x) do {                                         \
  if ((e) == NULL && !(x)) {                                            \
    (e) = sc3_error_new_fatal (__FILE__, __LINE__, #x);                 \
  }} while (0)
#define SC3E_NULL_BREAK(e) do {                                         \
  if ((e) != NULL) { break; }} while (0)

/*** TEST macros.  Always executed. */
#define SC3E_YES(r) do {                                                \
  if ((r) != NULL) { (r)[0] = '\0'; } return 1; } while (0)
#define SC3E_TEST(x,r)                                                  \
  do { if (!(x)) {                                                      \
    if ((r) != NULL) { SC3_BUFCOPY ((r), #x); }                         \
    return 0; }} while (0)
#define SC3E_IS(f,o,r)                                                  \
  do { if ((r) == NULL) {                                               \
    if (!(f ((o), NULL))) { return 0; }} else {                         \
    char _r[SC3_BUFSIZE];                                               \
    if (!(f ((o), _r))) {                                               \
      snprintf ((r), SC3_BUFSIZE, "%s(%s): %s", #f, #o, _r);            \
      return 0; }}} while (0)
#define SC3E_TERR(f,r) do {                                             \
  sc3_error_t * _e = (f);                                               \
  if (_e != NULL) {                                                     \
    sc3_error_destroy_noerr (&_e, r); return 0; }} while (0)

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

/** Check whether an error is not NULL and internally consistent.
 * The error may be valid in both its setup and usage phases.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer is not NULL and error consistent.
 */
int                 sc3_error_is_valid (sc3_error_t * e, char *reason);

/** Check whether an error is not NULL, consistent and not setup.
 * This means that the error is not in its usage phase.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL, error consistent, not setup.
 */
int                 sc3_error_is_new (sc3_error_t * e, char *reason);

/** Check whether an error is not NULL, internally consistent and setup.
 * This means that the error is in its usage phase.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL, error consistent and setup.
 */
int                 sc3_error_is_setup (sc3_error_t * e, char *reason);

/** Check an error object to be setup and fatal.
 * \param [in] a        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if error is not NULL, setup, and has severity
 *                      SC3_ERROR_FATAL, false otherwise.
 */
int                 sc3_error_is_fatal (sc3_error_t * e, char *reason);

/*** TODO error functions shall not throw new errors themselves?! ***/

/*** TODO document all default values after _new. ***/

/** Create a new error object in its setup phase.
 * It begins with default parameters that can be overridden explicitly.
 * Setting and modifying parameters is only allowed in the setup phase.
 * Call \ref sc3_error_setup to change the error into its usage phase.
 * After that, no more parameters may be set.
 * \param [in,out] aator    An allocator that is setup.
 *                          The allocator is refd and remembered internally
 *                          and will be unrefd on error destruction.
 * \param [out] eap     Pointer must not be NULL.
 *                      If the function returns an error, value set to NULL.
 *                      Otherwise, value set to an error with default values.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_new (sc3_allocator_t * eator,
                                   sc3_error_t ** ep);

/** Set the error to be the top of a stack of existing errors.
 * \param [in,out] e        Error object before \ref sc3_error_setup.
 * \param [in,out] stack    This function takes ownership of stack
 *                          (i.e. does not ref it), the pointer is NULLed.
 *                          The stack may be NULL or must be setup.
 *                          If called multiple times, a stack passed earlier
 *                          is unrefd internally.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_stack (sc3_error_t * e,
                                         sc3_error_t ** stack);

/** Set the filename and line number of an error.
 * \param [in,out] e    Error object before \ref sc3_error_setup.
 * \param [in] filename Null-terminated string.  Pointer must not be NULL.
 * \param [in] line     Line number, non-negative.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_location (sc3_error_t * e,
                                            const char *filename, int line);

/** Set the message of an error.
 * \param [in,out] e    Error object before \ref sc3_error_setup.
 * \param [in] message  Null-terminated string.  Pointer must not be NULL.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_message (sc3_error_t * e,
                                           const char *errmsg);

/** Set the severity of an error.
 * \param [in,out] e    Error object before \ref sc3_error_setup.
 * \param [in] sev      Enum value in [0, SC3_ERROR_SEVERITY_LAST).
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_severity (sc3_error_t * e,
                                            sc3_error_severity_t sev);

#if 0
void                sc3_error_set_sync (sc3_error_t * e,
                                        sc3_error_sync_t syn);
sc3_error_t        *sc3_error_set_msgf (sc3_error_t * e,
                                        const char *errfmt, ...)
  __attribute__ ((format (printf, 2, 3)));
#endif

/** Setup an error and put it into its usable phase.
 * \param [in,out] e    This error must not yet be setup.
 *                      Internal storage is allocated, the setup phase ends,
 *                      and the error is put into its usable phase.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_setup (sc3_error_t * e);

/** Increase the reference count on an error object by 1.
 * This is only allowed after the error has been setup.
 * Does nothing if error has not been created by \ref sc3_error_new.
 * \param [in,out] e    This error must be setup.  Its refcount is increased.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_ref (sc3_error_t * e);

/** Decrease the reference of an error object by 1.
 * If the count reaches zero the error object is deallocated.
 * Does nothing if error has not been created by \ref sc3_error_new.
 * \param [in,out] e    Pointer must not be NULL and the error valid.
 *                      The refcount is decreased.  If it reaches zero,
 *                      the error is deallocated and the value to NULL.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_unref (sc3_error_t ** ep);

/** Takes an error object with one remaining reference and deallocates it.
 * It is an error to destroy an error that is multiply refd.
 * Does nothing if error has not been created by \ref sc3_error_new.
 * \param [in,out] ep   Setup error with one reference.  NULL on output.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_destroy (sc3_error_t ** ep);

/** Destroy an error object and condense its messages into a string.
 * \param [in,out] pe   This error will be destroyed and pointer NULLed.
 *                      If any errors occur in the process, they are ignored.
 *                      One would be that the error has multiple references.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is filled with the error stack messages flattened.
 */
void                sc3_error_destroy_noerr (sc3_error_t ** pe,
                                             char *flatmsg);

/** Create a new fatal error without relying on dynamic memory allocation.
 * This is useful when allocating an error with \ref sc3_error_new might fail.
 * \param [in] filename The filename is copied into the error object.
 *                      Pointer not NULL, string null-terminated.
 * \param [in] line     Line number set in the error.
 * \param [in] errmsg   The error message is copied into the error.
 *                      Pointer not NULL, string null-terminated.
 * \return          The new error object; there is no other error handling.
 */
sc3_error_t        *sc3_error_new_fatal (const char *filename,
                                         int line, const char *errmsg);

/** Create a new fatal error without relying on dynamic memory allocation.
 * This is useful when allocating an error with \ref sc3_error_new might fail.
 * This function expects a setup error as stack.
 * \param [in] pstack   We take owership of the stack, pointer is NULLed.
 * \param [in] filename The filename is copied into the error object.
 *                      Pointer not NULL, string null-terminated.
 * \param [in] line     Line number set in the error.
 * \param [in] errmsg   The error message is copied into the error.
 *                      Pointer not NULL, string null-terminated.
 * \return          The new error object; there is no other error handling.
 */
sc3_error_t        *sc3_error_new_stack (sc3_error_t ** stack,
                                         const char *filename,
                                         int line, const char *errmsg);

/** Create a new error without relying on dynamic memory allocation.
 * This is useful when allocating an error with \ref sc3_error_new might fail.
 * This function expects a setup error as stack and uses its severity value.
 * \param [in] pstack   We take owership of the stack, pointer is NULLed.
 *                      The severity of *pstack is used for the result error.
 * \param [in] filename The filename is copied into the error object.
 *                      Pointer not NULL, string null-terminated.
 * \param [in] line     Line number set in the error.
 * \param [in] errmsg   The error message is copied into the error.
 *                      Pointer not NULL, string null-terminated.
 * \return          The new error object; there is no other error handling.
 */
sc3_error_t        *sc3_error_new_inherit (sc3_error_t ** stack,
                                           const char *filename,
                                           int line, const char *errmsg);

/** Access location string and line number in a setup error object.
 * The filename output pointer is only valid as long as the error is alive.
 * This can be ensured, optionally, by refing and later unrefing the error.
 * We do not ensure this by default to simplify short-time usage.
 * \param [in] e            Setup error object.
 * \param [out] filename    Pointer may be NULL, then it is not assigned.
 *                          On success, set to error's filename.
 * \param [out] line        Pointer may be NULL, then it is not assigned.
 *                          On success, set to error's line number.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_get_location (sc3_error_t * e,
                                            const char **filename, int *line);

/** Access pointer to the message string in a setup error object.
 * The message output pointer is only valid as long as the error is alive.
 * This can be ensured, optionally, by refing and later unrefing the error.
 * We do not ensure this by default to simplify short-time usage.
 * \param [in] e        Setup error object.
 * \param [out] errmsg  Pointer may be NULL, then it is not assigned.
 *                      On success, set to error's message.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_get_message (sc3_error_t * e,
                                           const char **errmsg);

/** Access the severity of a setup error object.
 * \param [in] e        Setup error object.
 * \param [out] sev     Pointer may be NULL, then it is not assigned.
 *                      On success, set to error's severity.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_get_severity (sc3_error_t * e,
                                            sc3_error_severity_t * sev);

/** Return the next deepest error stack with an added reference.
 * The input error object must be setup.  Its stack is allowed to be NULL.
 * It is not changed by the call except for its stack to get referenced.
 * \param [in,out] e    The error object must be setup.
 * \param [out] pstack  Pointer may be NULL, then it is not assigned.
 *                      When function returns cleanly, set to input error's
 *                      stack object.  If the stack is not NULL, it is refd.
 *                      In this case it must be unrefd when no longer needed.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_get_stack (sc3_error_t * e,
                                         sc3_error_t ** stack);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_ERROR_H */
