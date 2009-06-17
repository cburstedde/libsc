
#ifndef SC_OBJECT_H
#define SC_OBJECT_H

#include <sc.h>
#include <sc_object_system.h>

typedef struct sc_object
{
  sc_object_system_t *s;
}
sc_object_t;

void                sc_object_destroy (sc_object_t * o);
void                sc_object_print (sc_object_t * o, FILE * f);

#endif /* !SC_OBJECT_H */
