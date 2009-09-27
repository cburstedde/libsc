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

#include <sc_object.h>

const char         *sc_object_type = "sc_object";

static unsigned
sc_object_value_hash (const void *v, const void *u)
{
  const sc_object_value_t *ov = v;
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
  const sc_object_value_t *ov1 = v1;
  const sc_object_value_t *ov2 = v2;

  return !strcmp (ov1->key, ov2->key);
}

static unsigned
sc_object_entry_hash (const void *v, const void *u)
{
  uint32_t            a, b, c;
  const sc_object_entry_t *e = v;
  const unsigned long l = (unsigned long) e->key;
#if SC_SIZEOF_UNSIGNED_LONG > 4
  const unsigned long m = ((1UL << 32) - 1);

  a = (uint32_t) (l & m);
  b = (uint32_t) ((l >> 32) & m);
#else
  a = (uint32_t) l;
  b = 0xb0defe1dU;
#endif
  c = 0xdeadbeefU;
  sc_hash_final (a, b, c);

  return (unsigned) c;
}

static int
sc_object_entry_equal (const void *v1, const void *v2, const void *u)
{
  const sc_object_entry_t *e1 = v1;
  const sc_object_entry_t *e2 = v2;

  return e1->key == e2->key;
}

static unsigned
sc_object_hash (const void *v, const void *u)
{
  uint32_t            a, b, c;
  const unsigned long l = (unsigned long) v;
#if SC_SIZEOF_UNSIGNED_LONG > 4
  const unsigned long m = ((1UL << 32) - 1);

  a = (uint32_t) (l & m);
  b = (uint32_t) ((l >> 32) & m);
#else
  a = (uint32_t) l;
  b = 0xb0defe1dU;
#endif
  c = 0xdeadbeefU;
  sc_hash_final (a, b, c);

  return (unsigned) c;
}

static int
sc_object_equal (const void *v1, const void *v2, const void *u)
{
  return v1 == v2;
}

static int
sc_object_entry_free (void **v, const void *u)
{
  /* *INDENT-OFF* HORRIBLE indent bug */
  sc_object_entry_t  *e = (sc_object_entry_t *) * v;
  /* *INDENT-ON* */

  SC_ASSERT (e->oinmi == NULL || e->odata == NULL);

  SC_FREE (e->odata);           /* legal to be NULL */
  SC_FREE (e);

  return 1;
}

static int
is_type_fn (sc_object_t * o, const char *type)
{
  return !strcmp (type, sc_object_type);
}

static void
finalize_fn (sc_object_t * o)
{
  SC_ASSERT (sc_object_is_type (o, sc_object_type));

  sc_object_delegate_pop_all (o);

  if (o->table != NULL) {
    sc_hash_foreach (o->table, sc_object_entry_free);
    sc_hash_destroy (o->table);
    o->table = NULL;
  }

  SC_FREE (o);
}

static void
write_fn (sc_object_t * o, sc_object_t * m, FILE * out)
{
  SC_ASSERT (sc_object_is_type (o, sc_object_type));

  fprintf (out, "sc_object_t write with %d refs\n", o->num_refs);
}

void
sc_object_recursion_init (sc_object_recursion_context_t * rc,
                          sc_object_method_t ifm, sc_array_t * found)
{
  rc->visited = NULL;
  rc->lookup = ifm;

  if (found != NULL) {
    sc_array_init (found, sizeof (sc_object_recursion_match_t));
    rc->found = found;
  }
  else {
    rc->found = NULL;
  }

  rc->skip_top = 0;
  rc->accept_self = 0;
  rc->accept_delegate = 0;
  rc->callfn = NULL;
  rc->user_data = NULL;
  rc->last_match = NULL;
}

int
sc_object_recursion (sc_object_t * o, sc_object_recursion_context_t * rc)
{
  int                 toplevel, added, answered;
  int                 found_self, found_delegate;
  size_t              zz;
  sc_object_method_t  oinmi;
  sc_object_recursion_match_t *match;
  sc_object_t        *d;
  void               *v;

  SC_ASSERT (rc->lookup != NULL);
  SC_ASSERT (rc->found == NULL ||
             rc->found->elem_size == sizeof (sc_object_recursion_match_t));

  if (rc->visited == NULL) {
    toplevel = 1;
    rc->visited = sc_hash_new (sc_object_hash, sc_object_equal, NULL, NULL);
  }
  else {
    toplevel = 0;
  }

  answered = 0;
  found_self = found_delegate = 0;
  added = sc_hash_insert_unique (rc->visited, o, NULL);

  if (added) {
    if (!toplevel || !rc->skip_top) {
      oinmi = sc_object_method_lookup (o, rc->lookup);
      if (oinmi != NULL) {
        if (rc->found != NULL) {
          match = sc_array_push (rc->found);
          match->oinmi = oinmi;
          found_self = 1;
        }
        if (rc->callfn != NULL) {
          answered = rc->callfn (o, oinmi, rc->user_data);
        }
        rc->last_match = o;
      }
    }

    if (!answered && !(found_self && rc->accept_self)) {
      for (zz = o->delegates.elem_count; zz > 0; --zz) {
        v = sc_array_index (&o->delegates, zz - 1);
        d = *((sc_object_t **) v);
        answered = sc_object_recursion (d, rc);
        if (answered) {
          found_delegate = 1;
          if (rc->callfn != NULL || rc->accept_delegate) {
            break;
          }
        }
      }
    }
  }
  else {
    SC_LDEBUG ("Avoiding double recursion\n");
  }

  if (toplevel) {
    sc_hash_destroy (rc->visited);
    rc->visited = NULL;
  }

  return rc->callfn != NULL ? answered : found_self || found_delegate;
}

void
sc_object_ref (sc_object_t * o)
{
  SC_ASSERT (o->num_refs > 0);

  ++o->num_refs;
}

void
sc_object_unref (sc_object_t * o)
{
  SC_ASSERT (o->num_refs > 0);

  if (--o->num_refs == 0) {
    sc_object_finalize (o);
  }
}

sc_object_t        *
sc_object_dup (sc_object_t * o)
{
  sc_object_ref (o);

  return o;
}

int
sc_object_method_register (sc_object_t * o,
                           sc_object_method_t ifm, sc_object_method_t oinmi)
{
  int                 added, first;
  void              **lookup;
  sc_object_entry_t   se, *e = &se;

  if (o->table == NULL) {
    o->table =
      sc_hash_new (sc_object_entry_hash, sc_object_entry_equal, NULL, NULL);
    first = 1;
  }
  else {
    first = 0;
  }

  e->key = ifm;
  added = sc_hash_insert_unique (o->table, e, &lookup);

  if (!added) {
    SC_ASSERT (!first);

    /* replace existing method */
    /* *INDENT-OFF* HORRIBLE indent bug */
    e = (sc_object_entry_t *) *lookup;
    /* *INDENT-ON* */
    SC_ASSERT (e->key == ifm && e->odata == NULL);
  }
  else {
    e = SC_ALLOC (sc_object_entry_t, 1);
    e->key = ifm;
    e->odata = NULL;
    *lookup = e;
  }

  e->oinmi = oinmi;

  return added;
}

void
sc_object_method_unregister (sc_object_t * o, sc_object_method_t ifm)
{
  int                 found;
  void               *lookup;
  sc_object_entry_t   se, *e = &se;

  SC_ASSERT (o->table != NULL);

  e->key = ifm;
  found = sc_hash_remove (o->table, e, &lookup);
  SC_ASSERT (found);

  e = (sc_object_entry_t *) lookup;
  SC_ASSERT (e->oinmi != NULL && e->odata == NULL);

  SC_FREE (e);
}

sc_object_method_t
sc_object_method_lookup (sc_object_t * o, sc_object_method_t ifm)
{
  int                 found;
  void              **lookup;
  sc_object_entry_t   se, *e = &se;

  if (o->table == NULL) {
    return NULL;
  }

  e->key = ifm;
  found = sc_hash_lookup (o->table, e, &lookup);
  if (found) {
    /* *INDENT-OFF* HORRIBLE indent bug */
    e = (sc_object_entry_t *) *lookup;
    /* *INDENT-ON* */

    SC_ASSERT (e->key == ifm && e->oinmi != NULL && e->odata == NULL);

    return e->oinmi;
  }
  else {
    return NULL;
  }
}

void
sc_object_delegate_push (sc_object_t * o, sc_object_t * d)
{
  void               *v;

  sc_object_ref (d);
  v = sc_array_push (&o->delegates);
  *((sc_object_t **) v) = d;
}

void
sc_object_delegate_pop (sc_object_t * o)
{
  sc_object_t        *d;
  void               *v;

  v = sc_array_pop (&o->delegates);
  d = *((sc_object_t **) v);
  sc_object_unref (d);
}

void
sc_object_delegate_pop_all (sc_object_t * o)
{
  sc_object_t        *d;
  void               *v;
  size_t              zz;

  for (zz = o->delegates.elem_count; zz > 0; --zz) {
    v = sc_array_index (&o->delegates, zz - 1);
    d = *((sc_object_t **) v);
    sc_object_unref (d);
  }

  sc_array_reset (&o->delegates);
}

sc_object_t        *
sc_object_delegate_index (sc_object_t * o, int i)
{
  void               *v = sc_array_index_int (&o->delegates, i);

  return *((sc_object_t **) v);
}

typedef struct sc_object_delegate_lookup_data
{
  sc_object_method_t  oinmi;
}
sc_object_delegate_lookup_data_t;

static int
delegate_lookup_fn (sc_object_t * o,
                    sc_object_method_t oinmi, void *user_data)
{
  ((sc_object_delegate_lookup_data_t *) user_data)->oinmi = oinmi;

  return 1;
}

sc_object_method_t
sc_object_delegate_lookup (sc_object_t * o, sc_object_method_t ifm,
                           int skip_top, sc_object_t ** m)
{
  sc_object_recursion_context_t src, *rc = &src;
  sc_object_delegate_lookup_data_t sdld, *dld = &sdld;

  dld->oinmi = NULL;

  sc_object_recursion_init (rc, ifm, NULL);
  rc->skip_top = skip_top;
  rc->callfn = delegate_lookup_fn;
  rc->user_data = dld;

  if (sc_object_recursion (o, rc)) {
    SC_ASSERT (rc->last_match != NULL);
    if (m != NULL)
      *m = rc->last_match;
  }

  return dld->oinmi;
}

static sc_object_arguments_t *
sc_object_arguments_new_va (va_list ap)
{
  const char         *s;
  int                 added;
  void              **found;
  sc_object_arguments_t *args;
  sc_object_value_t  *value;

  args = SC_ALLOC (sc_object_arguments_t, 1);
  args->hash = sc_hash_new (sc_object_value_hash, sc_object_value_equal,
                            NULL, NULL);
  args->value_allocator = sc_mempool_new (sizeof (sc_object_value_t));

  for (;;) {
    s = va_arg (ap, const char *);
    if (s == NULL) {
      break;
    }
    SC_ASSERT (s[0] != '\0' && s[1] == ':' && s[2] != '\0');
    value = sc_mempool_alloc (args->value_allocator);
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

sc_object_arguments_t *
sc_object_arguments_new (int dummy, ...)
{
  va_list             ap;
  sc_object_arguments_t *args;

  SC_ASSERT (dummy == 0);

  va_start (ap, dummy);
  args = sc_object_arguments_new_va (ap);
  va_end (ap);

  return args;
}

void
sc_object_arguments_destroy (sc_object_arguments_t * args)
{
  sc_hash_destroy (args->hash);
  sc_mempool_destroy (args->value_allocator);

  SC_FREE (args);
}

sc_object_value_type_t
sc_object_arguments_exist (sc_object_arguments_t * args, const char *key)
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
sc_object_arguments_int (sc_object_arguments_t * args, const char *key,
                         int dvalue)
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
sc_object_arguments_double (sc_object_arguments_t * args, const char *key,
                            double dvalue)
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
sc_object_arguments_string (sc_object_arguments_t * args, const char *key,
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
sc_object_arguments_pointer (sc_object_arguments_t * args, const char *key,
                             void *dvalue)
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

sc_object_t        *
sc_object_alloc (void)
{
  sc_object_t        *o;

  o = SC_ALLOC (sc_object_t, 1);
  o->num_refs = 1;
  sc_array_init (&o->delegates, sizeof (sc_object_t *));
  o->table = NULL;

  return o;
}

sc_object_t        *
sc_object_klass_new (void)
{
  int                 a1, a2, a3;
  sc_object_t        *o;

  o = sc_object_alloc ();

  a1 = sc_object_method_register (o, (sc_object_method_t) sc_object_is_type,
                                  (sc_object_method_t) is_type_fn);
  a2 = sc_object_method_register (o, (sc_object_method_t) sc_object_finalize,
                                  (sc_object_method_t) finalize_fn);
  a3 = sc_object_method_register (o, (sc_object_method_t) sc_object_write,
                                  (sc_object_method_t) write_fn);
  SC_ASSERT (a1 && a2 && a3);

  sc_object_initialize (o, NULL);

  return o;
}

sc_object_t        *
sc_object_new_from_klass (sc_object_t * d, sc_object_arguments_t * args)
{
  sc_object_t        *o;

  SC_ASSERT (d != NULL);

  o = sc_object_alloc ();
  sc_object_delegate_push (o, d);
  sc_object_initialize (o, args);

  return o;
}

sc_object_t        *
sc_object_new_from_klass_values (sc_object_t * d, ...)
{
  va_list             ap;
  sc_object_t        *o;
  sc_object_arguments_t *args;

  va_start (ap, d);
  args = sc_object_arguments_new_va (ap);
  va_end (ap);

  o = sc_object_new_from_klass (d, args);
  sc_object_arguments_destroy (args);

  return o;
}

void               *
sc_object_get_data (sc_object_t * o, sc_object_method_t ifm, size_t s)
{
  int                 added, first;
  void              **lookup;
  sc_object_entry_t   se, *e = &se;

  if (o->table == NULL) {
    o->table =
      sc_hash_new (sc_object_entry_hash, sc_object_entry_equal, NULL, NULL);
    first = 1;
  }
  else {
    first = 0;
  }

  e->key = ifm;
  added = sc_hash_insert_unique (o->table, e, &lookup);

  if (!added) {
    SC_ASSERT (!first);

    /* get existing data */
    /* *INDENT-OFF* HORRIBLE indent bug */
    e = (sc_object_entry_t *) *lookup;
    /* *INDENT-ON* */
    SC_ASSERT (e->key == ifm && e->oinmi == NULL && e->odata != NULL);
  }
  else {
    e = SC_ALLOC (sc_object_entry_t, 1);
    e->key = ifm;
    e->oinmi = NULL;
    e->odata = sc_calloc (sc_package_id, 1, s);

    *lookup = e;
  }

  return e->odata;
}

typedef struct sc_object_is_type_data
{
  const char         *type;
}
sc_object_is_type_data_t;

static int
is_type_call_fn (sc_object_t * o, sc_object_method_t oinmi, void *user_data)
{
  sc_object_is_type_data_t *itd = user_data;

  return ((int (*)(sc_object_t *, const char *)) oinmi) (o, itd->type);
}

int
sc_object_is_type (sc_object_t * o, const char *type)
{
  sc_object_recursion_context_t src, *rc = &src;
  sc_object_is_type_data_t sitd, *itd = &sitd;

  itd->type = type;

  sc_object_recursion_init (rc, (sc_object_method_t) sc_object_is_type, NULL);
  rc->callfn = is_type_call_fn;
  rc->user_data = itd;

  return sc_object_recursion (o, rc);
}

sc_object_t        *
sc_object_copy (sc_object_t * o)
{
  size_t              zz;
  sc_array_t          sfound, *found = &sfound;
  sc_object_recursion_match_t *match;
  sc_object_recursion_context_t src, *rc = &src;
  sc_object_method_t  oinmi;
  sc_object_t        *c;

  SC_ASSERT (sc_object_is_type (o, sc_object_type));

  c = sc_object_alloc ();
  sc_object_delegate_push (c, o);

  sc_object_recursion_init (rc, (sc_object_method_t) sc_object_copy, found);

  /* post-order */
  if (sc_object_recursion (o, rc)) {
    for (zz = found->elem_count; zz > 0; --zz) {
      match = sc_array_index (found, zz - 1);
      oinmi = match->oinmi;
      SC_ASSERT (oinmi != NULL);

      ((void (*)(sc_object_t *, sc_object_t *)) oinmi) (o, c);
    }
  }

  sc_array_reset (found);

  return c;
}

void
sc_object_initialize (sc_object_t * o, sc_object_arguments_t * args)
{
  size_t              zz;
  sc_array_t          sfound, *found = &sfound;
  sc_object_recursion_match_t *match;
  sc_object_recursion_context_t src, *rc = &src;
  sc_object_method_t  oinmi;

  SC_ASSERT (sc_object_is_type (o, sc_object_type));

  sc_object_recursion_init (rc, (sc_object_method_t) sc_object_initialize,
                            found);

  /* post-order */
  if (sc_object_recursion (o, rc)) {
    for (zz = found->elem_count; zz > 0; --zz) {
      match = sc_array_index (found, zz - 1);
      oinmi = match->oinmi;
      SC_ASSERT (oinmi != NULL);

      ((void (*)(sc_object_t *, sc_object_arguments_t *)) oinmi) (o, args);
    }
  }

  sc_array_reset (found);
}

void
sc_object_finalize (sc_object_t * o)
{
  size_t              zz;
  sc_array_t          sfound, *found = &sfound;
  sc_object_recursion_match_t *match;
  sc_object_recursion_context_t src, *rc = &src;
  sc_object_method_t  oinmi;

  SC_ASSERT (sc_object_is_type (o, sc_object_type));

  sc_object_recursion_init (rc, (sc_object_method_t) sc_object_finalize,
                            found);

  /* pre-order */
  if (sc_object_recursion (o, rc)) {
    for (zz = 0; zz < found->elem_count; ++zz) {
      match = sc_array_index (found, zz);
      oinmi = match->oinmi;
      SC_ASSERT (oinmi != NULL);

      ((void (*)(sc_object_t *)) oinmi) (o);
    }
  }

  sc_array_reset (found);
}

void
sc_object_write (sc_object_t * o, FILE * out)
{
  sc_object_method_t  oinmi;
  sc_object_t        *m;

  SC_ASSERT (sc_object_is_type (o, sc_object_type));

  oinmi = sc_object_delegate_lookup (o, (sc_object_method_t) sc_object_write,
                                     0, &m);

  if (oinmi != NULL) {
    ((void (*)(sc_object_t *, sc_object_t *, FILE *)) oinmi) (o, m, out);
  }
}
