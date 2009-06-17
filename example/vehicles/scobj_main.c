
#include <boat.h>
#include <car.h>
#include <sc.h>
#include <sc_object.h>

int
main (int argc, char **argv)
{
  Car                 car = car_create ();
  Boat                boat = boat_create ();

  sc_object_system_t *scos;

  scos = sc_object_system_new ();

  car_accelerate (car);
  car_print (car, stdout);
  car_destroy (car);

  boat_accelerate (boat);
  boat_print (boat, stdout);
  boat_destroy (boat);

  sc_object_system_destroy (scos);

  return 0;
}
