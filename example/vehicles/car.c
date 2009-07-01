
#include <car.h>

const char         *car_type = "car";

static void
initialize_fn (sc_object_t * o)
{
  SC_ASSERT (sc_object_is_type (o, car_type));

  /*
  car->speed = 0;
  car->wheelsize = 17;
  */
}

static void
write_fn (sc_object_t * o, FILE * out)
{
  SC_ASSERT (sc_object_is_type (o, car_type));

  /*
  fprintf (out, "Car (wheel size %f) speeds at %f km/h\n",
           car->wheelsize, car->speed);
  */
}

static float
wheelsize_fn (sc_object_t * o)
{
  SC_ASSERT (sc_object_is_type (o, car_type));

  return 0; // car->wheelsize;
}

void
car_register_methods (sc_object_t * o)
{
  sc_object_system_t *s = o->s;

  ++o->num_regs;
  sc_object_method_register (s, (sc_void_function_t) sc_object_initialize,
                             o, (sc_void_function_t) initialize_fn);

  ++o->num_regs;
  sc_object_method_register (s, (sc_void_function_t) sc_object_write,
                             o, (sc_void_function_t) write_fn);

  ++o->num_regs;
  sc_object_method_register (s, (sc_void_function_t) car_wheelsize,
                             o, (sc_void_function_t) wheelsize_fn);
}

sc_object_t        *
car_klass_new (sc_object_t * d)
{
  sc_object_t        *o;

  SC_ASSERT (d != NULL);

  o = sc_object_alloc (d->s);
  sc_object_delegate_push (o, d);
  car_register_methods (o);
  sc_object_initialize (o);

  return o;
}

float
car_wheelsize (sc_object_t * o)
{
  sc_void_function_t  oinmi;

  oinmi = sc_object_delegate_lookup (o, (sc_void_function_t) car_wheelsize);
  SC_ASSERT (oinmi != NULL);

  return ((float (*)(sc_object_t *)) oinmi) (o);
}
