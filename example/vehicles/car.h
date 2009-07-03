
#ifndef CAR_H
#define CAR_H

#include <sc.h>
#include <sc_object.h>

/*
  car is a subclass of sc_object_t
  car implements interface vehicle
*/

/*
typedef struct Car
{
  sc_object_t         object;
  float               speed;
  float               wheelsize;
}
Car;
*/

extern const char  *car_type;

/* construction */
sc_object_t        *car_klass_new (sc_object_t * d);

/* virtual method prototypes */
float               car_wheelsize (sc_object_t * o);

#endif /* !CAR_H */
