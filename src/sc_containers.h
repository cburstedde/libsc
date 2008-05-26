/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2007,2008 Carsten Burstedde, Lucas Wilcox.

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

#ifndef SC_CONTAINERS_H
#define SC_CONTAINERS_H

/* sc.h should be included before this header file */

/** Function to compute a hash value of an object.
 * \return Returns an unsigned integer.
 */
typedef unsigned    (*sc_hash_function_t) (const void *v);

/** Function to check equality of two objects.
 * \return Returns 0 if *v1 is unequal *v2 and true otherwise.
 */
typedef int         (*sc_equal_function_t) (const void *v1, const void *v2);

/** The sc_array object provides a large array of equal-size elements.
 * The array can be resized.
 * Elements are accessed by their 0-based index, their address may change.
 * The size (== elem_count) of the array can be changed by array_resize.
 * Elements can be sorted with array_sort.
 * If the array is sorted elements can be binary searched with array_bsearch.
 * A priority queue is implemented with pqueue_add and pqueue_pop.
 * Use sort and search whenever possible, they are faster than the pqueue.
 */
typedef struct sc_array
{
  /* interface variables */
  size_t              elem_size;        /* size of a single element */
  size_t              elem_count;       /* number of valid elements */

  /* implementation variables */
  size_t              byte_alloc;       /* number of allocated bytes */
  char               *array;    /* linear array to store elements */
}
sc_array_t;

/** Creates a new array structure with 0 elements.
 * \param [in] elem_size  Size of one array element in bytes.
 * \return Returns an allocated and initialized array.
 */
sc_array_t         *sc_array_new (size_t elem_size);

/** Destroys an array structure.
 * \param [in] array  The array to be destroyed.
 */
void                sc_array_destroy (sc_array_t * array);

/** Initializes an already allocated array structure.
 * \param [in,out]  array       Array structure to be initialized.
 * \param [in] elem_size  Size of one array element in bytes.
 */
void                sc_array_init (sc_array_t * array, size_t elem_size);

/** Sets the array count to zero and frees all elements.
 * \param [in,out]  array       Array structure to be resetted.
 * \note Calling sc_array_init, then any array operations,
 *       then sc_array_reset is memory neutral.
 */
void                sc_array_reset (sc_array_t * array);

/** Sets the element count to new_count.
 * Reallocation takes place only occasionally, so this function is usually fast.
 */
void                sc_array_resize (sc_array_t * array, size_t new_count);

/** Sorts the array in ascending order wrt. the comparison function.
 * \param [in] compar The comparison function to be used.
 */
void                sc_array_sort (sc_array_t * array,
                                   int (*compar) (const void *,
                                                  const void *));

/** Removed duplicate entries from a sorted array.
 * \param [in,out] array  The array size will be reduced as necessary.
 * \param [in] compar     The comparison function to be used.
 */
void                sc_array_uniq (sc_array_t * array,
                                   int (*compar) (const void *,
                                                  const void *));

/** Performs a binary search on an array. The array must be sorted.
 * \param [in] key     An element to be searched for.
 * \param [in] compar  The comparison function to be used.
 * \return Returns the index into array for the item found, or -1.
 */
ssize_t             sc_array_bsearch (sc_array_t * array,
                                      const void *key,
                                      int (*compar) (const void *,
                                                     const void *));

/** Computes the adler32 checksum of array data.
 * This is a faster checksum than crc32, and it works with zeros as data.
 * \param [in] first_elem  Index of the element to start with.
 *                         Can be between 0 and elem_count (inclusive).
 */
unsigned            sc_array_checksum (sc_array_t * array, size_t first_elem);

/** Adds an element to a priority queue.
 * The priority queue is implemented as a heap in ascending order.
 * A heap is a binary tree where the children are not less than their parent.
 * Assumes that elements [0]..[elem_count-2] form a valid heap.
 * Then propagates [elem_count-1] upward by swapping if necessary.
 * \param [in] temp    Pointer to unused allocated memory of elem_size.
 * \param [in] compar  The comparison function to be used.
 * \return Returns the number of swap operations.
 * \note  If the return value is zero for all elements in an array,
 *        the array is sorted linearly and unchanged.
 */
size_t              sc_array_pqueue_add (sc_array_t * array,
                                         void *temp,
                                         int (*compar) (const void *,
                                                        const void *));

/** Pops the smallest element from a priority queue.
 * This function assumes that the array forms a valid heap in ascending order.
 * \param [out] result  Pointer to unused allocated memory of elem_size.
 * \param [in]  compar  The comparison function to be used.
 * \return Returns the number of swap operations.
 * \note This function resizes the array to elem_count-1.
 */
size_t              sc_array_pqueue_pop (sc_array_t * array,
                                         void *result,
                                         int (*compar) (const void *,
                                                        const void *));

/** Returns a pointer to an array element.
 * \param [in] index needs to be in [0]..[elem_count-1].
 */
/*@unused@*/
static inline void *
sc_array_index (sc_array_t * array, size_t index)
{
  SC_ASSERT (index < array->elem_count);

  return (void *) (array->array + (array->elem_size * index));
}

/** Remove the last element from an array and return a pointer to it.
 * This is safe since the array memory is not freed or reallocated.
 * \return Returns a pointer to the last element before removal.
 */
static inline void *
sc_array_pop (sc_array_t * array)
{
  SC_ASSERT (array->elem_count > 0);

  return (void *) (array->array + (array->elem_size * --array->elem_count));
}

/** Enlarge an array by one.  Grows the array memory if necessary.
 * \return Returns a pointer to the uninitialized newly added element.
 */
static inline void *
sc_array_push (sc_array_t * array)
{
  if (array->elem_size * (array->elem_count + 1) > array->byte_alloc) {
    const size_t        old_count = array->elem_count;

    sc_array_resize (array, old_count + 1);
    return (void *) (array->array + (array->elem_size * old_count));
  }

  return (void *) (array->array + (array->elem_size * array->elem_count++));
}

/** The sc_mempool object provides a large pool of equal-size elements.
 * The pool grows dynamically for element allocation.
 * Elements are referenced by their address which never changes.
 * Elements can be freed (that is, returned to the pool)
 *    and are transparently reused.
 */
typedef struct sc_mempool
{
  /* interface variables */
  size_t              elem_size;        /* size of a single element */
  size_t              elem_count;       /* number of valid elements */

  /* implementation variables */
  struct obstack      obstack;  /* holds the allocated elements */
  sc_array_t          freed;    /* buffers the freed elements */
}
sc_mempool_t;

/** Creates a new mempool structure.
 * \param [in] elem_size  Size of one element in bytes.
 * \return Returns an allocated and initialized memory pool.
 */
sc_mempool_t       *sc_mempool_new (size_t elem_size);

/** Destroys a mempool structure.
 * All elements that are still in use are invalidated.
 */
void                sc_mempool_destroy (sc_mempool_t * mempool);

/** Invalidates all previously returned pointers, resets count to 0.
 */
void                sc_mempool_reset (sc_mempool_t * mempool);

/** Allocate a single element.
 * Elements previously returned to the pool are recycled.
 * \return Returns a new or recycled element pointer.
 */
/*@unused@*/
static inline void *
sc_mempool_alloc (sc_mempool_t * mempool)
{
  void               *ret;
  sc_array_t         *freed = &mempool->freed;

  ++mempool->elem_count;

  if (freed->elem_count > 0) {
    ret = *(void **) sc_array_pop (freed);
  }
  else {
    ret = obstack_alloc (&mempool->obstack, (long) mempool->elem_size);
  }

#ifdef SC_DEBUG
  memset (ret, -1, mempool->elem_size);
#endif

  return ret;
}

/** Return a previously allocated element to the pool.
 * \param [in] elem  The element to be returned to the pool.
 */
/*@unused@*/
static inline void
sc_mempool_free (sc_mempool_t * mempool, void *elem)
{
  sc_array_t         *freed = &mempool->freed;

  SC_ASSERT (mempool->elem_count > 0);

#ifdef SC_DEBUG
  memset (elem, -1, mempool->elem_size);
#endif

  --mempool->elem_count;

  *(void **) sc_array_push (freed) = elem;
}

/** The sc_link structure is one link of a linked list.
 */
typedef struct sc_link
{
  void               *data;
  struct sc_link     *next;
}
sc_link_t;

/** The sc_list object provides a linked list.
 */
typedef struct sc_list
{
  /* interface variables */
  size_t              elem_count;
  sc_link_t          *first;
  sc_link_t          *last;

  /* implementation variables */
  int                 allocator_owned;
  sc_mempool_t       *allocator;        /* must allocate sc_link_t */
}
sc_list_t;

/** Allocate a linked list structure.
 * \param [in] allocator Memory allocator for sc_link_t, can be NULL.
 */
sc_list_t          *sc_list_new (sc_mempool_t * allocator);

/** Destroy a linked list structure in O(N).
 * \note If allocator was provided in sc_list_new, it will not be destroyed.
 */
void                sc_list_destroy (sc_list_t * list);

/** Initializes an already allocated list structure.
 * \param [in,out]  list       List structure to be initialized.
 * \param [in]      allocator  External memory allocator for sc_link_t.
 */
void                sc_list_init (sc_list_t * list, sc_mempool_t * allocator);

/** Removes all elements from a list in O(N).
 * \param [in,out]  list       List structure to be resetted.
 * \note Calling sc_list_init, then any list operations,
 *       then sc_list_reset is memory neutral.
 */
void                sc_list_reset (sc_list_t * list);

/** Unliks all list elements without returning them to the mempool.
 * This runs in O(1) but is dangerous because of potential memory leaks.
 * \param [in,out]  list       List structure to be unlinked.
 */
void                sc_list_unlink (sc_list_t * list);

void                sc_list_prepend (sc_list_t * list, void *data);
void                sc_list_append (sc_list_t * list, void *data);

/** Insert an element after a given position.
 * \param [in] pred The predecessor of the element to be inserted.
 */
void                sc_list_insert (sc_list_t * list,
                                    sc_link_t * pred, void *data);

/** Remove an element after a given position.
 * \param [in] pred  The predecessor of the element to be removed.
                     If \a pred == NULL, the first element is removed.
 * \return Returns the data of the removed element.
 */
void               *sc_list_remove (sc_list_t * list, sc_link_t * pred);

/** Remove an element from the front of the list.
 * \return Returns the data of the removed first list element.
 */
void               *sc_list_pop (sc_list_t * list);

/** The sc_hash implements a hash table.
 * It uses an array which has linked lists as elements.
 */
typedef struct sc_hash
{
  /* interface variables */
  size_t              elem_count;       /* total number of objects contained */

  /* implementation variables */
  sc_array_t         *slots;    /* the slot count is slots->elem_count */
  sc_hash_function_t  hash_fn;
  sc_equal_function_t equal_fn;
  size_t              resize_checks, resize_actions;
  int                 allocator_owned;
  sc_mempool_t       *allocator;        /* must allocate sc_link_t */
}
sc_hash_t;

/** Create a new hash table.
 * The number of hash slots is chosen dynamically.
 * \param [in] hash_fn     Function to compute the hash value.
 * \param [in] equal_fn    Function to test two objects for equality.
 * \param [in] allocator   Memory allocator for sc_link_t, can be NULL.
 */
sc_hash_t          *sc_hash_new (sc_hash_function_t hash_fn,
                                 sc_equal_function_t equal_fn,
                                 sc_mempool_t * allocator);
/** Destroy a hash table in O(N).
 * \note If allocator was provided in sc_hash_new, it will not be destroyed.
 */
void                sc_hash_destroy (sc_hash_t * hash);

/** Remove all entries from a hash table in O(N). */
void                sc_hash_reset (sc_hash_t * hash);

/** Unlinks all hash elements without returning them to the mempool.
 * This runs faster than sc_hash_reset, still in O(N),
 *    but is dangerous because of potential memory leaks.
 * \param [in,out]  hash       Hash structure to be unlinked.
 */
void                sc_hash_unlink (sc_hash_t * hash);

/** Same effect as unlink and destroy, but in O(1).
 * This is dangerous because of potential memory leaks.
 * \param [in]  hash       Hash structure to be unlinked and destroyed.
 */
void                sc_hash_unlink_destroy (sc_hash_t * hash);

/** Check if an object is contained in the hash table.
 * \param [in]  v      The object to be looked up.
 * \param [out] found  If found != NULL, *found is set to the object
 *                     if the object is found.
 * \return Returns 1 if object is found, 0 otherwise.
 */
int                 sc_hash_lookup (sc_hash_t * hash, void *v, void **found);

/** Insert an object into a hash table if it is not contained already.
 * \param [in]  v      The object to be inserted.
 * \param [out] found  If found != NULL, *found is set to the object
 *                     that is already contained if that exists.
 * \return Returns 1 if object is added, 0 if it is already contained.
 */
int                 sc_hash_insert_unique (sc_hash_t * hash, void *v,
                                           void **found);

/** Remove an object from a hash table.
 * \param [in]  v      The object to be removed.
 * \param [out] found  If found != NULL, *found is set to the object
                       that is removed if that exists.
 * \return Returns 1 if object is found, 0 if is not contained.
 */
int                 sc_hash_remove (sc_hash_t * hash, void *v, void **found);

/** Compute and print statistical information about the occupancy.
 */
void                sc_hash_print_statistics (int log_priority,
                                              sc_hash_t * hash);

#endif /* !SC_CONTAINERS_H */
