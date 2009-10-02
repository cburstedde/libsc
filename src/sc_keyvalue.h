/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2009 Carsten Burstedde, Lucas Wilcox.

  The SC Library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with the SC Library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SC_KEYVALUE_H
#define SC_KEYVALUE_H

#include <sc.h>
#include <sc_containers.h>

SC_EXTERN_C_BEGIN;

typedef enum
{
  SC_OBJECT_VALUE_NONE = 0,
  SC_OBJECT_VALUE_INT,
  SC_OBJECT_VALUE_DOUBLE,
  SC_OBJECT_VALUE_STRING,
  SC_OBJECT_VALUE_POINTER,
}
sc_object_value_type_t;

typedef struct sc_object_value
{
  const char         *key;
  sc_object_value_type_t type;
  union
  {
    int                 i;
    double              g;
    const char         *s;
    void               *p;
  }
  value;
}
sc_object_value_t;

typedef struct sc_keyvalue
{
  sc_hash_t          *hash;
  sc_mempool_t       *value_allocator;
}
sc_keyvalue_t;

/* passing arguments */
/* Arguments come in pairs of 2: static string "type:key" and value;
   type is a letter like the identifier names in sc_object_value.value */
sc_keyvalue_t      *sc_keyvalue_new (int dummy, ...);
sc_keyvalue_t      *sc_keyvalue_new_va (va_list ap);
void                sc_keyvalue_destroy (sc_keyvalue_t * args);

sc_object_value_type_t sc_keyvalue_exist (sc_keyvalue_t *
                                          args, const char *key);

/* Routines to extract values from keys */
/* if the key is not present then dvalue is returned */
int                 sc_keyvalue_get_int (sc_keyvalue_t * args,
                                         const char *key, int dvalue);
double              sc_keyvalue_get_double (sc_keyvalue_t * args,
                                            const char *key, double dvalue);
const char         *sc_keyvalue_get_string (sc_keyvalue_t * args,
                                            const char *key,
                                            const char *dvalue);
void               *sc_keyvalue_get_pointer (sc_keyvalue_t * args,
                                             const char *key, void *dvalue);

/* Routines to set values for a given key */
void                sc_keyvalue_set_int (sc_keyvalue_t * args,
                                         const char *key, int newvalue);
void                sc_keyvalue_set_double (sc_keyvalue_t * args,
                                            const char *key, double newvalue);
void                sc_keyvalue_set_string (sc_keyvalue_t * args,
                                            const char *key,
                                            const char *newvalue);
void                sc_keyvalue_set_pointer (sc_keyvalue_t * args,
                                             const char *key,
                                             const char *newvalue);

SC_EXTERN_C_END;

#endif /* !SC_KEYVALUE_H */
