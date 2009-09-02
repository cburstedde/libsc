
#ifndef TUNED_CAR_H
#define TUNED_CAR_H

#include <car.h>
#include <sc.h>
#include <sc_object.h>

/*
  tuned_car is a subclass of car
  tuned_car implements interface vehicle
*/

typedef struct TunedCar
{
  int                 faster;
  int                 tickets;
}
TunedCar;

extern const char  *tuned_car_type;

/* construction */
sc_object_t        *tuned_car_klass_new (sc_object_t * d);

/* data */
TunedCar           *tuned_car_get_data (sc_object_t * o);

/* virtual method prototypes */
int                 tuned_car_tickets (sc_object_t * o);

#endif /* !TUNED_CAR_H */
