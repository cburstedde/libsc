
#ifndef SC_OBJECT_H
#define SC_OBJECT_H

#include <sc.h>
#include <sc_object_system.h>

typedef struct sc_object
{
  sc_object_system_t *s;
}
sc_object_t;

/* virtual method prototypes */
void                sc_object_destroy_V (sc_object_t * o);
void                sc_object_print_V (sc_object_t * o, FILE * f);

#endif /* !SC_OBJECT_H */
