
#include <car.h>

typedef struct CarImpl
{
  float               speed;
}
CarImpl;

void
car_print (Car self, FILE * out)
{
  fprintf (out, "Car speeds at %f km/h\n", self->speed);
}

void
car_accelerate (Car self)
{
  self->speed += 100;
}

float
car_getSpeed (Car self)
{
  return self->speed;
}

void
car_destroy (Car self)
{
  free (self);
}

Car
car_create (void)
{
  Car                 self = SC_ALLOC (CarImpl, 1);
  self->speed = 0.0f;
  return self;
}
