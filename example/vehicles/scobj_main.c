
#include <boat.h>
#include <car.h>
#include <vehicle.h>
#include <sc.h>
#include <sc_object.h>

int
main (int argc, char **argv)
{
  int                 i;
  sc_object_system_t *scos;
  sc_object_t        *os[3], *o;

  scos = sc_object_system_new ();

  os[0] = &car_create (scos)->object;
  os[1] = &car_create (scos)->object;
  os[2] = &boat_create (scos)->object;

  for (i = 0; i < 3; ++i) {
    o = os[i];
    vehicle_accelerate (o);
    sc_object_print (o, stdout);
    sc_object_destroy (o);
  }

  SC_ASSERT (scos->methods->elem_count == 0);
  sc_object_system_destroy (scos);

  return 0;
}
