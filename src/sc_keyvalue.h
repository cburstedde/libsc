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

typedef struct sc_object_arguments
{
  sc_hash_t          *hash;
  sc_mempool_t       *value_allocator;
}
sc_object_arguments_t;

/* passing arguments */
/* Arguments come in pairs of 2: static string "type:key" and value;
   type is a letter like the identifier names in sc_object_value.value */
sc_object_arguments_t *sc_object_arguments_new (int dummy, ...);
void                sc_object_arguments_destroy (sc_object_arguments_t *
                                                 args);
sc_object_value_type_t sc_object_arguments_exist (sc_object_arguments_t *
                                                  args, const char *key);
/* if the key is not present then dvalue is returned */
int                 sc_object_arguments_int (sc_object_arguments_t * args,
                                             const char *key, int dvalue);
double              sc_object_arguments_double (sc_object_arguments_t * args,
                                                const char *key,
                                                double dvalue);
const char         *sc_object_arguments_string (sc_object_arguments_t * args,
                                                const char *key,
                                                const char *dvalue);
void               *sc_object_arguments_pointer (sc_object_arguments_t * args,
                                                 const char *key,
                                                 void *dvalue);

/* Helper function for sc_object_arguments_new */
sc_object_arguments_t *sc_object_arguments_new_va (va_list ap);

SC_EXTERN_C_END;

#endif /* !SC_KEYVALUE_H */
