
#ifndef CAR_H
#define CAR_H

#include <sc.h>
#include <sc_object.h>

/*
  car is a subclass of sc_object_t
  car implements interface vehicle
*/

typedef struct Car
{
  sc_object_t         object;
  float               speed;
  float               wheelsize;
}
Car;

/* virtual method prototypes */
float               car_wheelsize_V (sc_object_t * o);

/* construction and destruction */
Car                *car_create (sc_object_system_t * s);
void                car_initialize (sc_object_system_t * s, Car * self);
void                car_finalize (Car * self);

/* implementation of virtual methods of sc_object_t */
void                car_destroy (Car * self);
void                car_print (Car * self, FILE * out);

/* implementation of virtual methods of car */
float               car_wheelsize (Car * self);

/* implementation of interface methods of vehicle */
void                car_accelerate (Car * self);

#endif /* !CAR_H */
