
#include <sc_object.h>

void
sc_object_destroy (sc_object_t * o)
{
  sc_object_system_t *s = o->s;
  sc_void_function_t  oinmi;

  /* get the implementation of this method for this object */
  oinmi = sc_object_method_lookup (s, (sc_void_function_t) sc_object_destroy,
                                   (void *) o);

  /* cast object instance method implementation appropriately and call it */
  ((void (*)(sc_object_t *)) oinmi) (o);
}

void
sc_object_print (sc_object_t * o, FILE * f)
{
  sc_object_system_t *s = o->s;
  sc_void_function_t  oinmi;

  /* get the implementation of this method for this object */
  oinmi = sc_object_method_lookup (s, (sc_void_function_t) sc_object_print,
                                   (void *) o);

  /* cast object instance method implementation appropriately and call it */
  ((void (*)(sc_object_t *, FILE *)) oinmi) (o, f);
}
