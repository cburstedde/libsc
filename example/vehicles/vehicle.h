
#ifndef VEHICLE_H
#define VEHICLE_H

#include <sc.h>
#include <sc_object.h>

/*
  vehicle is an interface
 */

extern const char  *vehicle_type;

/* interface method prototypes */
void                vehicle_accelerate (sc_object_t * o);

#endif /* !VEHICLE_H */
