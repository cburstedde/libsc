
#ifndef SC_OBJECT_H
#define SC_OBJECT_H

#include <sc.h>
#include <sc_containers.h>

typedef void        (*sc_object_method_t) (void);

typedef enum {
  SC_OBJECT_VALUE_NONE,
  SC_OBJECT_VALUE_INT,
  SC_OBJECT_VALUE_DOUBLE,
  SC_OBJECT_VALUE_STRING,
  SC_OBJECT_VALUE_POINTER,
}
sc_object_value_type_t;

typedef struct sc_object_value
{
  const char         *key;
  sc_object_value_type_t type;
  union
  {
    int                 i;
    double              g;
    const char         *s;
    void               *v;
  }
  value;
}
sc_object_value_t;

typedef struct sc_object_arguments
{
  sc_hash_t          *hash;
  sc_mempool_t       *value_allocator;
}
sc_object_arguments_t;

typedef struct sc_object_entry
{
  sc_object_method_t  key;
  sc_object_method_t  oinmi;
  void               *odata;
}
sc_object_entry_t;

typedef struct sc_object
{
  int                 num_refs; /* reference count */
  sc_hash_t          *table;    /* contains sc_object_entry_t elements */
  sc_array_t          delegates;
}
sc_object_t;

typedef struct sc_object_recursion_match
{
  sc_object_method_t  oinmi;
}
sc_object_recursion_match_t;

/* *INDENT-OFF* HORRIBLE indent bug */
typedef struct sc_object_recursion_context
{
  sc_hash_t          *visited;
  sc_object_method_t  lookup;
  sc_array_t         *found;
  bool                skip_top;
  bool                accept_self;
  bool                accept_delegate;
  bool                (*callfn) (sc_object_t *, sc_object_method_t, void *);
  void               *user_data;
  sc_object_t        *last_match;
}
sc_object_recursion_context_t;
/* *INDENT-ON* */

extern const char  *sc_object_type;

/* reference counting */
void                sc_object_ref (sc_object_t * o);
void                sc_object_unref (sc_object_t * o);

/** Convenience function to create an empty sc_object_recursion_context_t.
 * \param [in] rc           The recursion context to be initialized.
 * \param [in] ifm          Interface method used as key for lookup.
 * \param [in,out] found    If not NULL, call sc_array_init on \a found
 *                          and set rc->found = found.
 */
void                sc_object_recursion_init (sc_object_recursion_context_t
                                              * rc, sc_object_method_t ifm,
                                              sc_array_t * found);

/** Look up a method recursively for all delegates.
 * Search is in preorder.  Ignore objects that have already been searched.
 * Optionally all matches are pushed onto an array.
 * Optionally a callback is run on each match.
 * Early-exit options are available, see descriptions of the fields in \a rc.
 *
 * \param [in,out] o    The object to start looking.
 * \param [in,out] rc   Recursion context with rc->visited == NULL initially.
 *                      rc->lookup must contain a method to look up.
 *                      If rc->found is init'ed to sc_object_recursion_entry
 *                      all matches are pushed onto this array in preorder.
 *                      rc->skip_top skips matching of the toplevel object.
 *                      rc->accept_self skips search in delegates on a match.
 *                      rc->accept_delegate skips search in delegate siblings.
 *                      If rc->callfn != NULL it is called on a match, and
 *                      rc->user_data is passed as third parameter to callfn.
 *                      If the call returns true the search is ended.
 *                      rc->last_match points to the last object matched.
 * \return              True if a callback returns true.  If callfn == NULL,
 *                      true if any match was found.  False otherwise.
 */
bool                sc_object_recursion (sc_object_t * o,
                                         sc_object_recursion_context_t * rc);

/** Register the implementation of an interface method for an object.
 * If the method is already registered it is replaced.
 * \param[in] o     object instance
 * \param[in] ifm   interface method
 * \param[in] oinmi object instance method implementation
 * \return          true if the method did not exist and was added.
 */
bool                sc_object_method_register (sc_object_t * o,
                                               sc_object_method_t ifm,
                                               sc_object_method_t oinmi);

/** Unregister the implementation of an interface method for an object.
 * The method is required to exist.
 * \param[in] o     object instance
 * \param[in] ifm   interface method
 */
void                sc_object_method_unregister (sc_object_t * o,
                                                 sc_object_method_t ifm);

/** Look up a method in an object.  This function is not recursive.
 */
sc_object_method_t  sc_object_method_lookup (sc_object_t * o,
                                             sc_object_method_t ifm);

/* handle delegates */
void                sc_object_delegate_push (sc_object_t * o,
                                             sc_object_t * d);
void                sc_object_delegate_pop (sc_object_t * o);
void                sc_object_delegate_pop_all (sc_object_t * o);
sc_object_t        *sc_object_delegate_index (sc_object_t * o, int i);

/** Look up an object method recursively.
 * \param [in] skip_top If true then the object o is not tested, only
 *                      its delegates recursively.
 * \param [out] f       If not NULL will be set to the matching object.
 */
sc_object_method_t  sc_object_delegate_lookup (sc_object_t * o,
                                               sc_object_method_t ifm,
                                               bool skip_top,
                                               sc_object_t ** m);

/* passing arguments */
/* Arguments come in pairs of 2: static string "type:key" and value;
   type is a letter like the identifier names in sc_object_value.value */
sc_object_arguments_t *sc_object_arguments_new (int dummy, ...);
void                sc_object_arguments_destroy (sc_object_arguments_t *args);
int                 sc_object_arguments_int (sc_object_arguments_t * args,
                                             const char *key);
double              sc_object_arguments_double (sc_object_arguments_t * args,
                                                const char *key);
const char         *sc_object_arguments_string (sc_object_arguments_t * args,
                                                const char *key);
void               *sc_object_arguments_pointer (sc_object_arguments_t * args,
                                                 const char *key);

/* construction */
sc_object_t        *sc_object_alloc (void);
sc_object_t        *sc_object_klass_new (void);
sc_object_t        *sc_object_new_from_klass (sc_object_t * d);

/* handle object data */
void               *sc_object_get_data (sc_object_t * o,
                                        sc_object_method_t ifm, size_t s);

/* virtual method prototypes */
/* All delegate's methods are called in pre-order until one returns true */
bool                sc_object_is_type (sc_object_t * o, const char *type);
/* All delegate's methods are called in post-order */
void                sc_object_initialize (sc_object_t * o);
/* All delegate's methods are called in pre-order */
void                sc_object_finalize (sc_object_t * o);
/* Standard virtual methods get passed the match object, see sc_object.c */
void                sc_object_write (sc_object_t * o, FILE * out);

#endif /* !SC_OBJECT_H */
