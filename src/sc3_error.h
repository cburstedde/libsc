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
typedef struct sc3_error_args sc3_error_args_t;

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
  if (_e != NULL) {                                                     \
    sc3_error_destroy (_e);                                             \
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
#define SC3A_RETVAL(r,v) do {                                           \
  SC3A_CHECK ((r) != NULL);                                             \
    *(r) = (v);                                                         \
  } while (0)

typedef enum sc3_error_severity
{
  SC3_RUNTIME,
  SC3_WARNING,
  SC3_FATAL,
  SC3_ERROR_SEVERITY_LAST
}
sc3_error_severity_t;

typedef enum sc3_error_sync
{
  SC3_LOCAL,
  SC3_SYNCED,
  SC3_DISAGREE,
  SC3_ERROR_SYNC_LAST
}
sc3_error_sync_t;

/*** TODO pass counting memory allocator to constructor */

/*** TODO implement reference counting */

/* TODO error functions shall not throw new errors themselves */

sc3_error_args_t   *sc3_error_args_new (void);
void                sc3_error_args_set_child (sc3_error_args_t * ea,
                                              sc3_error_t ec);
void                sc3_error_args_set_severity (sc3_error_args_t * ea,
                                                 sc3_error_severity_t sev);
void                sc3_error_args_set_sync (sc3_error_args_t * ea,
                                             sc3_error_sync_t syn);
void                sc3_error_args_set_file (sc3_error_args_t * ea,
                                             const char *filename);
void                sc3_error_args_set_line (sc3_error_args_t * ea, int line);
void                sc3_error_args_set_msgf (sc3_error_args_t * ea,
                                             const char *errfmt, ...)
  __attribute__ ((format (printf, 2, 3)));
void                sc3_error_args_destroy (sc3_error_args_t * ea);

sc3_error_t        *sc3_error_new (sc3_error_args_t * ea);

sc3_error_t        *sc3_error_new_ssm (sc3_error_severity_t sev,
                                       sc3_error_sync_t syn,
                                       const char *errmsg);
sc3_error_t        *sc3_error_new_stack (sc3_error_t * stack,
                                         const char *filename,
                                         int line, const char *errmsg);
sc3_error_t        *sc3_error_new_fatal (const char *filename,
                                         int line, const char *errmsg);

/*** TODO need a bunch of _get_ and/or _is_ functions ***/

void                sc3_error_destroy (sc3_error_t * e);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_ERROR_H */
