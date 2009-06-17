
#include <tunedcar.h>
#include <vehicle.h>

void
tuned_car_print (TunedCar * self, FILE * out)
{
  fprintf (out, "Tuned car (wheel size %f) speeds at %f km/h\n",
           self->car.wheelsize, self->car.speed);
}

void
tuned_car_accelerate (TunedCar * self)
{
  int                 i;

  for (i = 0; i < self->faster; ++i) {
    car_accelerate (&self->car);
  }
}

int
tuned_car_tickets (TunedCar * self)
{
  return self->tickets;
}

void
tuned_car_finalize (TunedCar * self)
{
  sc_object_method_unregister (self->car.object.s,
                               (sc_void_function_t) tuned_car_tickets_V,
                               self);

  /* post chain */
  car_finalize (&self->car);
}

void
tuned_car_destroy (TunedCar * self)
{
  tuned_car_finalize (self);

  SC_FREE (self);
}

void
tuned_car_initialize (sc_object_system_t * s, TunedCar * self, int faster)
{
  /* pre chain */
  car_initialize (s, &self->car);

  /* sc_object */
  sc_object_method_override (s, (sc_void_function_t) sc_object_destroy_V,
                             self, (sc_void_function_t) tuned_car_destroy);
  sc_object_method_override (s, (sc_void_function_t) sc_object_print_V,
                             self, (sc_void_function_t) tuned_car_print);

  /* car */
  self->car.wheelsize = 21;

  /* tuned car */
  sc_object_method_register (s, (sc_void_function_t) tuned_car_tickets_V,
                             self, (sc_void_function_t) tuned_car_tickets);
  self->faster = faster;
  self->tickets = 0;

  /* vehicle */
  sc_object_method_override (s, (sc_void_function_t) vehicle_accelerate_I,
                             self, (sc_void_function_t) tuned_car_accelerate);
}

TunedCar           *
tuned_car_create (sc_object_system_t * s, int faster)
{
  TunedCar           *self = SC_ALLOC (TunedCar, 1);

  tuned_car_initialize (s, self, faster);

  return self;
}

int
tuned_car_tickets_V (sc_object_t * o)
{
  sc_object_system_t *s = o->s;
  sc_void_function_t  oinmi;

  /* get the implementation of this method for this object */
  oinmi =
    sc_object_method_lookup (s, (sc_void_function_t) tuned_car_tickets_V,
                             (void *) o);

  /* cast object instance method implementation appropriately and call it */
  return ((int (*)(sc_object_t *)) oinmi) (o);
}
