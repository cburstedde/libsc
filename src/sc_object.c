
#include <sc_object.h>

const char         *sc_object_type = "sc_object";

static unsigned
sc_object_hash (const void *v, const void *u)
{
  const unsigned long m = ((1UL << 32) - 1);
  unsigned long       l = (unsigned long) v;
  uint32_t            a, b, c;

  a = (uint32_t) (l & m);
  b = (uint32_t) ((l >> 32) & m);
  c = 0;

  sc_hash_mix (a, b, c);

  return (unsigned) c;
}

static              bool
sc_object_equal (const void *v1, const void *v2, const void *u)
{
  return v1 == v2;
}

static unsigned
sc_object_entry_hash (const void *v, const void *u)
{
  const sc_object_entry_t *e = v;
  const unsigned long m = ((1UL << 32) - 1);
  unsigned long       l = (unsigned long) e->key;
  uint32_t            a, b, c;

  a = (uint32_t) (l & m);
  b = (uint32_t) ((l >> 32) & m);
  c = 0;

  sc_hash_mix (a, b, c);

  return (unsigned) c;
}

static              bool
sc_object_entry_equal (const void *v1, const void *v2, const void *u)
{
  const sc_object_entry_t *e1 = v1;
  const sc_object_entry_t *e2 = v2;

  return e1->key == e2->key;
}

static              bool
sc_object_entry_free (void **v, const void *u)
{
  /* *INDENT-OFF* HORRIBLE indent bug */
  sc_object_entry_t  *e = (sc_object_entry_t *) * v;
  /* *INDENT-ON* */

  SC_ASSERT (e->oinmi == NULL || e->odata == NULL);

  SC_FREE (e->odata);           /* legal to be NULL */
  SC_FREE (e);

  return true;
}

static const char  *
get_type_fn (sc_object_t * o)
{
  SC_LDEBUG ("sc_object_t get_type\n");

  return sc_object_type;
}

static const char  *
get_type_call (sc_object_method_t oinmi, sc_object_t * o)
{
  return ((const char *(*)(sc_object_t *)) oinmi) (o);
}

static void
finalize_fn (sc_object_t * o)
{
  SC_ASSERT (sc_object_is_type (o, sc_object_type));

  SC_LDEBUG ("sc_object_t finalize\n");

  sc_object_delegate_pop_all (o);

  if (o->table != NULL) {
    sc_hash_foreach (o->table, sc_object_entry_free);
    sc_hash_destroy (o->table);
    o->table = NULL;
  }

  SC_FREE (o);
}

static void
write_fn (sc_object_t * o, FILE * out)
{
  SC_ASSERT (sc_object_is_type (o, sc_object_type));

  fprintf (out, "sc_object_t write with %d refs\n", o->num_refs);
}

bool
sc_object_recursion (sc_object_t * o, sc_object_recursion_context_t * rc)
{
  bool                toplevel, added, answered;
  bool                found_self, found_delegate;
  size_t              zz;
  sc_object_method_t  oinmi;
  sc_object_recursion_match_t *match;
  sc_object_t        *d;
  void               *v;

  SC_ASSERT (rc->lookup != NULL);
  SC_ASSERT (rc->found == NULL ||
             rc->found->elem_size == sizeof (sc_object_recursion_match_t));

  if (rc->visited == NULL) {
    toplevel = true;
    rc->visited = sc_hash_new (sc_object_hash, sc_object_equal, NULL, NULL);
  }
  else {
    toplevel = false;
  }

  answered = false;
  found_self = found_delegate = false;
  added = sc_hash_insert_unique (rc->visited, o, NULL);

  if (added) {
    oinmi = sc_object_method_lookup (o, rc->lookup);
    if (oinmi != NULL) {
      if (rc->found != NULL) {
        match = sc_array_push (rc->found);
        match->oinmi = oinmi;
        found_self = true;
      }
      if (rc->callfn != NULL) {
        answered = rc->callfn (o, oinmi, rc->user_data);
      }
    }

    if (!answered && !(found_self && rc->accept_self)) {
      for (zz = o->delegates.elem_count; zz > 0; --zz) {
        v = sc_array_index (&o->delegates, zz - 1);
        d = *((sc_object_t **) v);
        answered = sc_object_recursion (d, rc);
        if (answered) {
          found_delegate = true;
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

bool
sc_object_method_register (sc_object_t * o,
                           sc_object_method_t ifm, sc_object_method_t oinmi)
{
  bool                added, first;
  void              **lookup;
  sc_object_entry_t   se, *e = &se;

  if (o->table == NULL) {
    o->table =
      sc_hash_new (sc_object_entry_hash, sc_object_entry_equal, NULL, NULL);
    first = true;
  }
  else {
    first = false;
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
  bool                found;
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
  bool                found;
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

typedef struct sc_object_delegate_lookup_data
{
  sc_object_method_t  oinmi;
}
sc_object_delegate_lookup_data_t;

static              bool
delegate_lookup_fn (sc_object_t * o,
                    sc_object_method_t oinmi, void *user_data)
{
  ((sc_object_delegate_lookup_data_t *) user_data)->oinmi = oinmi;

  return true;
}

sc_object_method_t
sc_object_delegate_lookup (sc_object_t * o, sc_object_method_t ifm)
{
  sc_object_recursion_context_t src, *rc = &src;
  sc_object_delegate_lookup_data_t sdld, *dld = &sdld;

  dld->oinmi = NULL;

  rc->visited = NULL;
  rc->lookup = ifm;
  rc->found = NULL;
  rc->accept_self = false;
  rc->accept_delegate = false;
  rc->callfn = delegate_lookup_fn;
  rc->user_data = dld;

  (void) sc_object_recursion (o, rc);

  return dld->oinmi;
}

typedef struct sc_object_is_type_data
{
  const char         *type;
}
sc_object_is_type_data_t;

static              bool
is_type_fn (sc_object_t * o, sc_object_method_t oinmi, void *user_data)
{
  sc_object_is_type_data_t *itd = user_data;

  return !strcmp (get_type_call (oinmi, o), itd->type);
}

bool
sc_object_is_type (sc_object_t * o, const char *type)
{
  sc_object_recursion_context_t src, *rc = &src;
  sc_object_is_type_data_t sitd, *itd = &sitd;

  itd->type = type;

  rc->visited = NULL;
  rc->lookup = (sc_object_method_t) sc_object_get_type;
  rc->found = NULL;
  rc->accept_self = false;
  rc->accept_delegate = false;
  rc->callfn = is_type_fn;
  rc->user_data = itd;

  return sc_object_recursion (o, rc);
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
  bool                a1, a2, a3;
  sc_object_t        *o;

  o = sc_object_alloc ();

  a1 = sc_object_method_register (o, (sc_object_method_t) sc_object_get_type,
                                  (sc_object_method_t) get_type_fn);
  a2 = sc_object_method_register (o, (sc_object_method_t) sc_object_finalize,
                                  (sc_object_method_t) finalize_fn);
  a3 = sc_object_method_register (o, (sc_object_method_t) sc_object_write,
                                  (sc_object_method_t) write_fn);
  SC_ASSERT (a1 && a2 && a3);

  sc_object_initialize (o);

  return o;
}

sc_object_t        *
sc_object_new_from_klass (sc_object_t * d)
{
  sc_object_t        *o;

  SC_ASSERT (d != NULL);

  o = sc_object_alloc ();
  sc_object_delegate_push (o, d);
  sc_object_initialize (o);

  return o;
}

void               *
sc_object_get_data (sc_object_t * o, sc_object_method_t ifm, size_t s)
{
  bool                added, first;
  void              **lookup;
  sc_object_entry_t   se, *e = &se;

  if (o->table == NULL) {
    o->table =
      sc_hash_new (sc_object_entry_hash, sc_object_entry_equal, NULL, NULL);
    first = true;
  }
  else {
    first = false;
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

const char         *
sc_object_get_type (sc_object_t * o)
{
  sc_object_method_t  oinmi;

  oinmi =
    sc_object_delegate_lookup (o, (sc_object_method_t) sc_object_get_type);

  if (oinmi != NULL) {
    return get_type_call (oinmi, o);
  }

  SC_ABORT_NOT_REACHED ();
}

void
sc_object_initialize (sc_object_t * o)
{
  sc_object_method_t  oinmi;

  oinmi =
    sc_object_delegate_lookup (o, (sc_object_method_t) sc_object_initialize);

  if (oinmi != NULL) {
    ((void (*)(sc_object_t *)) oinmi) (o);
  }
}

void
sc_object_finalize (sc_object_t * o)
{
  sc_object_method_t  oinmi;

  oinmi =
    sc_object_delegate_lookup (o, (sc_object_method_t) sc_object_finalize);

  if (oinmi != NULL) {
    ((void (*)(sc_object_t *)) oinmi) (o);
  }
}

void
sc_object_write (sc_object_t * o, FILE * out)
{
  sc_object_method_t  oinmi;

  oinmi = sc_object_delegate_lookup (o, (sc_object_method_t) sc_object_write);

  if (oinmi != NULL) {
    ((void (*)(sc_object_t *, FILE *)) oinmi) (o, out);
  }
}
