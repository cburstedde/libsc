
#include <boat.h>
#include <car.h>
#include <sc.h>

int
main (int argc, char **argv)
{
  Car                 car = car_create ();
  Boat                boat = boat_create ();

  car_accelerate (car);
  car_print (car, stdout);
  car_destroy (car);

  boat_accelerate (boat);
  boat_print (boat, stdout);
  boat_destroy (boat);

  return 0;
}
