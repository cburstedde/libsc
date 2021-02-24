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

/** \file sc3_error.h \ingroup sc3
 * The error object is fundamental to the design of the library.
 *
 * We propagate errors up the call stack by returning a pointer of type
 * \ref sc3_error_t from most library functions.
 * If a function returns NULL, it has executed successfully.
 * Otherwise, it encountered an error condition encoded in the return value.
 *
 * The error object may be used to indicate both fatal and non-fatal errors.
 * The fatal sort arises when out of memory, encountering unreachable code
 * and on assertion failures, or whenever a function decides to return fatal.
 * If a function returns any of these hard error conditions, the application
 * must consider that it returned prematurely and future crashes are likely.
 * In addition, communication and I/O may be pending and cause blocking.
 * It is the application's responsibility to take this into account, for
 * example by reporting the error and shutting down as cleanly as possible.
 * The SC library does *not* abort on its own to play nice with callers.
 * See \ref sc3_error_is_fatal for the kinds considered fatal.
 *
 * Logically, a function returns non-fatal => all internal state is consistent.
 * This invariant is preserved if non-fatal errors are propagated upward as
 * fatal.  It is incorrect to propagate a fatal error up as non-fatal.
 *
 * In other words, a function may return inconsistent/buggy only if it
 * returns an error of the fatal kind.  This only-if-condition is one-way.
 *
 * To propagate fatal errors up the call stack, the SC3A and SC3E types of
 * macros are provided for convenience.
 * The SC3A macros (Assert) are only active when configured --enable-debug
 * and intended to check conditions that are generally true in absence of bugs.
 * The SC3E macros (Execute) are always active and intended to call subroutines.
 * Both check conditions or error returns and return fatal errors themselves.
 * These macros are understood to effect immediate return on error.
 *
 * The library functions return an error of kind \ref SC3_ERROR_LEAK
 * when encountering leftover memory, references, or other resources,
 * but ensure that the program may continue cleanly.
 * Thus, an application is free to treat leaks as fatal or not.
 * Library functions may return leaks on an object destroy or unref call.
 * To catch leak errors gracefully, consider using the SC3L macros.
 * They are designed to continue the control flow.
 *
 * Non-fatal errors are imaginable when accessing files on disk or parsing
 * input.  It is up to the application to define and implement error handling
 * that ideally reports the situation and either continues or shuts down cleanly.
 *
 * The object query functions sc3_<object>_is?_* shall return cleanly on
 * any kind of input parameters, even those that do not make sense.
 * Query functions must be designed not to crash under any circumstance.
 * On incorrect or undefined input, they must return false.
 * On defined input, they return the proper query result.
 * This may lead to the situation that sc3_object_is_A and sc3_object_is_not_A
 * both return false, say when the call convention is violated.
 *
 * The following functions are used to construct error objects.
 *  * \ref sc3_error_new
 *  * \ref sc3_error_set_stack
 *  * \ref sc3_error_set_location
 *  * \ref sc3_error_set_message
 *  * \ref sc3_error_set_kind
 *  * \ref sc3_error_setup
 *
 * Error objects after setup can be accessed by the following functions.
 *  * \ref sc3_error_access_location
 *  * \ref sc3_error_restore_location must be called for every location access
 *  * \ref sc3_error_access_message
 *  * \ref sc3_error_restore_message must be called for every message access
 *  * \ref sc3_error_get_kind
 *  * \ref sc3_error_get_stack yields NULL or an error that is accessed
 *                             and dropped in the same way.
 *  * \ref sc3_error_get_text creates a text summary of all error information.
 *
 * To drop responsibility for an error object, use \ref sc3_error_unref.
 *
 * As a convenience for calling libsc from an application, we have added \ref
 * sc3_error_check.  It examines an error passed in and returns 0 if the error
 * is NULL.  Otherwise, it extracts its text block using \ref
 * sc3_error_get_text, calls \ref sc3_error_unref and returns -1.  This way, an
 * application program can react on the return value and use the message for
 * its own reporting.  What is lost by this approach is the ability to
 * distinguish various kinds of error.  In effect, all errors are treated the
 * same downstream, which is fine when only using it for fatal conditions.
 */

#ifndef SC3_ERROR_H
#define SC3_ERROR_H

#include <sc3_base.h>

/** The sc3 error object is an opaque structure. */
typedef struct sc3_error sc3_error_t;

#include <sc3_alloc.h>

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/*** DEBUG statements do nothing unless configured with --enable-debug. ***/

#ifndef SC_ENABLE_DEBUG

/** Assertion statement to require a true sc3_object_is_* return value.
 * The argument \a f is such a function and \a o the object to query. */
#define SC3A_IS(f,o) SC3_NOOP

/** Assertion statement to require a true sc3_object_is2_* return value.
 * The argument \a f is such a function taking \a o the object to query,
 * a second argument \a p (and the common string buffer as all of them). */
#define SC3A_IS2(f,o,p) SC3_NOOP

/** Assertion statement requires some condition \a x to be true. */
#define SC3A_CHECK(x) SC3_NOOP

#else
#define SC3A_IS(f,o) SC3E_DEMIS(f,o)
#define SC3A_IS2(f,o,p) SC3E_DEMIS2(f,o,p)
#define SC3A_CHECK(x) do {                                              \
  if (!(x)) {                                                           \
    return sc3_error_new_bug ( __FILE__, __LINE__, #x);                 \
  }} while (0)
#endif

/*** ERROR statements are always active and return fatal errors. ***/

/** Execute an expression \a f that produces an \ref sc3_error_t object.
 * If that error is not NULL, create a fatal error and return it.
 * This macro stacks every non-NULL error returned into a fatal error.
 * For clean recovery of non-fatal errors, please use something else.
 */
#define SC3E(f) do {                                                    \
  sc3_error_t *_e = (f);                                                \
  if (_e != NULL) {                                                     \
    return sc3_error_new_stack (&_e, __FILE__, __LINE__, #f);           \
  }} while (0)

/** If a condition \a x is not met, create a fatal error and return it.
 * The error created is of type \ref SC3_ERROR_BUG and its message is set
 * to the failed condition and argument \a s.
 * For clean recovery of non-fatal errors, please use something else.
 */
#define SC3E_DEMAND(x,s) do {                                           \
  if (!(x)) {                                                           \
    char _errmsg[SC3_BUFSIZE];                                          \
    sc3_snprintf (_errmsg, SC3_BUFSIZE, "%s: %s", #x, (s));             \
    return sc3_error_new_bug (__FILE__, __LINE__, _errmsg);             \
  }} while (0)

/** Execute an is_* query \a f and return a fatal error if it returns false.
 * The error created is of type \ref SC3_ERROR_BUG and its message is set
 * to the failed query function and its object \a o.
 * For clean recovery of non-fatal errors, please use something else.
 */
#define SC3E_DEMIS(f,o) do {                                            \
  char _r[SC3_BUFSIZE];                                                 \
  if (!(f ((o), _r))) {                                                 \
    char _errmsg[SC3_BUFSIZE];                                          \
    sc3_snprintf (_errmsg, SC3_BUFSIZE, "%s(%s): %s", #f, #o, _r);      \
    return sc3_error_new_bug (__FILE__, __LINE__, _errmsg);             \
  }} while (0)

/** Execute an is_* query \a f that takes an additional argument
 * and return a fatal error if it returns false.
 * The error created is of type \ref SC3_ERROR_BUG and its message is set
 * to the failed query function and its object \a o.
 * For clean recovery of non-fatal errors, please use something else.
 */
#define SC3E_DEMIS2(f,o,p) do {                                         \
  char _r[SC3_BUFSIZE];                                                 \
  if (!(f ((o), (p), _r))) {                                            \
    char _errmsg[SC3_BUFSIZE];                                          \
    sc3_snprintf (_errmsg, SC3_BUFSIZE,                                 \
                  "%s(%s,%s): %s", #f, #o, #p, _r);                     \
    return sc3_error_new_bug (__FILE__, __LINE__, _errmsg);             \
  }} while (0)

/** When encountering unreachable code, return a fatal error.
 * The error created is of type \ref SC3_ERROR_BUG
 * and its message is provided as parameter \a s.
 */
#define SC3E_UNREACH(s) do {                                            \
  char _errmsg[SC3_BUFSIZE];                                            \
  sc3_snprintf (_errmsg, SC3_BUFSIZE, "Unreachable: %s", (s));          \
  return sc3_error_new_bug (__FILE__, __LINE__, _errmsg);               \
  } while (0)

/** Assert a pointer parameter \a r not to be NULL
 * and initialize its value to \a v. */
#define SC3E_RETVAL(r,v) do {                                           \
  SC3A_CHECK ((r) != NULL);                                             \
  *(r) = (v);                                                           \
  } while (0)

/** Require an input pointer \a pp to dereference to a non-NULL value.
 * The value is assigned to the pointer \a p for future use. */
#define SC3E_INOUTP(pp,p) do {                                          \
  SC3A_CHECK ((pp) != NULL && *(pp) != NULL);                           \
  (p) = *(pp);                                                          \
  } while (0)

/** Require an input pointer \a pp to dereference to a non-NULL value,
 * assign its value to \a p for future use, then set input value to NULL. */
#define SC3E_INULLP(pp,p) do {                                          \
  SC3A_CHECK ((pp) != NULL && *(pp) != NULL);                           \
  (p) = *(pp);                                                          \
  *(pp) = NULL;                                                         \
  } while (0)

/** Execute an expression \a f that produces an \ref sc3_error_t object.
 * If that error is fatal, stack it into a new fatal error and return it.
 * If that error is NULL, this will be assigned to the output argument.
 * Otherwise, stack the error into a new object of the same kind and
 * place this new error in the output argument \a o.  In this case,
 * the calling code takes ownership of the error.
 */
#define SC3F(f, o) do {                                                 \
  sc3_error_t *_e = o = (f);                                            \
  if (_e != NULL) {                                                     \
    o = sc3_error_new_inherit (&_e, __FILE__, __LINE__, #f);            \
    if (sc3_error_is_fatal (o, NULL)) { return o; }                     \
  }} while (0)

/*** ERROR statements for proceeding without return in case of errors. ***/

/** Initialize an error object \a e using the result of an expression \a f.
 * If the result is NULL, that becomes the value of the error object.
 * If the result is not NULL, it is stacked into a fatal error,
 * which is then assigned to the error object.
 */
#define SC3E_SET(e,f) do {                                              \
  sc3_error_t *_e = (f);                                                \
  if (_e != NULL) {                                                     \
    (e) = sc3_error_new_stack (&_e, __FILE__, __LINE__, #f);            \
  } else { (e) = NULL; }                                                \
  } while (0)

/* TODO think about propagating leak errors correctly */
/** Set an error object \a e that is NULL using the result of an expression.
 * The update is perforrmed as in \ref SC3E_SET with expression \a f.
 * If the error object is not NULL, the macro does not evaluate \a f.
 */
#define SC3E_NULL_SET(e,f) do {                                         \
  if ((e) == NULL) SC3E_SET(e,f);                                       \
  } while (0)

/** If an error object \a e is NULL, set it if condition \a x fails.
 * In this case, we set the object to a new \ref SC3_ERROR_BUG.
 */
#define SC3E_NULL_REQ(e,x) do {                                         \
  if ((e) == NULL && !(x)) {                                            \
    (e) = sc3_error_new_bug (__FILE__, __LINE__, #x);                   \
  }} while (0)

/** If an error object \a e is not NULL, break a loop. */
#define SC3E_NULL_BREAK(e) do {                                         \
  if ((e) != NULL) { break; }} while (0)

/*** QUERY statements.  Always executed. ***/

/** Set the reason output parameter \a r inside an sc3_object_is_* function
 * to a given value \a reason before unconditionally returning false.
 * \a r may be NULL in which case it is not updated.
 */
#define SC3E_NO(r,reason) do {                                          \
  if ((r) != NULL) { SC3_BUFCOPY ((r), (reason)); } return 0; } while (0)

/** Set the reason output parameter \a r inside an sc3_object_is_* function
 * to "" before returning true unconditionally.
 * \a r may be NULL in which case it is not updated.
 */
#define SC3E_YES(r) do {                                                \
  if ((r) != NULL) { (r)[0] = '\0'; } return 1; } while (0)

/** Query condition \a x to be true, in which case the macro does nothing.
 * If the condition is false, the put the reason into \a r, a pointer
 * to a string of size \ref SC3_BUFSIZE, and return 0.
 */
#define SC3E_TEST(x,r)                                                  \
  do { if (!(x)) {                                                      \
    if ((r) != NULL) { SC3_BUFCOPY ((r), #x); }                         \
    return 0; }} while (0)

/** Query an sc3_object_is_* function.
 * The argument \a f shall be such a function and \a o an object to query.
 * If the test returns false, the function puts the reason into \a r,
 * a pointer to a string of size \ref SC3_BUFSIZE, and returns 0.
 * Otherwise the function does nothing.  \a r may be NULL.
 */
#define SC3E_IS(f,o,r)                                                  \
  do { if ((r) == NULL) {                                               \
    if (!(f ((o), NULL))) { return 0; }} else {                         \
    char _r[SC3_BUFSIZE];                                               \
    if (!(f ((o), _r))) {                                               \
      sc3_snprintf ((r), SC3_BUFSIZE, "%s(%s): %s", #f, #o, _r);        \
      return 0; }}} while (0)

/** Query an sc3_object_is2_* function.
 * The argument \a f such a function and \a o1 an object to query
 * with another argument \a p.
 * If the test returns false, the function puts the reason into \a r,
 * a pointer to a string of size \ref SC3_BUFSIZE, and returns 0.
 * Otherwise the function does nothing.  \a r may be NULL.
 */
#define SC3E_IS2(f,o,p,r)                                               \
  do { if ((r) == NULL) {                                               \
    if (!(f ((o), (p), NULL))) { return 0; }} else {                    \
    char _r[SC3_BUFSIZE];                                               \
    if (!(f ((o), (p), _r))) {                                          \
      sc3_snprintf ((r), SC3_BUFSIZE, "%s(%s,%s): %s", #f, #o, #p, _r); \
      return 0; }}} while (0)

/*** LEAK statements for working with (non-fatal) leak errors. ***/

/** Execute an expression \a f that may produce a fatal error or a leak.
 * If the expression is not a leak error, we behave as \ref SC3E, that is,
 * stack the error into a new fatal error object and return it.
 * If the expression is a leak, we stack it into the inout argument \a l
 * after flattening its stack into one message.  Noop if \a f is NULL.
 * The inout pointer \a l must be of type \ref sc3_error_t ** and not NULL.
 * Value may contain an error of kind \ref SC3_ERROR_LEAK or NULL.
 * We only return out of the calling context on fatal error.
 */
#define SC3L(l,f) do {                                                  \
  sc3_error_t *_e = sc3_error_leak (l, f, __FILE__, __LINE__, #f);      \
  if (_e != NULL) {                                                     \
    return sc3_error_new_stack (&_e, __FILE__, __LINE__, #f);           \
  }} while (0)

/** Examine a condition \a x and add to the inout leak error \a l if false.
 * The inout pointer \a l must be of type \ref sc3_error_t ** and not NULL.
 * Value may contain an error of kind \ref SC3_ERROR_LEAK or NULL.
 * We stack the inout error with a newly generated leak error.
 * We only return out of the calling context on fatal error.
 */
#define SC3L_DEMAND(l,x) do {                                               \
  sc3_error_t *_e = sc3_error_leak_demand (l, x, __FILE__, __LINE__, #x);   \
  if (_e != NULL) {                                                         \
    return sc3_error_new_stack (&_e, __FILE__, __LINE__, #x);               \
  }} while (0)

/** Macro for error checking without hope for clean recovery.
 * If an error is encountered in calling \b f, we print its message to stderr
 * and call abort (3).  If possible, an application should react more nicely.
 */
#define SC3X(f) do {                                            \
  sc3_error_t *_e = (f);                                        \
  char _buffer[SC3_BUFSIZE];                                    \
  if (sc3_error_check (&_e, _buffer, SC3_BUFSIZE)) {            \
    fprintf (stderr, "%s\n", _buffer);                          \
    fprintf (stderr, "EX %s:%d %c:%s\n", __FILE__, __LINE__,    \
             sc3_error_kind_char[SC3_ERROR_FATAL], #f);         \
    abort ();                                                   \
  }} while (0)

#if 0
/** The severity of an error used to be a proper enum.
 * We have introduced \ref sc3_error_kind_t to be used instead.
 * \deprecated We may not be using this property any longer.
 */
typedef enum sc3_error_severity
{
  SC3_ERROR_SEVERITY_LAST,
}
sc3_error_severity_t;
#endif

/** We indicate the kind of an error condition;
 * see also \ref sc3_error_is_fatal and \ref sc3_error_is_leak.
 * It must be documented if any call may return a non-fatal error,
 * since without further action such is promoted to fatal by \ref SC3E.
 */
typedef enum sc3_error_kind
{
  SC3_ERROR_FATAL,      /**< Generic error indicating a failed program. */
  SC3_ERROR_WARNING,    /**< Generic warning that is not a fatal error. */
  SC3_ERROR_RUNTIME,    /**< Generic runtime error that is recoverable. */
  SC3_ERROR_BUG,        /**< A failed pre-/post-condition or assertion.
                             May also be a violation of call convention.
                             The program may be in an undefined state. */
  SC3_ERROR_MEMORY,     /**< Out of memory or related error.
                             Memory subsystem may be in undefined state. */
  SC3_ERROR_NETWORK,    /**< Network error, possibly unrecoverable.
                             Network subsystem is assumed dysfunctional. */
  SC3_ERROR_LEAK,       /**< Leftover allocation or reference count.
                             The library does not consider this error fatal,
                              but the application should report it. */
  SC3_ERROR_IO,         /**< Input/output error due to external reasons.
                             For example, file permissions may be missing.
                             The application should attempt to recover. */
  SC3_ERROR_USER,       /**< Interactive usage or configuration error.
                             The application must handle this cleanly
                             without producing leaks or inconsistencies. */
  SC3_ERROR_KIND_LAST   /**< Guard range of possible enumeration values. */
}
sc3_error_kind_t;

/** One capital letter abbreviating the error kind. */
extern const char   sc3_error_kind_char[SC3_ERROR_KIND_LAST];

#if 0
/** Errors may be synchronized between multiple programs or threads.
 * \deprecated We are not sure how we will be indicating synchronization.
 *             Other approaches (PETSc) use MPI communicators for programs.
 *             Unclear how a communicator can indicate thread synchronization.
 *             We might demand that all errors from threads must be synced.
 */
typedef enum sc3_error_sync
{
  SC3_ERROR_LOCAL,      /**< Error just occurred in local program/thread.*/
  SC3_ERROR_SYNCED,     /**< Error is synchronized between processes. */
  SC3_ERROR_DISAGREE,   /**< We know the error has distinct values
                             among multiple processes. */
  SC3_ERROR_SYNC_LAST   /**< Guard range of possible enumeration values. */
}
sc3_error_sync_t;

/** The error handler callback can be invoked on some errors.
 * It takes ownership of the passed in error \a e.
 * It may return NULL if the error was handled or an error object
 * whose ownership is passed to the caller.
 * Thus, when returning NULL, the input error should be unrefd.
 * When not returning NULL, the simplest is to return \a e.
 * Otherwise, \a e must be unrefd and an error with a one up reference
 * returned.
 * In other words, one reference goes in and one goes out.
 */
typedef sc3_error_t *(*sc3_error_handler_t)
                    (sc3_error_t * e, const char *funcname, void *user);
#endif

/** Check whether an error is not NULL and internally consistent.
 * The error may be valid in both its setup and usage phases.
 * \param [in] e        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer is not NULL and error consistent.
 */
int                 sc3_error_is_valid (const sc3_error_t * e, char *reason);

/** Check whether an error is not NULL, consistent and not setup.
 * This means that the error is not (yet) in its usage phase.
 * \param [in] e        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL, error consistent, not setup.
 */
int                 sc3_error_is_new (const sc3_error_t * e, char *reason);

/** Check whether an error is not NULL, internally consistent and setup.
 * This means that the error is in its usage phase.
 * \param [in] e        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL, error consistent and setup.
 */
int                 sc3_error_is_setup (const sc3_error_t * e, char *reason);

/** Check an error object to be setup and fatal.
 * An error is considered fatal if it is of the kind \ref SC3_ERROR_FATAL,
 * \ref SC3_ERROR_BUG, \ref SC3_ERROR_MEMORY or \ref SC3_ERROR_NETWORK.
 * An application may implicitly define any other condition as fatal,
 * that is, stacking/promoting such other error into one of the fatal kind.
 * \param [in] e        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if error is not NULL, setup, and is of a
 *                      fatal kind, false otherwise.
 */
int                 sc3_error_is_fatal (const sc3_error_t * e, char *reason);

/** Check an error object to be setup and of kind \ref SC3_ERROR_LEAK.
 * \param [in] e        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff error is not NULL, setup, and a leak.
 */
int                 sc3_error_is_leak (const sc3_error_t * e, char *reason);

/** Check an error object to be setup and of a specified kind.
 * \param [in] e        Any pointer.
 * \param [in] kind     Legal value of \ref sc3_error_kind_t.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff error is not NULL, setup, and of \b kind.
 */
int                 sc3_error_is2_kind (const sc3_error_t * e,
                                        sc3_error_kind_t kind, char *reason);

/** Create a new error object in its setup phase.
 * It begins with default parameters that can be overridden explicitly.
 * Setting and modifying parameters is only allowed in the setup phase.
 * The default settings are documented with the sc3_error_set_* functions.
 * Call \ref sc3_error_setup to change the error into its usage phase.
 * After that, no more parameters may be set.
 *
 * This function is the only error constructor recommended for applications,
 * since it uses the full allocator logic and complete error checking.
 *
 * \param [in,out] eator    An allocator that is setup.
 *                          The allocator is refd and remembered internally
 *                          and will be unrefd on error destruction.
 * \param [out] ep      Pointer must not be NULL.
 *                      If the function returns an error, value set to NULL.
 *                      Otherwise, value set to an error with default values.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_new (sc3_allocator_t * eator,
                                   sc3_error_t ** ep);

/** Set the error to be the top of a stack of existing errors.
 * The default stack is NULL.
 *
 * An error may only have one parent (stack) pointer.
 * To aggregate multiple ancestors into the same error,
 * a workaround is to aggregate the ancestors' messages.
 * \see sc3_error_flatten
 * \see sc3_error_accumulate
 *
 * \param [in,out] e        Error object before \ref sc3_error_setup.
 * \param [in,out] pstack   This pointer to a pointer must not be NULL.
 *                          The function takes ownership of pointed-to stack
 *                          (i.e. does not ref it) and NULLs the argument.
 *                          Pointed-to stack may be NULL or must be setup.
 *                          If called multiple times, a stack passed earlier
 *                          is unrefd internally.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_stack (sc3_error_t * e,
                                         sc3_error_t ** pstack);

/** Set the filename and line number of an error.
 * The default location is ("", 0).
 * \param [in,out] e    Error object before \ref sc3_error_setup.
 * \param [in] filename Null-terminated string.  Pointer must not be NULL.
 * \param [in] line     Line number, non-negative.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_location (sc3_error_t * e,
                                            const char *filename, int line);

/** Set the message of an error.
 * The default message is "".
 * \param [in,out] e    Error object before \ref sc3_error_setup.
 * \param [in] errmsg   Null-terminated string.  Pointer must not be NULL.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_message (sc3_error_t * e,
                                           const char *errmsg);

/** Set the kind of an error.
 * The default kind is the generic \ref SC3_ERROR_FATAL.
 * \param [in,out] e    Error object before \ref sc3_error_setup.
 * \param [in] kind     Enum value in [0, \ref SC3_ERROR_KIND_LAST).
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_kind (sc3_error_t * e,
                                        sc3_error_kind_t kind);

#if 0
/** Set the severity of an error.
 * \param [in,out] e    Error object before \ref sc3_error_setup.
 * \param [in] sev      Enum value in [0, \ref SC3_ERROR_SEVERITY_LAST).
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_severity (sc3_error_t * e,
                                            sc3_error_severity_t sev);
void                sc3_error_set_sync (sc3_error_t * e,
                                        sc3_error_sync_t syn);
sc3_error_t        *sc3_error_set_msgf (sc3_error_t * e,
                                        const char *errfmt, ...)
  __attribute__((format (printf, 2, 3)));
#endif

/** Setup an error and put it into its usable phase.
 * If a fatal stack is specified and e is not set to a
 * fatal kind, we silently promote \a e to \ref SC3_ERROR_FATAL.
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
 *
 * This function may return a leak error, as other unref functions.
 * Consider an object has been created with a counting \ref sc3_allocator_t.
 * Use the same allocator elsewhere to allocate memory and unref it there.
 * Now this error has the last remaining reference to the allocator.
 * Since the allocator has a live allocation, when this function internally
 * calls \ref sc3_allocator_unref, we will take notice and return the leak.
 *
 * \param [in,out] ep   Pointer must not be NULL and the error valid.
 *                      The refcount is decreased.  If it reaches zero,
 *                      the error is deallocated and the value set NULL.
 * \return              NULL on success, error object otherwise.
 *                      We may return fatal or kind \ref SC3_ERROR_LEAK.
 */
sc3_error_t        *sc3_error_unref (sc3_error_t ** ep);

/** Takes an error object with one remaining reference and deallocates it.
 * Destroying an error that is multiply refd produces a reference leak.
 * Does nothing if error has not been created by \ref sc3_error_new,
 * i.e. if it is a static fallback predefined by the library.
 * \param [in,out] ep       Setup error with one reference.  NULL on output.
 * \return                  An error object or NULL without errors.
 *                          When the error has more than one reference,
 *                          return an error of kind \ref SC3_ERROR_LEAK.
 */
sc3_error_t        *sc3_error_destroy (sc3_error_t ** ep);

#if 0
/** Destroy an error object and condense its stack's messages into a string.
 * Such condensation removes some structure, so this is a last-resort call.
 * \param [in,out] pe   This error will be destroyed and pointer NULLed.
 *                      If any errors occur in the process, they are ignored.
 *                      One would be that the error has multiple references.
 * \param [out] flatmsg If not NULL, existing string of length SC3_BUFSIZE
 *                      is filled with the error stack messages flattened.
 */
void                sc3_error_destroy_noerr (sc3_error_t ** pe,
                                             char *flatmsg);
#endif

/** Create a new error of parameterizable kind.
 * If internal allocation fails, return a working static error object.
 * This is useful when allocating an error with \ref sc3_error_new might fail.
 * This function is intended for use in macros when no allocator is available.
 * \param [in] kind     Any valid \ref sc3_error_kind_t.
 * \param [in] filename The filename is copied into the error object.
 *                      Pointer not NULL, string null-terminated.
 * \param [in] line     Line number set in the error.
 * \param [in] errmsg   The error message is copied into the error.
 *                      Pointer not NULL, string null-terminated.
 * \return              The new error object or a static fatal fallback.
 */
sc3_error_t        *sc3_error_new_kind (sc3_error_kind_t kind,
                                        const char *filename,
                                        int line, const char *errmsg);

/** Create a new error of the kind \ref SC3_ERROR_BUG.
 * If internal allocation fails, return a working static error object.
 * This is useful when allocating an error with \ref sc3_error_new might fail.
 * This should generally be used when the error indicates a buggy program.
 * This function is intended for use in macros when no allocator is available.
 * \param [in] filename The filename is copied into the error object.
 *                      Pointer not NULL, string null-terminated.
 * \param [in] line     Line number set in the error.
 * \param [in] errmsg   The error message is copied into the error.
 *                      Pointer not NULL, string null-terminated.
 * \return              The new error object or a static fatal fallback.
 */
sc3_error_t        *sc3_error_new_bug (const char *filename,
                                       int line, const char *errmsg);

/** Stack a given error into a new one of the kind \ref SC3_ERROR_FATAL.
 * If internal allocation fails, return a working static error object.
 * This is useful when allocating an error with \ref sc3_error_new might fail.
 * This function expects a setup error as stack, which may in turn have stack.
 * It should generally be used when the error indicates a buggy program.
 * This function is intended for use in macros when no allocator is available.
 * \param [in] pstack   We take ownership of the stack, pointer is NULLed.
 * \param [in] filename The filename is copied into the error object.
 *                      Pointer not NULL, string null-terminated.
 * \param [in] line     Line number set in the error.
 * \param [in] errmsg   The error message is copied into the error.
 *                      Pointer not NULL, string null-terminated.
 * \return              The new error object or a static fatal fallback.
 */
sc3_error_t        *sc3_error_new_stack (sc3_error_t ** pstack,
                                         const char *filename,
                                         int line, const char *errmsg);

/** Stack a given error into a new one of the same kind.
 * If internal allocation fails, return a working static error object.
 * This is useful when allocating an error with \ref sc3_error_new might fail.
 * This function expects a setup error as stack, which may in turn have stack.
 * Function may be used when multiple errors of the same kind accumulate.
 * This function is intended for use in macros when no allocator is available.
 * \param [in] pstack   We take ownership of the stack, pointer is NULLed.
 *                      The error kind of *pstack is used for the result.
 * \param [in] filename The filename is copied into the error object.
 *                      Pointer not NULL, string null-terminated.
 * \param [in] line     Line number set in the error.
 * \param [in] errmsg   The error message is copied into the error.
 *                      Pointer not NULL, string null-terminated.
 * \return              The new error object or a static fatal fallback.
 */
sc3_error_t        *sc3_error_new_inherit (sc3_error_t ** pstack,
                                           const char *filename,
                                           int line, const char *errmsg);

/** Take an error, flatten its stack into one message, and unref it.
 * This function returns fatal if any leaks occur in freeing the error.
 * It is used internally for leak handling to avoid infinite recursion.
 * Flattening may create a very long string that is still parseable.
 * Thus, an application may provide more sensible output by examining
 * the message and location of each error in the stack using the
 * sc3_error_get_ calls, unrefing each stack level individually.
 * That alternative avoids losing information about leak errors.
 * \param [in,out] pe       Not NULL and pointing to an error that is setup.
 *                          NULL on output.
 * \param [in] prefix       String to prepend to flattened message.
 *                          We create "prefix: (flattened message)".
 *                          If NULL, we just use the flattened message.
 * \param [out] flatmsg     String buffer of size at least \ref SC3_BUFSIZE.
 * \return                  NULL on success, fatal error otherwise.
 */
sc3_error_t        *sc3_error_flatten (sc3_error_t ** pe, const char *prefix,
                                       char *flatmsg);

/** Extend a collection of errors by one, working on an inout argument.
 * This allows for using SC3A and SC3E macros on the return value while at
 * the same time preserving all error information in the inout argument.
 * The newly created error takes the input error as stack.
 * Use this function to construct the new error in this function.
 * If one existing error object shall be added, use \ref sc3_error_accumulate.
 * \param [in,out] alloc    Allocator must be setup.
 *                          Used to allocate the error returned in \a pcollect.
 *                          You may unref but not destroy the allocator while
 *                          any error created by this function is still around.
 * \param [in,out] pcollect Pointer to the collection inout error.
 *                      Pointer must not be NULL, its value may be
 *                      NULL for an empty collection, or an error setup.
 *                      The error on input may be of an arbitrary kind.
 *                      The input error is not refd.  New error on output.
 * \param [in] kind     The error added to \a pcollect will be of this kind.
 * \param [in] filename Filename to use in the new error object.
 * \param [in] line     Line number to use in the new error object.
 * \param [in] errmsg   Message to include in the new error object.
 * \return              Null on success, otherwise a fatal error.
 */
sc3_error_t        *sc3_error_accum_kind (sc3_allocator_t * alloc,
                                          sc3_error_t ** pcollect,
                                          sc3_error_kind_t kind,
                                          const char *filename, int line,
                                          const char *errmsg);

/** Extend a collection of errors by one, working on an inout argument.
 * This allows for using SC3A and SC3E macros on the return value while at
 * the same time preserving all error information in the inout argument.
 * This functions accumulates multiple errors and their messages.
 * If the incoming error \a pe has a stack, all messages in the hierarchy
 * are flattened into one.  (The flattening will work recursively.)
 * We add a new error with (potentially long) message to the inout \a pcollect.
 * We return fatal on any potential leak error freeing the input error object.
 * \param [in,out] alloc    Allocator must be setup.
 *                          Used to allocate the error returned in \a pcollect.
 *                          You may unref but not destroy the allocator while
 *                          any error created by this function is still around.
 * \param [in,out] pcollect Pointer to the collection inout error.
 *                      Pointer must not be NULL, its value may be
 *                      NULL for an empty collection, or an error setup.
 *                      The input error is not refd.  New error on output.
 *                      The new error is of the same \a kind as \a pe.
 * \param [in,out] pe   Pointer to an error to accumulate must not be NULL.
 *                      Its value may be NULL, then the function noops.
 *                      Otherwise we take ownership.  Value NULL on output.
 * \param [in] filename Filename to use in the new error object.
 * \param [in] line     Line number to use in the new error object.
 * \param [in] errmsg   Message to include in the new error object.
 *                      Prepended to the flattened message from \a pe.
 *                      May be NULL for not adding the prefix.
 * \return              Null on success, otherwise a fatal error.
 */
sc3_error_t        *sc3_error_accumulate
  (sc3_allocator_t * alloc, sc3_error_t ** pcollect, sc3_error_t ** pe,
   const char *filename, int line, const char *errmsg);

/** Act on an error \a e depending on it being a leak, NULL, or other.
 * If the error is neither NULL nor a leak we return a fatal error.
 * If it is a leak, we flatten its messages and add to the inout
 * error collection \a leak.  If it is NULL, we do nothing.
 * We maintain and grow an error collection of kind \ref SC3_ERROR_LEAK.
 * This function is a restriction of \ref sc3_error_accumulate to leaks.
 * The function is intended for use in macros when no allocator is available.
 * \param [in,out] leak Pointer to an error must not be NULL.
 *                      Its value may be NULL or of kind \ref SC3_ERROR_LEAK.
 *                      On output, it is a new error stacked with a message
 *                      derived from the stack of the incoming error \a e.
 * \param [in] e        If not NULL, is examined and we take ownership.
 *                      Must be of kind \ref SC3_ERROR_LEAK for success.
 *                      If \a e is NULL, we do nothing and return NULL.
 * \param [in] filename Filename to use in the new error object.
 * \param [in] line     Line number to use in the new error object.
 * \param [in] errmsg   Message to include in the new error object.
 * \return              Null on success, otherwise a fatal error.
 */
sc3_error_t        *sc3_error_leak (sc3_error_t ** leak, sc3_error_t * e,
                                    const char *filename, int line,
                                    const char *errmsg);

/** Act on a condition that indicates a leak if false.
 * We maintain and grow an error collection of kind \ref SC3_ERROR_LEAK.
 * This function is intended for use in macros when no allocator is available.
 * \param [in,out] leak Pointer to an error must not be NULL.
 *                      Its value may be NULL or of kind \ref SC3_ERROR_LEAK.
 *                      On output, a new leak error stacked with the previous.
 * \param [in] x        If true, we do nothing.
 * \param [in] filename Filename to use in the new leak error object.
 * \param [in] line     Line number to use in the new leak error object.
 * \param [in] errmsg   Message to include in the new leak error object.
 * \return              Null on success, otherwise a fatal error.
 */
sc3_error_t        *sc3_error_leak_demand (sc3_error_t ** leak, int x,
                                           const char *filename, int line,
                                           const char *errmsg);

/** Access location string and line number in a setup error object.
 * The filename output pointer is only valid as long as the error is alive.
 * Before it ends, you must restore resource by \ref sc3_error_restore_location.
 * Otherwise there will be a fatal error returned when error is deallocated.
 * We do this since otherwise a random crash may occur downstream.
 * \param [in,out] e        Setup error object.
 * \param [out] filename    Pointer must not be NULL.
 *                          On success, set to error's filename.
 * \param [out] line        Pointer must not be NULL.
 *                          On success, set to error's line number.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_access_location (sc3_error_t * e,
                                               const char **filename,
                                               int *line);

/** Restore previously accessed location string to the error object.
 * This must be done for every \ref sc3_error_access_location call.
 * Otherwise there will be a fatal error returned when error is deallocated.
 * \param [in,out] e        Setup error used previously to access location.
 * \param [in] filename     Must be the location string previously accessed.
 * \param [in] line         Must be the line number previously accessed.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_restore_location (sc3_error_t * e,
                                                const char *filename,
                                                int line);

/** Access the message string in a setup error object.
 * The message output pointer is only valid as long as the error is alive.
 * Before it ends, you must restore resource by \ref sc3_error_restore_message.
 * Otherwise there will be a fatal error returned when error is deallocated.
 * We do this since otherwise a random crash may occur downstream.
 * \param [in] e        Setup error object.
 * \param [out] errmsg  Pointer must not be NULL.
 *                      On success, set to error's message.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_access_message (sc3_error_t * e,
                                              const char **errmsg);

/** Restore previously accessed message string to the error object.
 * This must be done for every \ref sc3_error_access_message call.
 * Otherwise there will be a fatal error returned when error is deallocated.
 * \param [in,out] e        Setup error used previously to access message.
 * \param [in] errmsg       Must be the message string previously accessed.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_restore_message (sc3_error_t * e,
                                               const char *errmsg);

/** Access the kind of a setup error object.
 * \param [in] e        Setup error object.
 * \param [out] kind    Pointer must not be NULL.
 *                      On success, set to error's kind.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_get_kind (sc3_error_t * e,
                                        sc3_error_kind_t * kind);

#if 0
/** Access the severity of a setup error object.
 * \param [in] e        Setup error object.
 * \param [out] sev     Pointer must not be NULL.
 *                      On success, set to error's severity.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_get_severity (sc3_error_t * e,
                                            sc3_error_severity_t * sev);
#endif

/** Return the next deepest error stack with an added reference.
 * The input error object must be setup.  Its stack is allowed to be NULL.
 * It is not changed by the call except for its stack to get referenced.
 * \param [in,out] e    The error object must be setup.
 * \param [out] pstack  Pointer must not be NULL.
 *                      When function returns cleanly, set to input error's
 *                      stack object.  If the stack is not NULL, it is refd.
 *                      In this case it must be unrefd when no longer needed.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_get_stack (sc3_error_t * e,
                                         sc3_error_t ** pstack);

/** Access the toplevel information in an error object and turn it into text.
 * This function goes down the error stack as an option, or not.
 * When used recursively, each level is ended by '\n'.
 * \param [in] e            This error object must be setup.
 * \param [in] recursion    Negative values select postorder (i.e. deepest
 *                          stack object first), positive values select
 *                          preorder (toplevel object first).
 * \param [in] dobasename   If true, only print the basename (3) of file.
 *                          0 selects non-recursive mode.
 * \param [out] buffer      This buffer must exist and contain at least
 *                          the input \b buflen many bytes.
 *                          NUL-terminated, often multi-line string on output.
 *                          There is no final newline at the end of the text.
 * \param [in] buflen       Positive number of bytes available in \b buffer.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_get_text (sc3_error_t * e,
                                        int recursion, int dobasename,
                                        char *buffer, size_t buflen);

/** Translate an error object into a return value and a text block.
 * \param [in,out] e        On input, address of an error pointer.
 *                          The error itself may be NULL or a valid object.
 *                          Unrefd and NULLd on output in the latter case.
 * \param [out] buffer      This buffer must exist and contain at least
 *                          the input \b buflen many bytes.
 *                          NUL-terminated, often multi-line string on output.
 *                          There is no final newline at the end of the text.
 *                          When input \a *e is NULL, set to the empty string.
 * \param [in] buflen       Positive number of bytes available in \b buffer.
 * \return                  0 if e is non-NULL and *e is NULL on input,
 *                          a negative integer otherwise.
 */
int                 sc3_error_check (sc3_error_t ** e,
                                     char *buffer, size_t buflen);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_ERROR_H */
