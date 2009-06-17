
#include <sc_object.h>

typedef struct sc_object_method
{
  sc_void_function_t  ifm;      /* interface method */
  void               *o;        /* object instance */
  sc_void_function_t  oinmi;    /* object instance method implementation */
}
sc_object_method_t;

/* construct the hash key for an interface method and an object instance */
static unsigned
sc_object_method_hash (const void *v, const void *u)
{
  const sc_object_method_t *om = (sc_object_method_t *) v;
  uint32_t            a, b, c;

  /* this is very crude and probably not safe on all architectures */
  a = (uint32_t) (((long) om->ifm) & 0xffffffff);
  b = (uint32_t) (((long) om->o) & 0xffffffff);
  c = 0;

  sc_hash_mix (a, b, c);

  return (unsigned) c;
}

static              bool
sc_object_method_equal (const void *v1, const void *v2, const void *u)
{
  const sc_object_method_t *om1 = (sc_object_method_t *) v1;
  const sc_object_method_t *om2 = (sc_object_method_t *) v2;

  return om1->ifm == om2->ifm && om1->o == om2->o;
}

sc_object_system_t *
sc_object_system_new (void)
{
  sc_object_system_t *s;

  s = SC_ALLOC (sc_object_system_t, 1);
  s->methods = sc_hash_new (sc_object_method_hash,
                            sc_object_method_equal, NULL, NULL);
  s->mpool = sc_mempool_new (sizeof (sc_object_method_t));

  return s;
}

void
sc_object_system_destroy (sc_object_system_t * s)
{
  sc_hash_unlink_destroy (s->methods);
  sc_mempool_destroy (s->mpool);
  SC_FREE (s);
}

void
sc_object_method_register (sc_object_system_t * s, sc_void_function_t ifm,
                           void *o, sc_void_function_t oinmi)
{
  bool                found;
  sc_object_method_t *om;

  om = sc_mempool_alloc (s->mpool);
  om->ifm = ifm;
  om->o = o;
  om->oinmi = oinmi;

  found = sc_hash_insert_unique (s->methods, om, NULL);
  SC_ASSERT (!found);
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
  SC_ASSERT (found);

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
  SC_ASSERT (found);

  return ((sc_object_method_t *) * om)->oinmi;
}

void
sc_object_destroy (sc_object_system_t * s, sc_object_t * o)
{
  sc_void_function_t  oinmi;

  /* get and remove the implementation of this method for this object */
  oinmi =
    sc_object_method_unregister (s, (sc_void_function_t) sc_object_destroy,
                                 (void *) o);

  /* cast object instance method implementation appropriately and call it */
  ((void (*)(sc_object_t *)) oinmi) (o);
}

void
sc_object_print (sc_object_system_t * s, sc_object_t * o, FILE * f)
{
  sc_void_function_t  oinmi;

  /* get the implementation of this method for this object */
  oinmi = sc_object_method_lookup (s, (sc_void_function_t) sc_object_print,
                                   (void *) o);

  /* cast object instance method implementation appropriately and call it */
  ((void (*)(sc_object_t *, FILE *)) oinmi) (o, f);
}
