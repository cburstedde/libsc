
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
  float               speed;
  const char         *name;
}
Boat;

extern const char  *boat_type;

/* construction */
sc_object_t        *boat_klass_new (sc_object_t * d);
sc_object_t        *boat_new (sc_object_t * d, const char *name);

/* data */
Boat               *boat_get_data (sc_object_t * o);

#endif /* !BOAT_H */
