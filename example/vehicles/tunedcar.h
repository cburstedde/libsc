
#ifndef TUNEDCAR_H
#define TUNEDCAR_H

#include <car.h>
#include <sc.h>
#include <sc_object.h>

/*
  tunedcar is a subclass of car
  tunedcar implements interface vehicle
*/

typedef struct TunedCar
{
  Car                 car;
  int                 faster;
}
TunedCar;

/* construction */
void                tuned_car_initialize (sc_object_system_t * s,
                                          TunedCar * self, int faster);
TunedCar           *tuned_car_create (sc_object_system_t * s, int faster);

/* methods for sc_object_t */
void                tuned_car_print (TunedCar * self, FILE * out);

/* methods for vehicle */
void                tuned_car_accelerate (TunedCar * self);

#endif /* !TUNEDCAR_H */
