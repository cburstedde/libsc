/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2009 Carsten Burstedde, Lucas Wilcox.

  The SC Library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with the SC Library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SC_OBJECT_H
#define SC_OBJECT_H

#include <sc.h>
#include <sc_containers.h>
#include <sc_keyvalue.h>

SC_EXTERN_C_BEGIN;

typedef void        (*sc_object_method_t) (void);

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
  int                 skip_top;
  int                 accept_self;
  int                 accept_delegate;
  int                 (*callfn) (sc_object_t *, sc_object_method_t, void *);
  void               *user_data;
  sc_object_t        *last_match;
}
sc_object_recursion_context_t;
/* *INDENT-ON* */

extern const char  *sc_object_type;

/** Increase the reference count of an object by 1.
 */
void                sc_object_ref (sc_object_t * o);

/** Decrease the reference count of an object by 1.
 * When the count reaches zero, sc_object_finalize is called
 * and the object is freed.  This is the standard way of destroying objects.
 */
void                sc_object_unref (sc_object_t * o);

/** Duplicate a pointer to an object.
 * The duplicate and the original object are identical.
 * Changes to one reflect immediately in the other.
 * As usual the duplicate should be destroyed with sc_object_unref.
 * \return          The object \a o with reference count increased by 1.
 */
sc_object_t        *sc_object_dup (sc_object_t * o);

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
int                 sc_object_recursion (sc_object_t * o,
                                         sc_object_recursion_context_t * rc);

/** Register the implementation of an interface method for an object.
 * If the method is already registered it is replaced.
 * \param[in] o     object instance
 * \param[in] ifm   interface method
 * \param[in] oinmi object instance method implementation
 * \return          true if the method did not exist and was added.
 */
int                 sc_object_method_register (sc_object_t * o,
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

/* delegates */
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
                                               int skip_top,
                                               sc_object_t ** m);



/* construction */
sc_object_t        *sc_object_alloc (void);
sc_object_t        *sc_object_klass_new (void);
sc_object_t        *sc_object_new_from_klass (sc_object_t * d,
                                              sc_object_arguments_t * args);
sc_object_t        *sc_object_new_from_klass_values (sc_object_t * d, ...);

/* object data */
void               *sc_object_get_data (sc_object_t * o,
                                        sc_object_method_t ifm, size_t s);



/** There are 4 different semantics for calling virtual methods:
 * PRE-ALL          Call all methods in the delegate tree in pre-order.
 *                  Ancestors are called after and recursively.
 * PRE-BOOL         Call all methods in the delegate tree in pre-order
 *                  until one returns true.  Then stop the recursion.
 * PRE-FIRST        Call only the first method found in pre-order.
 *                  The method gets passed both the object \a o
 *                  and the object that contains the method that is called.
 * POST-ALL         Call all methods in the delegate tree in post-order.
 *                  Ancestors are called before and recursively.
 */

/** Determine whether an object is of a certain type.
 * An object can have multiple types, e.g., types inherited from an ancestor
 * or through implementation of interfaces which are understood as types.
 * Recursion: PRE-BOOL.
 */
int                 sc_object_is_type (sc_object_t * o, const char *type);

/** Make a deep copy of an object.
 * This function installs the original \a o as delegate in the copy.
 * The contents of the hash table and delegate array are not copied.
 * Then it invokes the copy virtual method which should copy all data.
 * This virtual method takes two arguments, the original and the copy.
 * Recursion: POST-ALL.
 */
sc_object_t        *sc_object_copy (sc_object_t * o);

/** Initialize object data.
 * Recursion: POST-ALL.
 */
void                sc_object_initialize (sc_object_t * o,
                                          sc_object_arguments_t * args);

/** Free object data.
 * Recursion: PRE-ALL.
 */
void                sc_object_finalize (sc_object_t * o);

/** Write object data to file.
 * Recursion: PRE-FIRST.
 */
void                sc_object_write (sc_object_t * o, FILE * out);

SC_EXTERN_C_END;

#endif /* !SC_OBJECT_H */
