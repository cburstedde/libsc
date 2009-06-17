
#include <boat.h>
#include <car.h>
#include <tunedcar.h>
#include <vehicle.h>
#include <sc.h>
#include <sc_object.h>

int
main (int argc, char **argv)
{
  int                 i;
  int                 tickets;
  float               wheels1, wheels2;
  sc_object_system_t *scos;
  sc_object_t        *os[3], *o;

  scos = sc_object_system_new ();

  os[0] = &car_create (scos)->object;
  os[1] = &tuned_car_create (scos, 2)->car.object;
  os[2] = &boat_create (scos)->object;

  wheels1 = car_wheelsize_V (os[0]);
  wheels2 = car_wheelsize_V (os[1]);
  tickets = tuned_car_tickets_V (os[1]);
  SC_INFOF ("We have wheelsizes %f and %f and tickets %d\n",
            wheels1, wheels2, tickets);

  for (i = 0; i < 3; ++i) {
    o = os[i];
    vehicle_accelerate_I (o);
    sc_object_print_V (o, stdout);
    sc_object_destroy_V (o);
  }

  SC_ASSERT (scos->methods->elem_count == 0);
  sc_object_system_destroy (scos);

  return 0;
}
