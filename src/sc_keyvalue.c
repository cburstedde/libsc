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

#include <sc_keyvalue.h>

static unsigned
sc_object_value_hash (const void *v, const void *u)
{
  const sc_object_value_t *ov = (const sc_object_value_t *) v;
  const char         *s;
  uint32_t            hash;

  hash = 0;
  for (s = ov->key; *s; ++s) {
    hash += *s;
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);

  return (unsigned) hash;
}

static int
sc_object_value_equal (const void *v1, const void *v2, const void *u)
{

  const sc_object_value_t *ov1 = (const sc_object_value_t *) v1;
  const sc_object_value_t *ov2 = (const sc_object_value_t *) v2;

  return !strcmp (ov1->key, ov2->key);
}

sc_keyvalue_t      *
sc_keyvalue_new_va (va_list ap)
{
  const char         *s;
  int                 added;
  void              **found;
  sc_keyvalue_t      *args;
  sc_object_value_t  *value;

  args = SC_ALLOC (sc_keyvalue_t, 1);
  args->hash = sc_hash_new (sc_object_value_hash, sc_object_value_equal,
                            NULL, NULL);
  args->value_allocator = sc_mempool_new (sizeof (sc_object_value_t));

  for (;;) {
    s = va_arg (ap, const char *);
    if (s == NULL) {
      break;
    }
    SC_ASSERT (s[0] != '\0' && s[1] == ':' && s[2] != '\0');
    value = (sc_object_value_t *) sc_mempool_alloc (args->value_allocator);
    value->key = &s[2];
    switch (s[0]) {
    case 'i':
      value->type = SC_OBJECT_VALUE_INT;
      value->value.i = va_arg (ap, int);
      break;
    case 'g':
      value->type = SC_OBJECT_VALUE_DOUBLE;
      value->value.g = va_arg (ap, double);
      break;
    case 's':
      value->type = SC_OBJECT_VALUE_STRING;
      value->value.s = va_arg (ap, const char *);
      break;
    case 'p':
      value->type = SC_OBJECT_VALUE_POINTER;
      value->value.p = va_arg (ap, void *);
      break;
    default:
      SC_ABORTF ("invalid argument character %c", s[0]);
    }
    added = sc_hash_insert_unique (args->hash, value, &found);
    if (!added) {
      sc_mempool_free (args->value_allocator, *found);
      *found = value;
    }
  }

  return args;
}

sc_keyvalue_t      *
sc_keyvalue_new (int dummy, ...)
{
  va_list             ap;
  sc_keyvalue_t      *args;

  SC_ASSERT (dummy == 0);

  va_start (ap, dummy);
  args = sc_keyvalue_new_va (ap);
  va_end (ap);

  return args;
}

void
sc_keyvalue_destroy (sc_keyvalue_t * args)
{
  sc_hash_destroy (args->hash);
  sc_mempool_destroy (args->value_allocator);

  SC_FREE (args);
}

sc_object_value_type_t
sc_keyvalue_exist (sc_keyvalue_t * args, const char *key)
{
  void              **found;
  sc_object_value_t   svalue, *pvalue = &svalue;
  sc_object_value_t  *value;

  SC_ASSERT (args != NULL);

  pvalue->key = key;
  pvalue->type = SC_OBJECT_VALUE_NONE;
  if (sc_hash_lookup (args->hash, pvalue, &found)) {
    value = (sc_object_value_t *) (*found);
    return value->type;
  }
  else
    return SC_OBJECT_VALUE_NONE;
}

int
sc_keyvalue_get_int (sc_keyvalue_t * args, const char *key, int dvalue)
{
  void              **found;
  sc_object_value_t   svalue, *pvalue = &svalue;
  sc_object_value_t  *value;

  SC_ASSERT (args != NULL);

  pvalue->key = key;
  pvalue->type = SC_OBJECT_VALUE_NONE;
  if (sc_hash_lookup (args->hash, pvalue, &found)) {
    value = (sc_object_value_t *) (*found);
    SC_ASSERT (value->type == SC_OBJECT_VALUE_INT);
    return value->value.i;
  }
  else
    return dvalue;
}

double
sc_keyvalue_get_double (sc_keyvalue_t * args, const char *key, double dvalue)
{
  void              **found;
  sc_object_value_t   svalue, *pvalue = &svalue;
  sc_object_value_t  *value;

  SC_ASSERT (args != NULL);

  pvalue->key = key;
  pvalue->type = SC_OBJECT_VALUE_NONE;
  if (sc_hash_lookup (args->hash, pvalue, &found)) {
    value = (sc_object_value_t *) (*found);
    SC_ASSERT (value->type == SC_OBJECT_VALUE_DOUBLE);
    return value->value.g;
  }
  else
    return dvalue;
}

const char         *
sc_keyvalue_get_string (sc_keyvalue_t * args, const char *key,
                        const char *dvalue)
{
  void              **found;
  sc_object_value_t   svalue, *pvalue = &svalue;
  sc_object_value_t  *value;

  SC_ASSERT (args != NULL);

  pvalue->key = key;
  pvalue->type = SC_OBJECT_VALUE_NONE;
  if (sc_hash_lookup (args->hash, pvalue, &found)) {
    value = (sc_object_value_t *) (*found);
    SC_ASSERT (value->type == SC_OBJECT_VALUE_STRING);
    return value->value.s;
  }
  else
    return dvalue;
}

void               *
sc_keyvalue_get_pointer (sc_keyvalue_t * args, const char *key, void *dvalue)
{
  void              **found;
  sc_object_value_t   svalue, *pvalue = &svalue;
  sc_object_value_t  *value;

  SC_ASSERT (args != NULL);

  pvalue->key = key;
  pvalue->type = SC_OBJECT_VALUE_NONE;
  if (sc_hash_lookup (args->hash, pvalue, &found)) {
    value = (sc_object_value_t *) (*found);
    SC_ASSERT (value->type == SC_OBJECT_VALUE_POINTER);
    return value->value.p;
  }
  else
    return dvalue;
}
