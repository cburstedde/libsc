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

#ifndef SC_KEYVALUE_H
#define SC_KEYVALUE_H

#include <sc.h>
#include <sc_containers.h>

SC_EXTERN_C_BEGIN;

typedef enum
{
  SC_KEYVALUE_ENTRY_NONE = 0,
  SC_KEYVALUE_ENTRY_INT,
  SC_KEYVALUE_ENTRY_DOUBLE,
  SC_KEYVALUE_ENTRY_STRING,
  SC_KEYVALUE_ENTRY_POINTER
}
sc_keyvalue_entry_type_t;

typedef struct sc_keyvalue
{
  sc_hash_t          *hash;
  sc_mempool_t       *value_allocator;
}
sc_keyvalue_t;

/* Constructors / destructors */
sc_keyvalue_t      *sc_keyvalue_new ();

/* Arguments come in pairs of 2: static string "type:key" and value;
   type is a letter like the identifier names in sc_keyvalue_entry.value */
sc_keyvalue_t      *sc_keyvalue_newf (int dummy, ...);
sc_keyvalue_t      *sc_keyvalue_newv (va_list ap);

void                sc_keyvalue_destroy (sc_keyvalue_t * kv);

/* Routine to check existence of an entry
   returns the type if found, and SC_KEYVALUE_ENTRY_NONE otherwise */
sc_keyvalue_entry_type_t sc_keyvalue_exists (sc_keyvalue_t * kv,
                                             const char *key);

/* Routine to remove an entry
   returne type if found and removed, SC_KEYVALUE_ENTRY_NONE otherwise */
sc_keyvalue_entry_type_t sc_keyvalue_unset (sc_keyvalue_t * kv,
                                            const char *key);

/* Routines to extract values from keys
   if the key is not present then dvalue is returned */
int                 sc_keyvalue_get_int (sc_keyvalue_t * kv,
                                         const char *key, int dvalue);
double              sc_keyvalue_get_double (sc_keyvalue_t * kv,
                                            const char *key, double dvalue);
const char         *sc_keyvalue_get_string (sc_keyvalue_t * kv,
                                            const char *key,
                                            const char *dvalue);
void               *sc_keyvalue_get_pointer (sc_keyvalue_t * kv,
                                             const char *key, void *dvalue);

/* Routines to set values for a given key */
void                sc_keyvalue_set_int (sc_keyvalue_t * kv,
                                         const char *key, int newvalue);
void                sc_keyvalue_set_double (sc_keyvalue_t * kv,
                                            const char *key, double newvalue);
void                sc_keyvalue_set_string (sc_keyvalue_t * kv,
                                            const char *key,
                                            const char *newvalue);
void                sc_keyvalue_set_pointer (sc_keyvalue_t * kv,
                                             const char *key, void *newvalue);

/** Function to call on every key value pair
 * \param [in] key   The key for this pair
 * \param [in] type  The type of entry
 * \param [in] entry Pointer to the entry
 * \param [in] u     Arbitrary user data.
 * \return Return true if the traversal should continue, false to stop.
 */
typedef int         (*sc_keyvalue_foreach_t) (const char *key,
                                              const sc_keyvalue_entry_type_t
                                              type, void *entry,
                                              const void *u);

void                sc_keyvalue_foreach (sc_keyvalue_t * kv,
                                         sc_keyvalue_foreach_t fn,
                                         void *user_data);
SC_EXTERN_C_END;

#endif /* !SC_KEYVALUE_H */
