
#include <car.h>
#include <vehicle.h>

void
car_print (Car * self, FILE * out)
{
  fprintf (out, "Car (wheel size %f) speeds at %f km/h\n",
           self->wheelsize, self->speed);
}

float
car_wheelsize (Car * self)
{
  return self->wheelsize;
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
                               (sc_void_function_t) sc_object_destroy_V, self);
  sc_object_method_unregister (self->object.s,
                               (sc_void_function_t) sc_object_print_V, self);
  sc_object_method_unregister (self->object.s,
                               (sc_void_function_t) car_wheelsize_V, self);
  sc_object_method_unregister (self->object.s,
                               (sc_void_function_t) vehicle_accelerate_I, self);

  SC_FREE (self);
}

void
car_initialize (sc_object_system_t * s, Car * self)
{
  self->object.s = s;

  sc_object_method_register (s, (sc_void_function_t) sc_object_destroy_V,
                             self, (sc_void_function_t) car_destroy);
  sc_object_method_register (s, (sc_void_function_t) sc_object_print_V,
                             self, (sc_void_function_t) car_print);
  sc_object_method_register (s, (sc_void_function_t) car_wheelsize_V,
                             self, (sc_void_function_t) car_wheelsize);
  sc_object_method_register (s, (sc_void_function_t) vehicle_accelerate_I,
                             self, (sc_void_function_t) car_accelerate);

  self->speed = 0;
  self->wheelsize = 17;
}

Car                *
car_create (sc_object_system_t * s)
{
  Car                *self = SC_ALLOC (Car, 1);

  car_initialize (s, self);

  return self;
}

float
car_wheelsize_V (sc_object_t * o)
{
  sc_object_system_t *s = o->s;
  sc_void_function_t  oinmi;

  /* get the implementation of this method for this object */
  oinmi = sc_object_method_lookup (s, (sc_void_function_t) car_wheelsize_V,
                                   (void *) o);

  /* cast object instance method implementation appropriately and call it */
  return ((float (*)(sc_object_t *)) oinmi) (o);
}
