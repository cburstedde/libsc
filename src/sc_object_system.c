
#include <sc_object_system.h>

/* construct the hash key for an interface method and an object instance */
static unsigned
sc_object_hash (const void *v, const void *u)
{
  const unsigned long m = ((1UL << 32) - 1);
  unsigned long       o = (unsigned long) v;
  uint32_t            a, b, c;

  /* this is very crude and probably not safe on all architectures */
  a = (uint32_t) (o & m);
  b = (uint32_t) ((o >> 32) & m);
  c = 0;

  sc_hash_mix (a, b, c);

  return (unsigned) c;
}

static              bool
sc_object_equal (const void *v1, const void *v2, const void *u)
{
  return v1 == v2;
}

sc_object_system_t *
sc_object_system_new (void)
{
  sc_object_system_t *s;

  s = SC_ALLOC (sc_object_system_t, 1);
  s->entries = sc_hash_new (sc_object_hash, sc_object_equal, NULL, NULL);
  s->mpool = sc_mempool_new (sizeof (sc_object_entry_t));

  return s;
}

void
sc_object_system_destroy (sc_object_system_t * s)
{
  sc_hash_unlink_destroy (s->entries);
  sc_mempool_destroy (s->mpool);

  SC_FREE (s);
}

void
sc_object_system_entry_new (sc_object_system_t * s, void * o)
{
  
}

void
sc_object_method_register (sc_object_system_t * s, sc_void_function_t ifm,
                           void *o, sc_void_function_t oinmi)
{
  bool                added;
  sc_object_method_t *om;

  om = sc_mempool_alloc (s->mpool);
  om->ifm = ifm;
  om->o = o;
  om->oinmi = oinmi;

  added = sc_hash_insert_unique (s->methods, om, NULL);
  SC_CHECK_ABORT (added, "duplicate method registration attempt");
}

sc_void_function_t
sc_object_method_unregister (sc_object_system_t * s, sc_void_function_t ifm,
                             void *o)
{
  bool                found;
  void               *om;
  sc_object_method_t  omk;
  sc_void_function_t  oinmi;

  omk.ifm = ifm;
  omk.o = o;
  omk.oinmi = NULL;

  found = sc_hash_remove (s->methods, &omk, &om);
  SC_CHECK_ABORT (found, "nonexistent method unregister attempt");

  oinmi = ((sc_object_method_t *) om)->oinmi;
  sc_mempool_free (s->mpool, om);

  return oinmi;
}

sc_void_function_t
sc_object_method_lookup (sc_object_system_t * s, sc_void_function_t ifm,
                         void *o)
{
  bool                found;
  void              **om;
  sc_object_method_t  omk;

  omk.ifm = ifm;
  omk.o = o;
  omk.oinmi = NULL;

  found = sc_hash_lookup (s->methods, &omk, &om);

  return found ? (((sc_object_method_t *) *om)->oinmi) : NULL;
}

void
sc_object_method_override (sc_object_system_t * s, sc_void_function_t ifm,
                           void *o, sc_void_function_t oinmi)
{
  bool                found;
  void              **om;
  sc_object_method_t  omk;

  omk.ifm = ifm;
  omk.o = o;
  omk.oinmi = NULL;

  found = sc_hash_lookup (s->methods, &omk, &om);
  SC_CHECK_ABORT (found, "nonexistent method override attempt");

  /* *INDENT-OFF* HORRIBLE indent bug */
  ((sc_object_method_t *) *om)->oinmi = oinmi;
  /* *INDENT-ON* */
}
