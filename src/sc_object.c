
#include <sc_object.h>

const char         *sc_object_type = "sc_object";

static const char *
type_fn (sc_object_t * o)
{
  SC_LDEBUG ("sc_object_t type\n");

  return sc_object_type;
}

static void
finalize_fn (sc_object_t * o)
{
  SC_LDEBUG ("sc_object_t finalize\n");

  sc_object_delegate_popall (o);

  SC_FREE (o);
}

static void
write_fn (sc_object_t * o, FILE *out)
{
  fprintf (out, "sc_object_t write with %d refs\n", o->num_refs);
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
sc_object_delegate_popall (sc_object_t * o)
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

sc_void_function_t
sc_object_delegate_lookup (sc_object_t * o, sc_void_function_t ifm)
{
  sc_object_system_t *s = o->s;
  sc_object_t        *d;
  void               *v;
  sc_void_function_t  oinmi;
  size_t              zz;

  oinmi = sc_object_method_lookup (s, ifm, o);
  if (oinmi != NULL) {
    return oinmi;
  }

  for (zz = o->delegates.elem_count; zz > 0; --zz) {
    v = sc_array_index (&o->delegates, zz - 1);
    d = *((sc_object_t **) v);
    oinmi = sc_object_delegate_lookup (d, ifm);
    if (oinmi != NULL) {
      return oinmi;
    }
  }

  return NULL;
}

bool
sc_object_is_type (sc_object_t * o, const char * type)
{
  return true;
}

void
sc_object_register_methods (sc_object_t * o)
{
  sc_object_system_t *s = o->s;

  ++o->num_regs;
  sc_object_method_register (s, (sc_void_function_t) sc_object_finalize,
                             o, (sc_void_function_t) finalize_fn);

  ++o->num_regs;
  sc_object_method_register (s, (sc_void_function_t) sc_object_write,
                             o, (sc_void_function_t) write_fn);

  ++o->num_regs;
  sc_object_method_register (s, (sc_void_function_t) sc_object_type,
                             o, (sc_void_function_t) type_fn);
}

sc_object_t        *
sc_object_alloc (sc_object_system_t * s)
{
  sc_object_t        *o;

  SC_ASSERT (s != NULL);

  o = SC_ALLOC (sc_object_t, 1);
  o->s = s;
  o->num_refs = 1;
  o->num_regs = 0;
  sc_array_init (&o->delegates, sizeof (sc_object_t *));

  return o;
}

sc_object_t        *
sc_object_klass_new (sc_object_system_t * s)
{
  sc_object_t        *o;

  SC_ASSERT (s != NULL);

  o = sc_object_alloc (s);
  sc_object_register_methods (o);
  sc_object_initialize (o);

  return o;
}

sc_object_t        *
sc_object_new_from_klass (sc_object_t * d)
{
  sc_object_t        *o;

  SC_ASSERT (d != NULL);

  o = sc_object_alloc (d->s);
  sc_object_delegate_push (o, d);
  sc_object_initialize (o);

  return o;
}

const char         *
sc_object_get_type (sc_object_t * o)
{
  sc_void_function_t  oinmi;

  oinmi =
    sc_object_delegate_lookup (o, (sc_void_function_t) sc_object_get_type);

  if (oinmi != NULL) {
    return ((const char * (*)(sc_object_t *)) oinmi) (o);
  }

  SC_CHECK_NOT_REACHED ();
}

void
sc_object_initialize (sc_object_t * o)
{
  sc_void_function_t  oinmi;

  oinmi =
    sc_object_delegate_lookup (o, (sc_void_function_t) sc_object_initialize);

  if (oinmi != NULL) {
    ((void (*)(sc_object_t *)) oinmi) (o);
  }
}

void
sc_object_finalize (sc_object_t * o)
{
  sc_void_function_t  oinmi;

  oinmi =
    sc_object_delegate_lookup (o, (sc_void_function_t) sc_object_finalize);

  if (oinmi != NULL) {
    ((void (*)(sc_object_t *)) oinmi) (o);
  }
}

void
sc_object_write (sc_object_t * o, FILE * out)
{
  sc_void_function_t  oinmi;

  oinmi = sc_object_delegate_lookup (o, (sc_void_function_t) sc_object_write);

  if (oinmi != NULL) {
    ((void (*)(sc_object_t *, FILE *)) oinmi) (o, out);
  }
}
