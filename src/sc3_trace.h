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

/** \file sc3_trace.h
 * Simple mechanism to access the current call stack.
 * In fact, what the stack is is up to the user.
 * We do not use any system or hardware access.
 * We do not use dynamic memory allocation.
 * The stack is a singly linked list made up from local variables
 * of nested scope.  There is usually no need to pop explicitly
 * since the top of the stack goes out of scope when the function
 * returns that it is associated to / lives on the actual stack of.
 */

#ifndef SC3_TRACE_H
#define SC3_TRACE_H

#include <sc3_error.h>

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** The trace is a public struct for simplicitc. */
typedef struct sc3_trace
{
  struct sc3_trace   *parent;   /**< Parent object or NULL
                                     for the root of the stack. */
  const char         *func;     /**< Identifier of function or
                                     more general interpretation. */
  int                 depth;    /**< Depth of stack from the root. */
  void               *user;     /**< Convenience context pointer. */
}
sc3_trace_t;

/** Initialize the root of a tracing stack.
 * \param [out] t       Trace object is initialized.
 * \param [in] func     Pointer is shallow copied (not its contents).
 * \param [in] user     Assigned to \a user member of trace \a t.
 */
void                sc3_trace_init (sc3_trace_t * t, const char *func,
                                    void *user);

/** Push a new level onto tracing stack.
 * \param [in,out] t    Non-NULL pointer.  Value is NULL or a valid trace.
 *                      This object serves as parent in the growing stack.
 *                      On output, value is set to the new top of the stack.
 * \param [out] stackvar    Trace on the memory stack of the calling function
 *                          such that it goes out of scope when caller returns.
 *                          Initialized with \a func and \a user and taking
 *                          the input value of \a t as parent object.
 *                          Its depth is increased by one over the parent.
 * \param [in] func     Pointer is shallow copied (not its contents).
 * \param [in] user     Assigned to \a user member of new trace.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_trace_push (sc3_trace_t ** t, sc3_trace_t * stackvar,
                                    const char *func, void *user);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_TRACE_H */
