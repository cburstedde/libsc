
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
}
Car;

/* construction */
void                car_initialize (sc_object_system_t * s, Car * self);
Car                *car_create (sc_object_system_t * s);

/* methods for sc_object_t */
void                car_destroy (Car * self);
void                car_print (Car * self, FILE * out);

/* methods for vehicle */
void                car_accelerate (Car * self);

#endif /* !CAR_H */
