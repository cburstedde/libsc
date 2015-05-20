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
 *  Provide reference counting facilities.
 */

/*
 * The idea is not so much to enable garbage collection but rather
 * to ensure that no objects are referenced after their deallocation.
 */

#ifndef SC_REFCOUNT_H
#define SC_REFCOUNT_H

#include <sc.h>

SC_EXTERN_C_BEGIN;

typedef struct sc_refcount
{
  int                 refcount;
}
sc_refcount_t;

/** Return the number of reference counters that are active.
 */
int                 sc_refcount_get_n_active (void);

/** Initialize a reference counter to 1.
 * It is legal if its status prior to this call is undefined.
 */
void                sc_refcount_init (sc_refcount_t * rc);

/** Create a new reference counter with count initialized to 1.
 * Equivalent to calling sc_refcount_init on a newly allocated refcount_t.
 */
sc_refcount_t   *sc_refcount_new (void);

/** Destroy a reference counter.  It must have count 0.
 */
void                sc_refcount_destroy (sc_refcount_t * rc);

/** Increase the reference count by 1.  The count must already be greater zero.
 */
void                sc_refcount_ref (sc_refcount_t * rc);

/** Decrease the reference count by 1.  The count must thus be greater zero.
 * If the reference count has reached zero, which is known by the return value,
 * the counter may be reactivated later by calling sc_refcount_init.
 * \return          True if the count has reached zero, false otherwise.
 */
int                 sc_refcount_unref (sc_refcount_t * rc);

SC_EXTERN_C_END;

#endif /* !SC_REFCOUNT_H */
