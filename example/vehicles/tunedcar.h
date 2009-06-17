
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
  int                 tickets;
}
TunedCar;

/* virtual method prototypes */
int                 tuned_car_tickets_V (sc_object_t * o);

/* construction and destruction */
TunedCar           *tuned_car_create (sc_object_system_t * s, int faster);
void                tuned_car_initialize (sc_object_system_t * s,
                                          TunedCar * self, int faster);
void                tuned_car_finalize (TunedCar * self);

/* implementation of virtual methods for sc_object_t */
void                tuned_car_destroy (TunedCar * self);
void                tuned_car_print (TunedCar * self, FILE * out);

/* implementation of virtual methods for tuned_car */
int                 tuned_car_tickets (TunedCar * self);

/* implementation of interface methods for vehicle */
void                tuned_car_accelerate (TunedCar * self);

#endif /* !TUNEDCAR_H */
