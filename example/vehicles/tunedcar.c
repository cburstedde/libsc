
#include <tunedcar.h>
#include <vehicle.h>

void
tuned_car_print (TunedCar * self, FILE * out)
{
  fprintf (out, "Tuned car speeds at %f km/h\n", self->car.speed);
}

void
tuned_car_accelerate (TunedCar * self)
{
  int                 i;

  for (i = 0; i < self->faster; ++i) {
    car_accelerate (&self->car);
  }
}

void
tuned_car_initialize (sc_object_system_t * s, TunedCar * self, int faster)
{
  car_initialize (s, &self->car);

  sc_object_method_override (s, (sc_void_function_t) sc_object_print,
                             self, (sc_void_function_t) tuned_car_print);
  sc_object_method_override (s, (sc_void_function_t) vehicle_accelerate,
                             self, (sc_void_function_t) tuned_car_accelerate);

  self->faster = faster;
}

TunedCar           *
tuned_car_create (sc_object_system_t * s, int faster)
{
  TunedCar           *self = SC_ALLOC (TunedCar, 1);

  tuned_car_initialize (s, self, faster);

  return self;
}
