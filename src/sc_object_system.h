
#ifndef SC_OBJECT_SYSTEM_H
#define SC_OBJECT_SYSTEM_H

#include <sc.h>
#include <sc_containers.h>

/*
  How could an interface-based object system work?

  As in OBC, methods are registered at runtime in the constructor.
  These method associations can be stored in hash tables.

  The hash key could be made up of the object memory address (unique)
  and the memory address of the method prototype (also unique).

  Say we want to register an implementation for the method obj_print
  for the object car with the function car_print.

  key = mix (&car, car_print)
  value = {
    void (*fn) (void)           the virtual function to call
    void * self;                this is passed to fn as self
  }
*/

/* generic virtual method prototype */
typedef void        (*sc_void_function_t) (void);

/* virtual method association stored in hash table */
typedef struct sc_object_method
{
  sc_void_function_t  ifm;      /* interface method */
  void               *o;        /* object instance */
  sc_void_function_t  oinmi;    /* object instance method implementation */
}
sc_object_method_t;

/*
  all object administrative information is stored here
  so you can have multiple object systems running independently
*/
typedef struct sc_object_system
{
  sc_hash_t          *methods;  /* hash table for method associations */
  sc_mempool_t       *mpool;    /* memory pool for association values */
}
sc_object_system_t;

sc_object_system_t *sc_object_system_new (void);
void                sc_object_system_destroy (sc_object_system_t * s);

/** register the implementation of an interface method for an object
 * \param[in] s     the object system context
 * \param[in] ifm   interface method
 * \param[in] o     object instance
 * \param[in] oinmi object instance method implementation
 */
void                sc_object_method_register (sc_object_system_t * s,
                                               sc_void_function_t ifm,
                                               void *o,
                                               sc_void_function_t oinmi);

/** unregister the implementation of an interface method for an object
 * \param[in] s     the object system context
 * \param[in] ifm   interface method
 * \param[in] o     object instance
 * \return          object instance method implementation
 */
sc_void_function_t  sc_object_method_unregister (sc_object_system_t * s,
                                                 sc_void_function_t ifm,
                                                 void *o);

sc_void_function_t  sc_object_method_lookup (sc_object_system_t * s,
                                             sc_void_function_t ifm, void *o);
void                sc_object_method_override (sc_object_system_t * s,
                                               sc_void_function_t ifm,
                                               void *o,
                                               sc_void_function_t oinmi);

#endif /* !SC_OBJECT_SYSTEM_H */
