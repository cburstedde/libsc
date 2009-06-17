
#include <boat.h>
#include <vehicle.h>

static void
boat_print (Boat * self, FILE * out)
{
  fprintf (out, "Boat speeds at %f km/h\n", self->speed);
}

static void
boat_accelerate (Boat * self)
{
  self->speed += 10;
}

static void
boat_destroy (Boat * self)
{
  /* we should know what methods self has registered, improve this */
  sc_object_method_unregister (self->object.s,
                               (sc_void_function_t) sc_object_destroy, self);
  sc_object_method_unregister (self->object.s,
                               (sc_void_function_t) sc_object_print, self);
  sc_object_method_unregister (self->object.s,
                               (sc_void_function_t) vehicle_accelerate, self);

  free (self);
}

Boat               *
boat_create (sc_object_system_t * s)
{
  Boat               *self = SC_ALLOC (Boat, 1);

  self->object.s = s;
  sc_object_method_register (s, (sc_void_function_t) sc_object_print,
                             self, (sc_void_function_t) boat_print);
  sc_object_method_register (s, (sc_void_function_t) sc_object_destroy,
                             self, (sc_void_function_t) boat_destroy);
  sc_object_method_register (s, (sc_void_function_t) vehicle_accelerate,
                             self, (sc_void_function_t) boat_accelerate);

  self->speed = 0.0f;
  return self;
}
