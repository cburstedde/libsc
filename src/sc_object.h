
#ifndef SC_OBJECT_H
#define SC_OBJECT_H

#include <sc.h>
#include <sc_containers.h>
#include <sc_object_system.h>

typedef struct sc_object
{
  sc_object_system_t *s;
  int                 num_refs; /* reference count */
  int                 num_regs; /* registered methods */
  sc_array_t          delegates;
}
sc_object_t;

extern const char  *sc_object_type;

/* reference counting */
void                sc_object_ref (sc_object_t * o);
void                sc_object_unref (sc_object_t * o);

/* handle delegates */
void                sc_object_delegate_push (sc_object_t * o,
                                             sc_object_t * d);
void                sc_object_delegate_pop (sc_object_t * o);
void                sc_object_delegate_popall (sc_object_t * o);
sc_void_function_t  sc_object_delegate_lookup (sc_object_t * o,
                                               sc_void_function_t ifm);

/* type and method handling */
bool                sc_object_is_type (sc_object_t * o, const char * type);
void                sc_object_register_methods (sc_object_t * o);

/* construction */
sc_object_t        *sc_object_alloc (sc_object_system_t * s);
sc_object_t        *sc_object_klass_new (sc_object_system_t * s);
sc_object_t        *sc_object_new_from_klass (sc_object_t * d);

/* virtual method prototypes */
const char         *sc_object_get_type (sc_object_t * o);
void                sc_object_initialize (sc_object_t * o);
void                sc_object_finalize (sc_object_t * o);
void                sc_object_write (sc_object_t * o, FILE * out);

#endif /* !SC_OBJECT_H */
