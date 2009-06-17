
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

Car                *car_create (sc_object_system_t * s);

#endif /* !CAR_H */
