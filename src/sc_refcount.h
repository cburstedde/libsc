/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System

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

/** \file sc_refcount.h
 *
 * Provide reference counting facilities.
 * The functions in this file can be used for multiple purposes.
 * The current setup is not so much targeted at garbage collection but rather
 * intended for debugging and verification.
 */

#ifndef SC_REFCOUNT_H
#define SC_REFCOUNT_H

#include <sc.h>

SC_EXTERN_C_BEGIN;

/** The refcount structure is declared in public so its size is known. */
typedef struct sc_refcount
{
  /** The reference count is always positive for a valid counter. */
  int                 refcount;
}
sc_refcount_t;

/** With --enable-debug, return the number of active reference counters.
 * This function is thread safe if configured with --enable-pthread.
 * This function always returns zero when not in debugging mode.
 * \return          With --enable-debug, the number of active (that is
 *                  initialized and positive) reference counters.
 *                  When not in debug mode, always return zero.
 */
int                 sc_refcount_get_n_active (void);

/** Initialize a reference counter to 1.
 * It is legal if its status prior to this call is undefined.
 * \param [out] rc          This reference counter is initialized to one.
 *                          The object's contents may be undefined on input.
 */
void                sc_refcount_init (sc_refcount_t * rc);

/** Create a new reference counter with count initialized to 1.
 * Equivalent to calling \ref sc_refcount_init on a newly allocated refcount_t
 * object.
 * \return                  A reference counter with count one.
 */
sc_refcount_t      *sc_refcount_new (void);

/** Destroy a reference counter.
 * It must have been count down to zero before, thus reached an inactive state.
 * \param [in,out] rc       This reference counter must have reached count zero.
 */
void                sc_refcount_destroy (sc_refcount_t * rc);

/** Increase a reference counter.
 * The counter must be active, that is, have a value greater than zero.
 * \param [in,out] rc       This reference counter must be valid (greater zero).
 *                          Its count is increased by one.
 */
void                sc_refcount_ref (sc_refcount_t * rc);

/** Decrease the reference counter and notify when it reaches zero.
 * The count must be greater zero on input.  If the reference count reaches
 * zero, which is indicated by the return value, the counter may not be used
 * furter with \ref sc_refcount_ref or \see sc_refcount_unref.  It is legal,
 * however, to reactivate it later by calling \see sc_refcount_init.
 * \param [in,out] rc       This reference counter must be valid (greater zero).
 *                          Its count is decreased by one.
 * \return          True if the count has reached zero, false otherwise.
 */
int                 sc_refcount_unref (sc_refcount_t * rc);

SC_EXTERN_C_END;

#endif /* !SC_REFCOUNT_H */
