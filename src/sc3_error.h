/*
  This file is part of the SC Library, version 3.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

  The SC Library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with the SC Library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

/** \file sc3_error.h
 */

#ifndef SC3_ERROR_H
#define SC3_ERROR_H

#include <sc3.h>

typedef struct sc3_error sc3_error_t;

#include <sc3_alloc.h>

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

#ifndef SC_ENABLE_DEBUG
#define SC3A_CHECK(x) do ; while (0)
#define SC3A_STACK(f) do ; while (0)
#define SC3E(f) do {                                                    \
  sc3_error_t *_e = (f);                                                \
  if (sc3_error_is_fatal (_e)) {                                        \
    return sc3_error_new_stack (_e, __FILE__, __LINE__, #f);            \
  } else if (_e != NULL) {                                              \
    (void) sc3_error_destroy (&_e);                                     \
  }} while (0)
#else
#define SC3A_CHECK(x) do {                                              \
  if (!(x)) {                                                           \
    return sc3_error_new_fatal (__FILE__, __LINE__, #x);                \
  }} while (0)
#define SC3A_STACK(f) do {                                              \
  sc3_error_t *_e = (f);                                                \
  if (_e != NULL) {                                                     \
    return sc3_error_new_stack (_e, __FILE__, __LINE__, #f);            \
  }} while (0)
#define SC3E(f) do {                                                    \
  sc3_error_t *_e = (f);                                                \
  if (_e != NULL) {                                                     \
    return sc3_error_new_stack (_e, __FILE__, __LINE__, #f);            \
  }} while (0)
#endif
#define SC3E_DEMAND(x) do {                                             \
  if (!(x)) {                                                           \
    return sc3_error_new_fatal (__FILE__, __LINE__, #x);                \
  }} while (0)
#define SC3E_NONNEG(r) SC3E_DEMAND ((r) >= 0)
#define SC3E_UNREACH() do {                                             \
  return sc3_error_new_fatal (__FILE__, __LINE__, "Unreachable code");  \
  } while (0)
#define SC3E_RETVAL(r,v) do {                                           \
  SC3A_CHECK ((r) != NULL);                                             \
    *(r) = (v);                                                         \
  } while (0)
#define SC3E_INOUTP(pp,p) do {                                          \
  SC3A_CHECK ((pp) != NULL && *(pp) != NULL);                           \
  (p) = *(pp);                                                          \
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

/* TODO error functions shall not throw new errors themselves */

sc3_error_t        *sc3_error_args_new (sc3_allocator_t * eator,
                                        sc3_error_args_t ** eap);
sc3_error_t        *sc3_error_args_destroy (sc3_error_args_t ** eap);

/** Takes ownership of stack (i.e. does not ref it).
 * If called multiple times, the stack passed earlier is unref'd.
 */
sc3_error_t        *sc3_error_args_set_stack (sc3_error_args_t * ea,
                                              sc3_error_t * stack);
sc3_error_t        *sc3_error_args_set_location (sc3_error_args_t * ea,
                                                 const char *filename,
                                                 int line);
sc3_error_t        *sc3_error_args_set_msg (sc3_error_args_t * ea,
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

sc3_error_t        *sc3_error_new (sc3_error_args_t ** ea, sc3_error_t ** ep);
sc3_error_t        *sc3_error_ref (sc3_error_t * e);
sc3_error_t        *sc3_error_unref (sc3_error_t ** ep);

/** It is an error to destroy an error that is not allocated.
 * \return          0 if input error object is cleanly deallocated, -1 otherwise.
 */
int                 sc3_error_destroy (sc3_error_t ** ep);
int                 sc3_error_pop (sc3_error_t ** ep);

/* TODO: Should we pass an allocator? */
sc3_error_t        *sc3_error_new_ssm (sc3_error_severity_t sev,
                                       sc3_error_sync_t syn,
                                       const char *errmsg);

/* TODO: new_fatal and new_stack always return consistent results.
         They must not lead to an infinite loop (e.g. when out of memory). */
/* TODO: shall we pass an allocator parameter to new_fatal and new_stack? */

sc3_error_t        *sc3_error_new_fatal (const char *filename,
                                         int line, const char *errmsg);

/** Takes owership of stack (i.e. does not ref it) */
sc3_error_t        *sc3_error_new_stack (sc3_error_t * stack,
                                         const char *filename,
                                         int line, const char *errmsg);

/*** TODO need a bunch of _get_ and/or _is_ functions ***/

/** Return true if e is not NULL and has severity SC3_ERROR_FATAL. */
int                 sc3_error_is_fatal (sc3_error_t * e);

/* TODO: Choose simplicity over export/release and error checking. */
void                sc3_error_get_location (sc3_error_t * e,
                                            const char **filename, int *line);
void                sc3_error_get_message (sc3_error_t * e,
                                           const char **errmsg);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_ERROR_H */
