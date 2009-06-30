
#include <car.h>

const char         *car_type = "car";

static void
initialize_fn (sc_object_t * o)
{
  Car                *car;

  SC_ASSERT (sc_object_is_type (o, car_type));
  car = (Car *) o;

  car->speed = 0;
  car->wheelsize = 17;
}

static void
write_fn (sc_object_t * o, FILE * out)
{
  Car                *car;

  SC_ASSERT (sc_object_is_type (o, car_type));
  car = (Car *) o;

  fprintf (out, "Car (wheel size %f) speeds at %f km/h\n",
           car->wheelsize, car->speed);
}

static float
wheelsize_fn (sc_object_t * o)
{
  Car                *car;

  SC_ASSERT (sc_object_is_type (o, car_type));
  car = (Car *) o;

  return car->wheelsize;
}

sc_object_t        *
car_new_klass (sc_object_system_t * s, sc_object_t * d, bool do_register)
{
  sc_object_t        *o;

  if (s == NULL) {
    SC_ASSERT (d != NULL);
    s = d->s;
  }
  else if (d != NULL) {
    SC_ASSERT (s == d->s);
  }

  o = sc_object_alloc (s, sizeof (Car));
  sc_object_delegate_push (o, d);

  if (do_register) {
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

  sc_object_initialize (o);

  return o;
}

sc_object_t        *
car_new (sc_object_t * d)
{
  SC_ASSERT (d != NULL);

  return car_new_klass (NULL, d, false);
}

float
car_wheelsize (sc_object_t * o)
{
  sc_void_function_t  oinmi;

  oinmi = sc_object_delegate_lookup (o, (sc_void_function_t) car_wheelsize);

  if (oinmi != NULL) {
    return ((float (*)(sc_object_t *)) oinmi) (o);
  }

  return 0;
}
