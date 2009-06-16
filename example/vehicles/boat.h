
#ifndef BOAT_H
#define BOAT_H

#include <sc.h>

typedef struct BoatImpl *Boat;

Boat                boat_create ();
void                boat_destroy (Boat self);
void                boat_print (Boat self, FILE * out);

void                boat_accelerate (Boat self);
float               boat_getSpeed (Boat self);

#endif /* !BOAT_H */
