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
 * We propagate fatal errors up the call stack by returning a pointer to \ref
 * sc3_error_t from most library functions.  Non-NULL is meant to indicate
 * unrecoverable or undefined conditions, when it is not safe to proceed.
 * For example, we may be out of memory, encounter an MPI communication error,
 * or trip an assertion or reference or memory leak.  We do not aim to proceed.
 * This means, we need not clean up state when returning such an error object.
 * Put differently, no caller may expect a defined state when being returend
 * a non-NULL pointer to \ref sc3_error_t.
 *
 * If a function returns any of these hard error conditions, the application
 * must consider that it returned prematurely and future crashes are likely.
 * In addition, communication and I/O may be pending and cause blocking.
 * It is the application's responsibility to take this into account, for
 * example by reporting the error and shutting down as soon as possible.
 * The SC library does not abort on its own to allow for clean shutdown.
 *
 * Error objects can be connected as linked list to represent the call stack.
 * To propagate fatal errors up the call stack, we provide the SC3A and SC3E
 * types of macros and associated convenience functions.
 * The SC3A macros (assertions) are only active when configured `--enable-debug`
 * and intended to check conditions that are generally true in absence of bugs.
 * The SC3E macros (executions) are always on, intended to call subroutines.
 * Both examine error returns and conditions and return fatal errors themselves.
 * These macros are understood to effect immediate function return on error.
 *
 * When internal allocation fails or the library finds internal bugs, we may
 * return internal static error objects.  The application is expected to
 * treat them normally, the library allowing but ignoring refing and unrefing
 * for those.  We return as directly as possible when out of memory.
 *
 * Non-fatal errors are imaginable when accessing files on disk or parsing
 * input.  We do not envision the \ref sc3_error_t to be used for this purpose.
 * It is up to the application to define and implement error handling that
 * ideally reports the situation and either continues or shuts down cleanly.
 * For example, the return value may be used for fatal conditions, while
 * recoverable errors are returned through a call-by-reference argument.
 *
 * Errors are created in a RIAA fashion with a relaxed setup phase.
 * An error comes to life in \ref sc3_error_new but is not yet usable.
 * Its properties must be set and then finalized with \ref sc3_error_setup.
 * Only then is the error ready for use, querying, and refing and unrefing.
 * As an exception, an error may be unrefd before setup is complete.
 * To drop responsibility for an error object, use \ref sc3_error_unref.
 * This deallocates the error when the last reference is dropped.
 * You may use \ref sc3_error_destroy when refcount is known to be 1.
 *
 * The object query functions sc3_error_is?_* shall return cleanly on
 * any kind of input parameters, even on those that do not make sense.
 * Query functions must be designed not to crash under any circumstance.
 * On incorrect or undefined input, they must return false.
 * On defined input, they return the proper query result.
 * This may lead to the situation that sc3_object_is_A and sc3_object_is_not_A
 * both return false, say when the call convention is violated.
 *
 * The following functions are used to construct error objects.
 *  * \ref sc3_error_new must be the first to call.
 *  * \ref sc3_error_set_location
 *  * \ref sc3_error_set_message
 *  * \ref sc3_error_set_kind to designate one of several conditions.
 *  * \ref sc3_error_set_stack to link with an existing error or stack.
 *  * \ref sc3_error_setup must be called after the set functions.
 *
 * At this point, refing and unrefing become legal, but no more setting.
 *
 * Error objects after setup can be accessed by the following functions.
 *  * \ref sc3_error_access_location
 *  * \ref sc3_error_restore_location must be called for every location access
 *  * \ref sc3_error_access_message
 *  * \ref sc3_error_restore_message must be called for every message access
 *  * \ref sc3_error_get_kind
 *  * \ref sc3_error_ref_stack yields NULL or an error that is accessed
 *                             and dropped in the same way.
 *  * \ref sc3_error_copy_text creates a text summary of all error information.
 *
 * As a convenience for calling libsc from an application, we have added \ref
 * sc3_error_check.  It examines an error passed in and returns 0 if the error
 * is NULL.  Otherwise, it extracts its text block using \ref
 * sc3_error_copy_text, calls \ref sc3_error_unref and returns -1.  This way,
 * an application program can react on the return value and use the message for
 * its own reporting.  What is lost by this approach is the ability to
 * distinguish various kinds of error after the conversion.
 *
 * Fatal error handling is free of dependencies on the rest of the library.
 * Error objects are allocated on the heap using malloc and free.
 * They are thread safe insofar as malloc and free are thread safe.
 * Since we do not use any locking, the same error object must not be
 * manipulated concurrently by different threads.
 */

#ifndef SC3_ERROR_H
#define SC3_ERROR_H

#include <sc3_base.h>

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** The sc3 error object is an opaque structure. */
typedef struct sc3_error sc3_error_t;

/** Allocate memory and copy a Nul-terminated string into it.
 * Unlike strdup (3), memory is passed by a reference argument.
 * \param [in] src      Nul-terminated string.
 * \param [out] dest    Non-NULL pointer to the address of output,
 *                      memory with the input string copied into it.
 *                      Release eventually with \ref sc3_free.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_strdup (const char *src, char **dest);

/** Allocate memory with error checking.
 * Unlike malloc (3), memory is passed by a reference argument.
 * \param [in] size     Bytes to allocate as in malloc (3).
 * \param [out] pmem    Non-NULL pointer to the address of memory.
 *                      Output value according to malloc (3).
 *                      On error return, output is undefined.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_malloc (size_t size, void *pmem);

/** Allocate zeroed-out memory with error checking.
 * Unlike calloc (3), memory is passed by a reference argument.
 * \param [in] nmemb    Number of items to allocate as in calloc (3).
 * \param [in] size     Bytes per item to allocate as in calloc (3).
 * \param [out] pmem    Non-NULL pointer to the address of memory.
 *                      Output value according to calloc (3).
 *                      On error return, output is undefined.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_calloc (size_t nmemb, size_t size, void *pmem);

/** Reallocate memory with error checking.
 * Unlike realloc (3), memory is passed by a reference argument.
 * \param [in,out] pmem     Non-NULL pointer to the address of memory.
 *                          In/out value according to realloc (3).
 *                          On error return, output is undefined.
 * \param [in] size         Bytes to allocate as in realloc (3).
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_realloc (void *pmem, size_t size);

/** Free memory previously allocated.
 * Unlike free (3), this function accepts a reference argument.
 * The argument value (freed address) is set to NULL on output.
 * \param [in,out] pmem     Non-NULL pointer to pointer to be freed.
 *                          Output value is set to NULL on success.
 *                          On error return, output is undefined.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_free (void *pmem);

/*** DEBUG statements do nothing unless configured with --enable-debug. ***/

#ifndef SC_ENABLE_DEBUG

/** Assertion statement to require a true sc3_object_is_* return value.
 * The argument \a f is such a function and \a o the object to query. */
#define SC3A_IS(f,o) SC3_NOOP

/** Assertion statement to require a true sc3_object_is2_* return value.
 * The argument \a f is such a function taking \a o the object to query,
 * a second argument \a p (and the common string buffer as all of them). */
#define SC3A_IS2(f,o,p) SC3_NOOP

/** Assertion statement to require a true sc3_object_is3_* return value.
 * The argument \a f is such a function taking \a o the object to query,
 * the next arguments are \a p1 and \a p2 (and the common string buffer
 * as all of them). */
#define SC3A_IS3(f,o,p1,p2) SC3_NOOP

/** Assertion statement requires some condition \a x to be true. */
#define SC3A_CHECK(x) SC3_NOOP

#else
#define SC3A_IS(f,o) SC3E_DEMIS (f, o, SC3_ERROR_ASSERT)
#define SC3A_IS2(f,o,p) SC3E_DEMIS2 (f, o, p, SC3_ERROR_ASSERT)
#define SC3A_IS3(f,o,p1,p2) SC3E_DEMIS3 (f, o, p1, p2, SC3_ERROR_ASSERT)
#define SC3A_CHECK(x) do {                                              \
  if (!(x)) {                                                           \
    return sc3_error_new_kind (SC3_ERROR_ASSERT,                        \
                               __FILE__, __LINE__, #x);                 \
  }} while (0)
#endif

/*** ERROR statements are always active and return fatal errors. ***/

/** Execute an expression \a f that produces an \ref sc3_error_t object.
 * If that error is not NULL, create a fatal error and return it.
 * This macro stacks every non-NULL error returned into a fatal error.
 */
#define SC3E(f) do {                                                    \
  sc3_error_t *_e = (f);                                                \
  if (_e != NULL) {                                                     \
    return sc3_error_new_stack (&_e, __FILE__, __LINE__, #f);           \
  }} while (0)

/** Return \ref sc3_error_t object with custom kind \a k and message \a s.
 */
#define SC3E_RETURN(k,s) do {                                           \
  return sc3_error_new_kind ((k), __FILE__, __LINE__, (s));             \
  } while (0)

/** If a condition \a x is not met, create a fatal error and return it.
 * The kind of the fatal error to create is passed in as argument \a k.
 * The error message is set to the provided string \a s.
 */
#define SC3E_DEMAND(x,k,s) do {                                         \
  if (!(x)) {                                                           \
    char _errmsg[SC3_BUFSIZE];                                          \
    sc3_snprintf (_errmsg, SC3_BUFSIZE, "%s", s);                       \
    return sc3_error_new_kind ((k), __FILE__, __LINE__, _errmsg);       \
  }} while (0)

/** Depending on a failed condition, create an \ref SC3_ERROR_LEAK. */
#define SC3E_DEM_LEAK(x,s) SC3E_DEMAND(x, SC3_ERROR_LEAK, s)

/** Depending on a failed condition, create an \ref SC3_ERROR_INVALID. */
#define SC3E_DEM_INVALID(x,s) SC3E_DEMAND(x, SC3_ERROR_INVALID, s)

/** Execute an is_* query \a f and return a fatal error if false.
 * The kind of the fatal error to create is passed in as an argument.
 * Its message is set to the failed query function and its object \a o.
 */
#define SC3E_DEMIS(f,o,k) do {                                          \
  char _r[SC3_BUFSIZE];                                                 \
  if (!(f ((o), _r))) {                                                 \
    char _errmsg[SC3_BUFSIZE];                                          \
    sc3_snprintf (_errmsg, SC3_BUFSIZE, "%s(%s): %s", #f, #o, _r);      \
    return sc3_error_new_kind ((k), __FILE__, __LINE__, _errmsg);       \
  }} while (0)

/** Execute an is_* query \a f that takes an additional argument.
 * Return a fatal error if the query returns false.
 * The kind of the fatal error to create is passed in as an argument.
 * Its message is set to the failed query function and its object \a o.
 */
#define SC3E_DEMIS2(f,o,p,k) do {                                       \
  char _r[SC3_BUFSIZE];                                                 \
  if (!(f ((o), (p), _r))) {                                            \
    char _errmsg[SC3_BUFSIZE];                                          \
    sc3_snprintf (_errmsg, SC3_BUFSIZE,                                 \
                  "%s(%s,%s): %s", #f, #o, #p, _r);                     \
    return sc3_error_new_kind ((k), __FILE__, __LINE__, _errmsg);       \
  }} while (0)

/** Execute an is_* query \a f that takes two additional arguments.
 * Return a fatal error if the query returns false.
 * The kind of the fatal error to create is passed in as an argument.
 * Its message is set to the failed query function and its object \a o.
 */
#define SC3E_DEMIS3(f,o,p1,p2,k) do {                                   \
  char _r[SC3_BUFSIZE];                                                 \
  if (!(f ((o), (p1), (p2), _r))) {                                     \
    char _errmsg[SC3_BUFSIZE];                                          \
    sc3_snprintf (_errmsg, SC3_BUFSIZE,                                 \
                  "%s(%s,%s,%s): %s", #f, #o, #p1, #p2, _r);            \
    return sc3_error_new_kind ((k), __FILE__, __LINE__, _errmsg);       \
  }} while (0)

/** When encountering unreachable code, return a fatal error.
 * The error created is of type \ref SC3_ERROR_INVALID
 * and its message is provided as parameter \a s.
 */
#define SC3E_UNREACH(s) do {                                            \
  char _errmsg[SC3_BUFSIZE];                                            \
  sc3_snprintf (_errmsg, SC3_BUFSIZE, "Unreachable: %s", (s));          \
  return sc3_error_new_kind (SC3_ERROR_INVALID,                         \
                             __FILE__, __LINE__, _errmsg);              \
  } while (0)

/*** VARIABLE statements.  Prepare in/out reference variables. ***/

/** Assert a pointer \a r not to be NULL and initialize its value to \a v.
 */
#define SC3E_RETVAL(r,v) do {                                           \
  SC3A_CHECK ((r) != NULL);                                             \
  *(r) = (v);                                                           \
  } while (0)

/** Require an input pointer \a pp to dereference to a non-NULL value.
 * The value is assigned to the pointer \a p for future use.
 */
#define SC3E_INOUTP(pp,p) do {                                          \
  SC3A_CHECK ((pp) != NULL && *(pp) != NULL);                           \
  (p) = *(pp);                                                          \
  } while (0)

/** Require an input pointer \a pp to dereference to a non-NULL value.
 * Assign its value to \a p for future use, then set input value to NULL.
 */
#define SC3E_INULLP(pp,p) do {                                          \
  SC3A_CHECK ((pp) != NULL && *(pp) != NULL);                           \
  (p) = *(pp);                                                          \
  *(pp) = NULL;                                                         \
  } while (0)

/*** QUERY statements.  Executed irrespective of debug setting. ***/

/** Set the reason output parameter \a r of a query and return false.
 * Set the reason parameter inside an sc3_object_is_* function
 * to a given value \a reason before unconditionally returning false.
 * \a r may be NULL in which case it is not updated, but we still return.
 */
#define SC3E_NO(r,reason) do {                                          \
  if ((r) != NULL) { SC3_BUFCOPY ((r), (reason)); } return 0; } while (0)

/** Set the reason output parameter \a r of a query and return true.
 * Set the reason parameter inside an sc3_object_is_* function
 * to "" before returning true unconditionally.
 * \a r may be NULL in which case it is not updated, but we still return.
 */
#define SC3E_YES(r) do {                                                \
  if ((r) != NULL) { (r)[0] = '\0'; } return 1; } while (0)

/** Query condition \a x to be true, in which case the macro does nothing.
 * If the condition is false, the put the reason into \a r, a pointer
 * to a string of size \ref SC3_BUFSIZE, and return false.
 */
#define SC3E_TEST(x,r)                                                  \
  do { if (!(x)) {                                                      \
    if ((r) != NULL) { SC3_BUFCOPY ((r), #x); }                         \
    return 0; }} while (0)

/** Run a function that may return an error for use in yes/no tests.
 * Since the caller will not return an error, we convert it to integer.
 * As with the other macros here, \a r must be of \ref SC3_BUFSIZE.
 */
#define SC3E_DO(f,r) do {                                               \
  sc3_error_t *_e = (f);                                                \
  if (sc3_error_check (r, SC3_BUFSIZE, _e)) { return 0; }} while (0)

/** Query an sc3_object_is_* function.
 * The argument \a f shall be such a function and \a o an object to query.
 * If the test returns false, the function puts the reason into \a r,
 * a pointer to a string of size \ref SC3_BUFSIZE, and returns false.
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
 * The argument \a f shall be such a function and \a o an object to query
 * with another argument \a p.
 * If the test returns false, the function puts the reason into \a r,
 * a pointer to a string of size \ref SC3_BUFSIZE, and returns false.
 * Otherwise the function does nothing.  \a r may be NULL.
 */
#define SC3E_IS2(f,o,p,r)                                               \
  do { if ((r) == NULL) {                                               \
    if (!(f ((o), (p), NULL))) { return 0; }} else {                    \
    char _r[SC3_BUFSIZE];                                               \
    if (!(f ((o), (p), _r))) {                                          \
      sc3_snprintf ((r), SC3_BUFSIZE, "%s(%s,%s): %s", #f, #o, #p, _r); \
      return 0; }}} while (0)

/** Query an sc3_object_is3_* function.
 * The argument \a f shall be such a function and \a o an object to query
 * with another arguments \a p1 and \a p2.
 * If the test returns false, the function puts the reason into \a r,
 * a pointer to a string of size \ref SC3_BUFSIZE, and returns false.
 * Otherwise the function does nothing.  \a r may be NULL.
 */
#define SC3E_IS3(f,o,p1,p2,r)                                           \
  do { if ((r) == NULL) {                                               \
    if (!(f ((o), (p1), (p2), NULL))) { return 0; }} else {             \
    char _r[SC3_BUFSIZE];                                               \
    if (!(f ((o), (p1), (p2), _r))) {                                   \
      sc3_snprintf ((r), SC3_BUFSIZE, "%s(%s,%s,%s): %s",               \
                                      #f, #o, #p1, #p2, _r);            \
      return 0; }}} while (0)

/** We indicate the kind of a fatal error condition. */
typedef enum sc3_error_kind
{
  SC3_ERROR_FATAL,      /**< Generic error indicating a failed program. */
  SC3_ERROR_ASSERT,     /**< Failed pre-/post-condition or assertion. */
  SC3_ERROR_LEAK,       /**< Hanging memory allocation. */
  SC3_ERROR_REF,        /**< Reference counting misuse. */
  SC3_ERROR_MEMORY,     /**< Out of memory. */
  SC3_ERROR_NETWORK,    /**< Network error. */
  SC3_ERROR_REQUIRED,   /**< Required feature not available. */
  SC3_ERROR_INVALID,    /**< Invalid condition or situation. */
  SC3_ERROR_UNKNOWN,    /**< Unknown fatal error. */
  SC3_ERROR_KIND_LAST   /**< Guard range of possible enumeration values. */
}
sc3_error_kind_t;

/** One capital letter abbreviating the error kind.
 * It is its first letter except for SC3_ERROR_REQUIRED, where it is 'Q'.
 */
extern const char   sc3_error_kind_char[SC3_ERROR_KIND_LAST];

/** Check whether an error is not NULL and internally consistent.
 * The error may be valid in both its setup and usage phases.
 * \param [in] e        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if pointer is not NULL and error consistent.
 */
int                 sc3_error_is_valid (const sc3_error_t * e, char *reason);

/** Check whether an error is not NULL, consistent and not setup.
 * This means that the error is not (yet) in its usage phase.
 * \param [in] e        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if pointer not NULL, error consistent, not setup.
 */
int                 sc3_error_is_new (const sc3_error_t * e, char *reason);

/** Check whether an error is not NULL, internally consistent and setup.
 * This means that the error is in its usage phase.
 * \param [in] e        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if pointer not NULL, error consistent and setup.
 */
int                 sc3_error_is_setup (const sc3_error_t * e, char *reason);

/** Check an error object to be setup and of a specified kind.
 * \param [in] e        Any pointer.
 * \param [in] kind     Legal value of \ref sc3_error_kind_t.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if error is not NULL, setup, and of \a kind.
 */
int                 sc3_error_is2_kind (const sc3_error_t * e,
                                        sc3_error_kind_t kind, char *reason);

/** Create a new error object in its setup phase.
 * The error begins with default parameters that can be overridden.
 * Setting and modifying parameters is only allowed in the setup phase.
 * The default settings are documented with the `sc3_error_set_*` functions.
 * Call \ref sc3_error_setup to change the error into its usage phase.
 * After that, no more parameters may be set.
 *
 * \param [in,out] ep   Pointer must not be NULL.  Input value ignored.
 *                      Value set to an error with default values.
 * \return              NULL on successful error creation.
 *                      Otherwise, output argument \a ep is undefined
 *                      and any internal error encountered is returned.
 */
sc3_error_t        *sc3_error_new (sc3_error_t ** ep);

/** Set the error to be the top of a stack of existing errors.
 * The default stack is NULL.
 *
 * An error may only have one parent pointer.
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

/** Set the kind of an error.
 * The default kind is the generic \ref SC3_ERROR_FATAL.
 * \param [in,out] e    Error object before \ref sc3_error_setup.
 * \param [in] kind     Enum value in [0, \ref SC3_ERROR_KIND_LAST).
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_kind (sc3_error_t * e,
                                        sc3_error_kind_t kind);

/** Set the filename and line number of an error.
 * The default location is ("", 0).
 * \param [in,out] e    Error object before \ref sc3_error_setup.
 * \param [in] filename Nul-terminated string.  Pointer must not be NULL.
 * \param [in] line     Line number, non-negative.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_location (sc3_error_t * e,
                                            const char *filename, int line);

/** Set the message of an error.
 * The default message is "".
 * \param [in,out] e    Error object before \ref sc3_error_setup.
 * \param [in] errmsg   Nul-terminated string.  Pointer must not be NULL.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_message (sc3_error_t * e,
                                           const char *errmsg);

/** Set the message of an error in the style of printf (3)
 * The default message is "".
 * \param [in,out] e    Error object before \ref sc3_error_setup.
 * \param [in] fmt      Nul-terminated string.  Pointer must not be NULL.
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_messagef (sc3_error_t * e,
                                            const char *fmt, ...)
  __attribute__ ((format (printf, 2, 3)));

/** Set the message of an error in the style of printf (3)
 * The default message is "".
 * \param [in,out] e    Error object before \ref sc3_error_setup.
 * \param [in] fmt      Nul-terminated string.  Pointer must not be NULL.
 * \param [in] ap       Parameter list macro as with stdarg (3).
 * \return              An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_set_messagev (sc3_error_t * e,
                                            const char *fmt, va_list ap);

/** Setup an error and put it into its usable phase.
 * \param [in,out] e    This error must not yet be setup.
 *                      Setup phase ends and it is put into its usable phase.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_setup (sc3_error_t * e);

/** Increase the reference count on an error object by 1.
 * This is only allowed after the error has been setup.
 * \param [in,out] e    This error must be setup.  Its refcount is increased.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_ref (sc3_error_t * e);

/** Decrease the reference of an error object by 1.
 * If the count reaches zero the error object is deallocated.
 * \param [in,out] ep   Pointer must not be NULL and the error valid.
 *                      The refcount is decreased.  If it reaches zero,
 *                      the error is deallocated and stops existing,
 *                      and the value is set to NULL most of the time.
 *                      It is legal, however, for the output to stay
 *                      non-NULL (say by refing occurring elsewhere,
 *                      or the error being a static internal object).
 *                      This safety feature should not be relied upon.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_unref (sc3_error_t ** ep);

/** Take an error object with one remaining reference and deallocate it.
 * Destroying an error that is multiply refd returns a reference error.
 * Previous refing and unrefing in the program must thus be predictable.
 * \param [in,out] ep       Setup error with one reference.  NULL on output.
 * \return                  An error object or NULL without errors.
 */
sc3_error_t        *sc3_error_destroy (sc3_error_t ** ep);

/** Make error of given kind and return either it or any error creating it.
 *
 * Contrary to most other libsc functions, its return value is its output.
 * The return value is never NULL, but the newly made error on success.
 *
 * This function is intended to be used by macros seeding an error stack.
 *
 * \param [in] kind     Any valid \ref sc3_error_kind_t.
 * \param [in] filename The filename is copied into the error object.
 *                      Pointer not NULL, string Nul-terminated.
 * \param [in] line     Non-negative line number set in the error.
 * \param [in] errmsg   The error message is copied into the error.
 *                      Pointer not NULL, string Nul-terminated.
 * \return              Never NULL, but rather the newly made error,
 *                      or any error encountered in constructing it.
 */
sc3_error_t        *sc3_error_new_kind (sc3_error_kind_t kind,
                                        const char *filename,
                                        int line, const char *errmsg);

/** Make stacked fatal error and return either it or any error creating it.
 * This function expects a setup error as stack, which may have more stack.
 *
 * Contrary to most other libsc functions, its return value is its output.
 * The return value is never NULL, but the newly made error on success.
 *
 * This function is intended to be used by macros building an error stack.
 *
 * \param [in] pstack   We take ownership of the stack, pointer is NULLd.
 * \param [in] filename The filename is copied into the error object.
 *                      Pointer not NULL, string Nul-terminated.
 * \param [in] line     Line number set in the error.
 * \param [in] errmsg   The error message is copied into the error.
 *                      Pointer not NULL, string Nul-terminated.
 * \return              Never NULL, but rather the newly made error,
 *                      or any error encountered in constructing it.
 */
sc3_error_t        *sc3_error_new_stack (sc3_error_t ** pstack,
                                         const char *filename,
                                         int line, const char *errmsg);

/** Return the next deepest error stack with an added reference.
 * The input error object must be setup.  Its stack is allowed to be NULL.
 * It is not changed by the call except for its stack to get referenced.
 * \param [in,out] e    The error object must be setup.
 *                      It is possible to unref the error while the refd stack
 *                      is still alive.  Don't \ref sc3_error_destroy it then.
 * \param [out] pstack  Pointer must not be NULL.
 *                      When function returns cleanly, set to input error's
 *                      stack object.  If the stack is not NULL, it is refd.
 *                      In this case it must be unrefd when no longer needed.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_ref_stack (sc3_error_t * e,
                                         sc3_error_t ** pstack);

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
sc3_error_t        *sc3_error_get_kind (const sc3_error_t * e,
                                        sc3_error_kind_t * kind);

/** Enumeration to determine recursion and if yes, which order. */
typedef enum sc3_error_recursion
{
  SC3_ERROR_RECURSION_NONE,         /**< No recursion. */
  SC3_ERROR_RECURSION_PREORDER,     /**< Preorder: top object first. */
  SC3_ERROR_RECURSION_POSTORDER,    /**< Postorder: deepest object first. */
  SC3_ERROR_RECURSION_LAST          /**< Unused guard value. */
}
sc3_error_recursion_t;

/** Access the information in an error object and turn it into text.
 * This function goes down the error stack as an option, pre or post.
 * When used recursively, each level is ended by '\n' in the output.
 * \param [in] e            This error object must be setup.
 * \param [in] recursion    Enumeration \ref sc3_error_recursion_t.
 * \param [in] dobasename   If true, only copy the basename (3) of file.
 * \param [out] buffer      If NULL, this function only checks consistency.
 *                          Otherwise must contain at least \a buflen bytes.
 *                          Nul-terminated, often multi-line string on output.
 *                          There is no final newline at the end of the text.
 * \param [in] buflen       Number of bytes available in \a buffer.
 *                          If zero, this function does nothing.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_error_copy_text (sc3_error_t * e,
                                         sc3_error_recursion_t recursion,
                                         int dobasename,
                                         char *buffer, size_t buflen);

/** Unref an error object with maximal tolerance and minimal checking.
 * Useful to clean up as much memory as possible after some fatal mishap.
 * \param [in] e        If not NULL, this error is unrefd.
 */
void                sc3_error_unref_noerr (sc3_error_t * e);

/** Translate an error value into a return value and a text block.
 * Useful to acknowledge errors from the libary without much effort.
 * This function can be called directly with the return value of an
 * error-object returning function.  Assumes ownership of the error.
 *
 * When the error is NULL, wo do not touch the output buffer and
 * its contents are undefined.  The buffer is written to if and only
 * if this function returns negative.  Otherwise buffer is untouched.
 *
 * Error is cleanly dereferenced using \ref sc3_error_unref_noerr.
 * This usually works, since we survive out-of-memory situations.
 *
 * \param [out] buffer      If this buffer is not NULL, it must provide at
 *                          least \a buflen many bytes.  On an error return,
 *                          nul-terminated, often multi-line string on output.
 *                          There is no final newline at the end of the text.
 * \param [in] buflen       Number of bytes available in \a buffer.
 *                          If zero, this function does not touch the buffer.
 * \param [in] e            If NULL, do nothing and return 0.
 *                          Otherwise, put multiline error message
 *                          into \a buffer and return negative value.
 * \return                  0 without error, a negative integer otherwise.
 */
int                 sc3_error_check (char *buffer, size_t buflen,
                                     sc3_error_t * e);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_ERROR_H */
