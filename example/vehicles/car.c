
#include <car.h>
#include <vehicle.h>

void
car_print (Car * self, FILE * out)
{
  fprintf (out, "Car speeds at %f km/h\n", self->speed);
}

void
car_accelerate (Car * self)
{
  self->speed += 100;
}

void
car_destroy (Car * self)
{
  /* we should know what methods self has registered, improve this */
  sc_object_method_unregister (self->object.s,
                               (sc_void_function_t) sc_object_destroy, self);
  sc_object_method_unregister (self->object.s,
                               (sc_void_function_t) sc_object_print, self);
  sc_object_method_unregister (self->object.s,
                               (sc_void_function_t) vehicle_accelerate, self);

  SC_FREE (self);
}

void
car_initialize (sc_object_system_t * s, Car * self)
{
  self->object.s = s;

  sc_object_method_register (s, (sc_void_function_t) sc_object_destroy,
                             self, (sc_void_function_t) car_destroy);
  sc_object_method_register (s, (sc_void_function_t) sc_object_print,
                             self, (sc_void_function_t) car_print);
  sc_object_method_register (s, (sc_void_function_t) vehicle_accelerate,
                             self, (sc_void_function_t) car_accelerate);

  self->speed = 0.0f;
}

Car                *
car_create (sc_object_system_t * s)
{
  Car                *self = SC_ALLOC (Car, 1);

  car_initialize (s, self);

  return self;
}
