
#include <boat.h>

typedef struct BoatImpl
{
  float               speed;
}
BoatImpl;

void
boat_print (Boat self, FILE * out)
{
  fprintf (out, "Boat speeds at %f km/h\n", self->speed);
}

void
boat_accelerate (Boat self)
{
  self->speed += 10;
}

float
boat_getSpeed (Boat self)
{
  return self->speed;
}

void
boat_destroy (Boat self)
{
  free (self);
}

Boat
boat_create ()
{
  Boat                self = SC_ALLOC (BoatImpl, 1);
  self->speed = 0.0f;
  return self;
}
