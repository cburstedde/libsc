
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
  float               speed;
  float               wheelsize;
}
Car;

typedef struct CarKlass
{
  int                 repairs;
}
CarKlass;

extern const char  *car_type;

/* construction */
sc_object_t        *car_klass_new (sc_object_t * d);

/* data */
Car                *car_get_data (sc_object_t * o);
CarKlass           *car_get_klass_data (sc_object_t * o);

/* virtual method prototypes */
float               car_wheelsize (sc_object_t * o);

#endif /* !CAR_H */
