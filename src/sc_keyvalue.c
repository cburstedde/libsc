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

#include <sc_keyvalue.h>

typedef struct sc_keyvalue_entry
{
  const char         *key;
  sc_keyvalue_entry_type_t type;
  union
  {
    int                 i;
    double              g;
    const char         *s;
    void               *p;
  }
  value;
}
sc_keyvalue_entry_t;

static unsigned
sc_keyvalue_entry_hash (const void *v, const void *u)
{
  const sc_keyvalue_entry_t *ov = (const sc_keyvalue_entry_t *) v;

  return sc_hash_function_string (ov->key, NULL);
}

static int
sc_keyvalue_entry_equal (const void *v1, const void *v2, const void *u)
{

  const sc_keyvalue_entry_t *ov1 = (const sc_keyvalue_entry_t *) v1;
  const sc_keyvalue_entry_t *ov2 = (const sc_keyvalue_entry_t *) v2;

  return !strcmp (ov1->key, ov2->key);
}

sc_keyvalue_t      *
sc_keyvalue_newv (va_list ap)
{
  const char         *s;
  int                 added;
  void              **found;
  sc_keyvalue_t      *kv;
  sc_keyvalue_entry_t *value;

  /* Create the initial empty keyvalue object */
  kv = sc_keyvalue_new ();

  /* loop through the variable arguments to fill keyvalue */
  for (;;) {
    s = va_arg (ap, const char *);
    if (s == NULL) {
      break;
    }
    /* if this assertion blows then the type prefix might be missing */
    SC_ASSERT (s[0] != '\0' && s[1] == ':' && s[2] != '\0');
    value = (sc_keyvalue_entry_t *) sc_mempool_alloc (kv->value_allocator);
    value->key = &s[2];
    switch (s[0]) {
    case 'i':
      value->type = SC_KEYVALUE_ENTRY_INT;
      value->value.i = va_arg (ap, int);
      break;
    case 'g':
      value->type = SC_KEYVALUE_ENTRY_DOUBLE;
      value->value.g = va_arg (ap, double);
      break;
    case 's':
      value->type = SC_KEYVALUE_ENTRY_STRING;
      value->value.s = va_arg (ap, const char *);
      break;
    case 'p':
      value->type = SC_KEYVALUE_ENTRY_POINTER;
      value->value.p = va_arg (ap, void *);
      break;
    default:
      SC_ABORTF ("invalid argument character %c", s[0]);
    }
    added = sc_hash_insert_unique (kv->hash, value, &found);
    if (!added) {
      sc_mempool_free (kv->value_allocator, *found);
      *found = value;
    }
  }

  return kv;
}

sc_keyvalue_t      *
sc_keyvalue_newf (int dummy, ...)
{
  va_list             ap;
  sc_keyvalue_t      *kv;

  SC_ASSERT (dummy == 0);

  va_start (ap, dummy);
  kv = sc_keyvalue_newv (ap);
  va_end (ap);

  return kv;
}

sc_keyvalue_t      *
sc_keyvalue_new ()
{
  sc_keyvalue_t      *kv;

  kv = SC_ALLOC (sc_keyvalue_t, 1);
  kv->hash = sc_hash_new (sc_keyvalue_entry_hash, sc_keyvalue_entry_equal,
                          NULL, NULL);
  kv->value_allocator = sc_mempool_new (sizeof (sc_keyvalue_entry_t));

  return kv;
}

void
sc_keyvalue_destroy (sc_keyvalue_t * kv)
{
  sc_hash_destroy (kv->hash);
  sc_mempool_destroy (kv->value_allocator);

  SC_FREE (kv);
}

sc_keyvalue_entry_type_t
sc_keyvalue_exists (sc_keyvalue_t * kv, const char *key)
{
  void              **found;
  sc_keyvalue_entry_t svalue, *pvalue = &svalue;
  sc_keyvalue_entry_t *value;

  SC_ASSERT (kv != NULL);
  SC_ASSERT (key != NULL);

  pvalue->key = key;
  pvalue->type = SC_KEYVALUE_ENTRY_NONE;
  if (sc_hash_lookup (kv->hash, pvalue, &found)) {
    value = (sc_keyvalue_entry_t *) (*found);
    return value->type;
  }
  else
    return SC_KEYVALUE_ENTRY_NONE;
}

sc_keyvalue_entry_type_t
sc_keyvalue_unset (sc_keyvalue_t * kv, const char *key)
{
  void               *found;
  sc_keyvalue_entry_t svalue, *pvalue = &svalue;
  sc_keyvalue_entry_t *value;

  int                 remove_test;
  sc_keyvalue_entry_type_t type;

  SC_ASSERT (kv != NULL);
  SC_ASSERT (key != NULL);

  pvalue->key = key;
  pvalue->type = SC_KEYVALUE_ENTRY_NONE;

  /* Remove this entry */
  remove_test = sc_hash_remove (kv->hash, pvalue, &found);

  /* Check whether anything was removed */
  if (!remove_test)
    return SC_KEYVALUE_ENTRY_NONE;

  /* Code reaching this point must have found something */
  SC_ASSERT (remove_test);
  SC_ASSERT (found != NULL);

  value = (sc_keyvalue_entry_t *) found;
  type = value->type;

  /* destroy the orignial hash entry */
  sc_mempool_free (kv->value_allocator, value);

  return type;
}

int
sc_keyvalue_get_int (sc_keyvalue_t * kv, const char *key, int dvalue)
{
  void              **found;
  sc_keyvalue_entry_t svalue, *pvalue = &svalue;
  sc_keyvalue_entry_t *value;

  SC_ASSERT (kv != NULL);
  SC_ASSERT (key != NULL);

  pvalue->key = key;
  pvalue->type = SC_KEYVALUE_ENTRY_NONE;
  if (sc_hash_lookup (kv->hash, pvalue, &found)) {
    value = (sc_keyvalue_entry_t *) (*found);
    SC_ASSERT (value->type == SC_KEYVALUE_ENTRY_INT);
    return value->value.i;
  }
  else
    return dvalue;
}

double
sc_keyvalue_get_double (sc_keyvalue_t * kv, const char *key, double dvalue)
{
  void              **found;
  sc_keyvalue_entry_t svalue, *pvalue = &svalue;
  sc_keyvalue_entry_t *value;

  SC_ASSERT (kv != NULL);
  SC_ASSERT (key != NULL);

  pvalue->key = key;
  pvalue->type = SC_KEYVALUE_ENTRY_NONE;
  if (sc_hash_lookup (kv->hash, pvalue, &found)) {
    value = (sc_keyvalue_entry_t *) (*found);
    SC_ASSERT (value->type == SC_KEYVALUE_ENTRY_DOUBLE);
    return value->value.g;
  }
  else
    return dvalue;
}

const char         *
sc_keyvalue_get_string (sc_keyvalue_t * kv, const char *key,
                        const char *dvalue)
{
  void              **found;
  sc_keyvalue_entry_t svalue, *pvalue = &svalue;
  sc_keyvalue_entry_t *value;

  SC_ASSERT (kv != NULL);
  SC_ASSERT (key != NULL);

  pvalue->key = key;
  pvalue->type = SC_KEYVALUE_ENTRY_NONE;
  if (sc_hash_lookup (kv->hash, pvalue, &found)) {
    value = (sc_keyvalue_entry_t *) (*found);
    SC_ASSERT (value->type == SC_KEYVALUE_ENTRY_STRING);
    return value->value.s;
  }
  else
    return dvalue;
}

void               *
sc_keyvalue_get_pointer (sc_keyvalue_t * kv, const char *key, void *dvalue)
{
  void              **found;
  sc_keyvalue_entry_t svalue, *pvalue = &svalue;
  sc_keyvalue_entry_t *value;

  SC_ASSERT (kv != NULL);
  SC_ASSERT (key != NULL);

  pvalue->key = key;
  pvalue->type = SC_KEYVALUE_ENTRY_NONE;
  if (sc_hash_lookup (kv->hash, pvalue, &found)) {
    value = (sc_keyvalue_entry_t *) (*found);
    SC_ASSERT (value->type == SC_KEYVALUE_ENTRY_POINTER);
    return value->value.p;
  }
  else
    return dvalue;
}

void
sc_keyvalue_set_int (sc_keyvalue_t * kv, const char *key, int newvalue)
{
  void              **found;
  int                 added;
  sc_keyvalue_entry_t svalue, *pvalue = &svalue;
  sc_keyvalue_entry_t *value;

  SC_ASSERT (kv != NULL);
  SC_ASSERT (key != NULL);

  pvalue->key = key;
  pvalue->type = SC_KEYVALUE_ENTRY_NONE;
  if (sc_hash_lookup (kv->hash, pvalue, &found)) {
    /* Key already exists in hash table */
    value = (sc_keyvalue_entry_t *) (*found);
    SC_ASSERT (value->type == SC_KEYVALUE_ENTRY_INT);

    value->value.i = newvalue;
  }
  else {
    /* Key does not exist and must be created */
    value = (sc_keyvalue_entry_t *) sc_mempool_alloc (kv->value_allocator);
    value->key = key;
    value->type = SC_KEYVALUE_ENTRY_INT;
    value->value.i = newvalue;

    /* Insert value into the hash table */
    added = sc_hash_insert_unique (kv->hash, value, &found);
    SC_ASSERT (added);
  }
}

void
sc_keyvalue_set_double (sc_keyvalue_t * kv, const char *key, double newvalue)
{
  void              **found;
  int                 added;
  sc_keyvalue_entry_t svalue, *pvalue = &svalue;
  sc_keyvalue_entry_t *value;

  SC_ASSERT (kv != NULL);
  SC_ASSERT (key != NULL);

  pvalue->key = key;
  pvalue->type = SC_KEYVALUE_ENTRY_NONE;
  if (sc_hash_lookup (kv->hash, pvalue, &found)) {
    /* Key already exists in hash table */
    value = (sc_keyvalue_entry_t *) (*found);
    SC_ASSERT (value->type == SC_KEYVALUE_ENTRY_DOUBLE);

    value->value.g = newvalue;
  }
  else {
    /* Key does not exist and must be created */
    value = (sc_keyvalue_entry_t *) sc_mempool_alloc (kv->value_allocator);
    value->key = key;
    value->type = SC_KEYVALUE_ENTRY_DOUBLE;
    value->value.g = newvalue;

    /* Insert value into the hash table */
    added = sc_hash_insert_unique (kv->hash, value, &found);
    SC_ASSERT (added);
  }
}

void
sc_keyvalue_set_string (sc_keyvalue_t * kv, const char *key,
                        const char *newvalue)
{
  void              **found;
  int                 added;
  sc_keyvalue_entry_t svalue, *pvalue = &svalue;
  sc_keyvalue_entry_t *value;

  SC_ASSERT (kv != NULL);
  SC_ASSERT (key != NULL);

  pvalue->key = key;
  pvalue->type = SC_KEYVALUE_ENTRY_NONE;
  if (sc_hash_lookup (kv->hash, pvalue, &found)) {
    /* Key already exists in hash table */
    value = (sc_keyvalue_entry_t *) (*found);
    SC_ASSERT (value->type == SC_KEYVALUE_ENTRY_STRING);

    value->value.s = newvalue;
  }
  else {
    /* Key does not exist and must be created */
    value = (sc_keyvalue_entry_t *) sc_mempool_alloc (kv->value_allocator);
    value->key = key;
    value->type = SC_KEYVALUE_ENTRY_STRING;
    value->value.s = newvalue;

    /* Insert value into the hash table */
    added = sc_hash_insert_unique (kv->hash, value, &found);
    SC_ASSERT (added);
  }
}

void
sc_keyvalue_set_pointer (sc_keyvalue_t * kv, const char *key, void *newvalue)
{
  void              **found;
  int                 added;
  sc_keyvalue_entry_t svalue, *pvalue = &svalue;
  sc_keyvalue_entry_t *value;

  SC_ASSERT (kv != NULL);
  SC_ASSERT (key != NULL);

  pvalue->key = key;
  pvalue->type = SC_KEYVALUE_ENTRY_NONE;
  if (sc_hash_lookup (kv->hash, pvalue, &found)) {
    /* Key already exists in hash table */
    value = (sc_keyvalue_entry_t *) (*found);
    SC_ASSERT (value->type == SC_KEYVALUE_ENTRY_POINTER);

    value->value.p = (void *) newvalue;
  }
  else {
    /* Key does not exist and must be created */
    value = (sc_keyvalue_entry_t *) sc_mempool_alloc (kv->value_allocator);
    value->key = key;
    value->type = SC_KEYVALUE_ENTRY_POINTER;
    value->value.p = (void *) newvalue;

    /* Insert value into the hash table */
    added = sc_hash_insert_unique (kv->hash, value, &found);
    SC_ASSERT (added);
  }
}

typedef struct sc_kv_hash_data
{
  sc_keyvalue_foreach_t fn;
  void               *data;
}
sc_kv_hash_data_t;

static int
sc_kv_hash_fn (void **v, const void *u)
{
  const sc_kv_hash_data_t *hdata = (const sc_kv_hash_data_t *) u;
  sc_keyvalue_entry_t *hentry = (sc_keyvalue_entry_t *) * v;
  const char         *key = hentry->key;
  sc_keyvalue_entry_type_t type = hentry->type;
  void               *entry = &(hentry->value.p);

  return hdata->fn (key, type, entry, hdata->data);
}

void
sc_keyvalue_foreach (sc_keyvalue_t * kv, sc_keyvalue_foreach_t fn,
                     void *user_data)
{
  sc_kv_hash_data_t   hdata;

  hdata.fn = fn;
  hdata.data = user_data;

  SC_ASSERT (kv->hash->user_data == NULL);
  kv->hash->user_data = &hdata;

  sc_hash_foreach (kv->hash, sc_kv_hash_fn);

  kv->hash->user_data = NULL;
}
