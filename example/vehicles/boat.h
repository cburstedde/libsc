
#ifndef BOAT_H
#define BOAT_H

#include <sc.h>
#include <sc_object.h>

/*
  boat is a subclass of sc_object_t
  boat implements interface vehicle
*/

typedef struct Boat
{
  sc_object_t         object;
  float               speed;
}
Boat;

Boat               *boat_create (sc_object_system_t * s);

#endif /* !BOAT_H */
