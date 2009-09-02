
#include <car.h>
#include <tunedcar.h>
#include <vehicle.h>

#define NUM_OBJECTS     4
#define NUM_CARS        2
#define NUM_TUNED_CARS  1
#define NUM_VEHICLES    2

int
main (int argc, char **argv)
{
  int                 i;
  int                 tickets;
  float               wheelsize;
  sc_object_t        *object_klass;
  sc_object_t        *car_klass;
  sc_object_t        *tuned_car_klass;
  sc_object_t        *o[NUM_OBJECTS];
  sc_object_t        *c[NUM_CARS];
  sc_object_t        *t[NUM_TUNED_CARS];
  sc_object_t        *v[NUM_VEHICLES];
  CarKlass           *car_klass_data;

  sc_init (MPI_COMM_NULL, true, true, NULL, SC_LP_DEFAULT);

  SC_INFO ("Construct sc_object_ts\n");
  object_klass = sc_object_klass_new ();
  o[0] = sc_object_new_from_klass (object_klass, NULL);
  o[1] = sc_object_new_from_klass (object_klass, NULL);

  SC_INFO ("Construct cars\n");
  car_klass = car_klass_new (object_klass);
  o[2] = v[0] = c[0] = car_new (car_klass, 17.);

  SC_INFO ("Construct tuned cars\n");
  tuned_car_klass = tuned_car_klass_new (car_klass);
  o[3] = v[1] = c[1] = t[0] = tuned_car_new (tuned_car_klass, 2);

  SC_INFO ("Write klasses\n");
  sc_object_write (object_klass, stdout);
  sc_object_write (car_klass, stdout);
  sc_object_write (tuned_car_klass, stdout);

  SC_INFO ("Get wheel sizes\n");
  for (i = 0; i < NUM_CARS; ++i) {
    SC_ASSERT (sc_object_is_type (c[i], car_type));
    wheelsize = car_wheelsize (c[i]);
    SC_INFOF ("Wheelsize of car[%d] is %f\n", i, wheelsize);
  }

  SC_INFO ("Get tickets\n");
  for (i = 0; i < NUM_TUNED_CARS; ++i) {
    SC_ASSERT (sc_object_is_type (t[i], tuned_car_type));
    tickets = tuned_car_tickets (t[i]);
    SC_INFOF ("Tickets of tuned car[%d] are %d\n", i, tickets);
  }

  SC_INFO ("Accelerate vehicles\n");
  for (i = 0; i < NUM_VEHICLES; ++i) {
    SC_ASSERT (sc_object_is_type (v[i], vehicle_type));
    sc_object_write (v[i], stdout);
    vehicle_accelerate (v[i]);
  }

  SC_INFO ("Write and destroy objects\n");
  for (i = 0; i < NUM_OBJECTS; ++i) {
    SC_ASSERT (sc_object_is_type (o[i], sc_object_type));
    sc_object_write (o[i], stdout);
    sc_object_unref (o[i]);
  }

  car_klass_data = car_get_klass_data (car_klass);
  SC_INFOF ("Car klass has %d repairs\n", car_klass_data->repairs);

  sc_object_unref (object_klass);
  sc_object_unref (tuned_car_klass);
  sc_object_unref (car_klass);

  sc_finalize ();

  return 0;
}
