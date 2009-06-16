
#ifndef CAR_H
#define CAR_H

#include <sc.h>

typedef struct CarImpl *Car;

Car                 car_create (void);
void                car_destroy (Car self);
void                car_print (Car self, FILE * out);
void                car_accelerate (Car self);
float               car_getSpeed (Car self);

#endif /* !CAR_H */
