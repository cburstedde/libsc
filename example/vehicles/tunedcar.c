
#include <tunedcar.h>
#include <vehicle.h>

const char         *tuned_car_type = "tuned_car";

static              bool
is_type_fn (sc_object_t * o, const char *type)
{
  SC_LDEBUG ("tuned_car is_type\n");

  return !strcmp (type, tuned_car_type) || !strcmp (type, vehicle_type);
}

static void
initialize_fn (sc_object_t * o)
{
  Car                *car = car_get_data (o);
  TunedCar           *tuned_car = tuned_car_get_data (o);

  SC_LDEBUG ("tuned_car initialize\n");

  car->wheelsize = 21;
  tuned_car->faster = 2;
  tuned_car->tickets = 0;
}

static void
write_fn (sc_object_t * o, FILE * out)
{
  Car                *car = car_get_data (o);
  TunedCar           *tuned_car = tuned_car_get_data (o);

  fprintf (out, "Tuned car (wheel size %f tickets %d) speeds at %f km/h\n",
           car->wheelsize, tuned_car->tickets, car->speed);
}

static int
tickets_fn (sc_object_t * o)
{
  TunedCar           *tuned_car = tuned_car_get_data (o);

  return tuned_car->tickets;
}

static void
accelerate_fn (sc_object_t * o)
{
  Car                *car = car_get_data (o);

  SC_LDEBUG ("tuned car accelerate\n");

  car->speed += 20;
}

sc_object_t        *
tuned_car_klass_new (sc_object_t * d)
{
  bool                a1, a2, a3, a4, a5;
  sc_object_t        *o;

  SC_ASSERT (d != NULL);
  SC_ASSERT (sc_object_is_type (d, car_type));

  o = sc_object_alloc ();
  sc_object_delegate_push (o, d);

  a1 = sc_object_method_register (o, (sc_object_method_t) sc_object_is_type,
                                  (sc_object_method_t) is_type_fn);
  a2 =
    sc_object_method_register (o, (sc_object_method_t) sc_object_initialize,
                               (sc_object_method_t) initialize_fn);
  a3 =
    sc_object_method_register (o, (sc_object_method_t) sc_object_write,
                               (sc_object_method_t) write_fn);
  a4 =
    sc_object_method_register (o, (sc_object_method_t) tuned_car_tickets,
                               (sc_object_method_t) tickets_fn);
  a5 =
    sc_object_method_register (o, (sc_object_method_t) vehicle_accelerate,
                               (sc_object_method_t) accelerate_fn);
  SC_ASSERT (a1 && a2 && a3 && a4 && a5);

  sc_object_initialize (o);

  return o;
}

TunedCar           *
tuned_car_get_data (sc_object_t * o)
{
  SC_ASSERT (sc_object_is_type (o, tuned_car_type));

  return (TunedCar *) sc_object_get_data (o, (sc_object_method_t)
                                          tuned_car_get_data,
                                          sizeof (TunedCar));
}

int
tuned_car_tickets (sc_object_t * o)
{
  sc_object_method_t  oinmi;

  SC_ASSERT (sc_object_is_type (o, tuned_car_type));

  oinmi =
    sc_object_delegate_lookup (o, (sc_object_method_t) tuned_car_tickets);
  SC_ASSERT (oinmi != NULL);

  return ((int (*)(sc_object_t *)) oinmi) (o);
}
