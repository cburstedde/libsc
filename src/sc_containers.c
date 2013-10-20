/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System

  The SC Library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with the SC Library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#include <sc_containers.h>
#ifdef SC_HAVE_ZLIB
#include <zlib.h>
#endif

/* array routines */

size_t
sc_array_memory_used (sc_array_t * array, int is_dynamic)
{
  return (is_dynamic ? sizeof (sc_array_t) : 0) +
    (SC_ARRAY_IS_OWNER (array) ? array->byte_alloc : 0);
}

sc_array_t         *
sc_array_new (size_t elem_size)
{
  sc_array_t         *array;

  array = SC_ALLOC (sc_array_t, 1);

  sc_array_init (array, elem_size);

  return array;
}

sc_array_t         *
sc_array_new_size (size_t elem_size, size_t elem_count)
{
  sc_array_t         *array;

  array = SC_ALLOC (sc_array_t, 1);

  sc_array_init_size (array, elem_size, elem_count);

  return array;
}

sc_array_t         *
sc_array_new_view (sc_array_t * array, size_t offset, size_t length)
{
  sc_array_t         *view;

  view = SC_ALLOC (sc_array_t, 1);

  sc_array_init_view (view, array, offset, length);

  return view;
}

sc_array_t         *
sc_array_new_data (void *base, size_t elem_size, size_t elem_count)
{
  sc_array_t         *view;

  view = SC_ALLOC (sc_array_t, 1);

  sc_array_init_data (view, base, elem_size, elem_count);

  return view;
}

void
sc_array_destroy (sc_array_t * array)
{
  if (SC_ARRAY_IS_OWNER (array)) {
    SC_FREE (array->array);
  }
  SC_FREE (array);
}

void
sc_array_init (sc_array_t * array, size_t elem_size)
{
  SC_ASSERT (elem_size > 0);

  array->elem_size = elem_size;
  array->elem_count = 0;
  array->byte_alloc = 0;
  array->array = NULL;
}

void
sc_array_init_size (sc_array_t * array, size_t elem_size, size_t elem_count)
{
  SC_ASSERT (elem_size > 0);

  array->elem_size = elem_size;
  array->elem_count = elem_count;
  array->byte_alloc = (ssize_t) (elem_size * elem_count);
  array->array = SC_ALLOC (char, (size_t) array->byte_alloc);
}

void
sc_array_init_view (sc_array_t * view, sc_array_t * array, size_t offset,
                    size_t length)
{
  SC_ASSERT (offset + length <= array->elem_count);

  view->elem_size = array->elem_size;
  view->elem_count = length;
  view->byte_alloc = -(ssize_t) (length * array->elem_size + 1);
  view->array = array->array + offset * array->elem_size;
}

void
sc_array_init_data (sc_array_t * view, void *base, size_t elem_size,
                    size_t elem_count)
{
  SC_ASSERT (elem_size > 0);

  view->elem_size = elem_size;
  view->elem_count = elem_count;
  view->byte_alloc = -(ssize_t) (elem_count * elem_size + 1);
  view->array = (char *) base;
}

void
sc_array_reset (sc_array_t * array)
{
  if (SC_ARRAY_IS_OWNER (array)) {
    SC_FREE (array->array);
  }
  array->array = NULL;

  array->elem_count = 0;
  array->byte_alloc = 0;
}

void
sc_array_resize (sc_array_t * array, size_t new_count)
{
  size_t              newoffs, roundup, newsize;
#ifdef SC_DEBUG
  size_t              oldoffs;
  size_t              i, minoffs;
#endif

  if (!SC_ARRAY_IS_OWNER (array)) {
    /* *INDENT-OFF* HORRIBLE indent bug */
    SC_ASSERT (new_count * array->elem_size <=
               (size_t) -(array->byte_alloc + 1));
    /* *INDENT-ON* */
    array->elem_count = new_count;
    return;
  }

  /* We know that this array is not a view now so we can call reset. */
  if (new_count == 0) {
    sc_array_reset (array);
    return;
  }

#ifdef SC_DEBUG
  oldoffs = array->elem_count * array->elem_size;
#endif
  array->elem_count = new_count;
  newoffs = array->elem_count * array->elem_size;
  roundup = (size_t) SC_ROUNDUP2_64 (newoffs);
  SC_ASSERT (roundup >= newoffs && roundup <= 2 * newoffs);

  if (newoffs > (size_t) array->byte_alloc ||
      roundup < (size_t) array->byte_alloc) {
    array->byte_alloc = (ssize_t) roundup;
  }
  else {
#ifdef SC_DEBUG
    if (newoffs < oldoffs) {
      memset (array->array + newoffs, -1, oldoffs - newoffs);
    }
    for (i = oldoffs; i < newoffs; ++i) {
      SC_ASSERT (array->array[i] == (char) -1);
    }
#endif
    return;
  }
  SC_ASSERT ((size_t) array->byte_alloc >= newoffs);

  newsize = (size_t) array->byte_alloc;
  array->array = SC_REALLOC (array->array, char, newsize);

#ifdef SC_DEBUG
  minoffs = SC_MIN (oldoffs, newoffs);
  SC_ASSERT (minoffs <= newsize);
  memset (array->array + minoffs, -1, newsize - minoffs);
#endif
}

void
sc_array_copy (sc_array_t * dest, sc_array_t * src)
{
  SC_ASSERT (SC_ARRAY_IS_OWNER (dest));
  SC_ASSERT (dest->elem_size == src->elem_size);

  sc_array_resize (dest, src->elem_count);
  memcpy (dest->array, src->array, src->elem_count * src->elem_size);
}

void
sc_array_sort (sc_array_t * array, int (*compar) (const void *, const void *))
{
  qsort (array->array, array->elem_count, array->elem_size, compar);
}

int
sc_array_is_sorted (sc_array_t * array,
                    int (*compar) (const void *, const void *))
{
  const size_t        count = array->elem_count;
  size_t              zz;
  void               *vold, *vnew;

  if (count <= 1) {
    return 1;
  }

  vold = sc_array_index (array, 0);
  for (zz = 1; zz < count; ++zz) {
    vnew = sc_array_index (array, zz);
    if (compar (vold, vnew) > 0) {
      return 0;
    }
    vold = vnew;
  }

  return 1;
}

int
sc_array_is_equal (sc_array_t * array, sc_array_t * other)
{
  if (array->elem_size != other->elem_size ||
      array->elem_count != other->elem_count) {
    return 0;
  }
  return !memcmp (array->array, other->array,
                  array->elem_size * array->elem_count);
}

void
sc_array_uniq (sc_array_t * array, int (*compar) (const void *, const void *))
{
  size_t              incount, dupcount;
  size_t              i, j;
  void               *elem1, *elem2, *temp;

  SC_ASSERT (SC_ARRAY_IS_OWNER (array));

  incount = array->elem_count;
  if (incount == 0) {
    return;
  }

  dupcount = 0;                 /* count duplicates */
  i = 0;                        /* read counter */
  j = 0;                        /* write counter */
  elem1 = sc_array_index (array, 0);
  while (i < incount) {
    elem2 = ((i < incount - 1) ? sc_array_index (array, i + 1) : NULL);
    if (i < incount - 1 && compar (elem1, elem2) == 0) {
      ++dupcount;
      ++i;
    }
    else {
      if (i > j) {
        temp = sc_array_index (array, j);
        memcpy (temp, elem1, array->elem_size);
      }
      ++i;
      ++j;
    }
    elem1 = elem2;
  }
  SC_ASSERT (i == incount);
  SC_ASSERT (j + dupcount == incount);
  sc_array_resize (array, j);
}

ssize_t
sc_array_bsearch (sc_array_t * array, const void *key,
                  int (*compar) (const void *, const void *))
{
  ssize_t             is = -1;
  char               *retval;

  retval = (char *)
    bsearch (key, array->array, array->elem_count, array->elem_size, compar);

  if (retval != NULL) {
    is = (ssize_t) ((retval - array->array) / array->elem_size);
    SC_ASSERT (is >= 0 && is < (ssize_t) array->elem_count);
  }

  return is;
}

void
sc_array_split (sc_array_t * array, sc_array_t * offsets, size_t num_types,
                sc_array_type_t type_fn, void *data)
{
  const size_t        count = array->elem_count;
  size_t              zi, *zp;
  size_t              guess, low, high, type, step;

  SC_ASSERT (offsets->elem_size == sizeof (size_t));

  sc_array_resize (offsets, num_types + 1);

  /** The point of this algorithm is to put offsets[i] into its final position
   * for i = 0,...,num_types, where the final position of offsets[i] is the
   * unique index k such that type_fn (array, j, data) < i for all j < k
   * and type_fn (array, j, data) >= i for all j >= k.
   *
   * The invariants of the loop are:
   *  1) if i < step, then offsets[i] <= low, and offsets[i] is final.
   *  2) if i >= step, then low is less than or equal to the final value of
   *     offsets[i].
   *  3) for 0 <= i <= num_types, offsets[i] is greater than or equal to its
   *     final value.
   *  4) for every index k in the array with k < low,
   *     type_fn (array, k, data) < step,
   *  5) for 0 <= i < num_types,
   *     for every index k in the array with k >= offsets[i],
   *     type_fn (array, k, data) >= i.
   *  6) if i < j, offsets[i] <= offsets[j].
   *
   * Initializing offsets[0] = 0, offsets[i] = count for i > 0,
   * low = 0, and step = 1, the invariants are trivially satisfied.
   */
  zp = (size_t *) sc_array_index (offsets, 0);
  *zp = 0;
  for (zi = 1; zi <= num_types; zi++) {
    zp = (size_t *) sc_array_index (offsets, zi);
    *zp = count;
  }

  if (count == 0 || num_types <= 1) {
    return;
  }

  /** Because count > 0 we can add another invariant:
   *   7) if step < num_types, low < high = offsets[step].
   */

  low = 0;
  high = count;                 /* high = offsets[step] */
  step = 1;
  for (;;) {
    guess = low + (high - low) / 2;     /* By (7) low <= guess < high. */
    type = type_fn (array, guess, data);
    SC_ASSERT (type < num_types);
    /** If type < step, then we can set low = guess + 1 and still satisfy
     * invariant (4).  Also, because guess < high, we are assured low <= high.
     */
    if (type < step) {
      low = guess + 1;
    }
    /** If type >= step, then setting offsets[i] = guess for i = step,..., type
     * still satisfies invariant (5).  Because guess >= low, we are assured
     * low <= high, and we maintain invariant (6).
     */
    else {
      for (zi = step; zi <= type; zi++) {
        zp = (size_t *) sc_array_index (offsets, zi);
        *zp = guess;
      }
      high = guess;             /* high = offsets[step] */
    }
    /** If low = (high = offsets[step]), then by invariants (2) and (3)
     * offsets[step] is in its final position, so we can increment step and
     * still satisfy invariant (1).
     */
    while (low == high) {
      /* By invariant (6), high cannot decrease here */
      ++step;                   /* sc_array_index might be a macro */
      high = *((size_t *) sc_array_index (offsets, step));
      /** If step = num_types, then by invariant (1) we have found the final
       * positions for offsets[i] for i < num_types, and offsets[num_types] =
       * count in all situations, so we are done.
       */
      if (step == num_types) {
        return;
      }
    }
    /** To reach this point it must be true that low < high, so we preserve
     * invariant (7).
     */
  }
}

int
sc_array_is_permutation (sc_array_t * newindices)
{
  size_t              count = newindices->elem_count;
  int                *counted = SC_ALLOC_ZERO (int, count);
  size_t              zi;
  size_t              zj;
  size_t             *newind;

  SC_ASSERT (newindices->elem_size == sizeof (size_t));
  newind = (size_t *) sc_array_index (newindices, 0);

  for (zi = 0; zi < count; zi++) {
    zj = newind[zi];
    if (zj >= count) {
      SC_FREE (counted);
      return 0;
    }
    counted[zj]++;
  }

  for (zi = 0; zi < count; zi++) {
    if (counted[zi] != 1) {
      SC_FREE (counted);
      return 0;
    }
  }

  SC_FREE (counted);
  return 1;
}

/** permute an array in place.  newind[i] is the new index for the data that
 * is currently at index i. entries in newind will be altered by this
 * procedure */
void
sc_array_permute (sc_array_t * array, sc_array_t * newindices, int keepperm)
{
  size_t              zi, zj, zk;
  char               *temp = SC_ALLOC (char, array->elem_size);
  char               *carray = array->array;
  size_t              esize = array->elem_size * sizeof (char);
  size_t              count = array->elem_count;
  size_t             *newind;

  SC_ASSERT (newindices->elem_size == sizeof (size_t));
  SC_ASSERT (newindices->elem_count == count);
  SC_ASSERT (sc_array_is_permutation (newindices));

  if (!keepperm) {
    newind = (size_t *) sc_array_index (newindices, 0);
  }
  else {
    newind = SC_ALLOC (size_t, count);
    memcpy (newind, sc_array_index (newindices, 0), count * sizeof (size_t));
  }

  zi = 0;
  zj = 0;

  while (zi < count) {
    /* zi is the index of the current pivot slot */
    /* zj is the old index of the data in the pivot */
    /* zk is the new index for what is in the pivot */
    zk = newind[zj];
    SC_ASSERT (zk < count);
    while (zk != zi) {
      /* vacate zk */
      memcpy (temp, carray + esize * zk, esize);
      /* copy pivot to zk */
      memcpy (carray + esize * zk, carray + esize * zi, esize);
      /* copy zk to pivot */
      memcpy (carray + esize * zi, temp, esize);
      /* what was in zk is now in the pivot zi */
      zj = zk;
      zk = newind[zk];
      SC_ASSERT (zk < count);
      /* change newind to reflect the fact that what is now in zj is what is
       * supposed to be in zj */
      newind[zj] = zj;
    }
    newind[zi] = zi;
    zj = (++zi);
  }

  if (keepperm) {
    SC_FREE (newind);
  }

  SC_FREE (temp);
}

unsigned
sc_array_checksum (sc_array_t * array)
{
#ifdef SC_HAVE_ZLIB
  uInt                bytes;
  uLong               crc;

  crc = adler32 (0L, Z_NULL, 0);
  if (array->elem_count == 0) {
    return (unsigned) crc;
  }

  bytes = (uInt) (array->elem_count * array->elem_size);
  crc = adler32 (crc, (const Bytef *) array->array, bytes);

  return (unsigned) crc;
#else
  SC_GLOBAL_LERROR("Configure did not find a recent enough zlib.  Abort.\n");

  return 0;
#endif
}

size_t
sc_array_pqueue_add (sc_array_t * array, void *temp,
                     int (*compar) (const void *, const void *))
{
  int                 comp;
  size_t              parent, child, swaps;
  const size_t        size = array->elem_size;
  void               *p, *c;

  /* this works on a pre-allocated array that is not a view */
  SC_ASSERT (SC_ARRAY_IS_OWNER (array));
  SC_ASSERT (array->elem_count > 0);

  swaps = 0;
  child = array->elem_count - 1;
  c = array->array + (size * child);
  while (child > 0) {
    parent = (child - 1) / 2;
    p = array->array + (size * parent);

    /* compare child to parent */
    comp = compar (p, c);
    if (comp <= 0) {
      break;
    }

    /* swap child and parent */
    memcpy (temp, c, size);
    memcpy (c, p, size);
    memcpy (p, temp, size);
    ++swaps;

    /* walk up the tree */
    child = parent;
    c = p;
  }

  return swaps;
}

size_t
sc_array_pqueue_pop (sc_array_t * array, void *result,
                     int (*compar) (const void *, const void *))
{
  int                 comp;
  size_t              new_count, swaps;
  size_t              parent, child, child1;
  const size_t        size = array->elem_size;
  void               *p, *c, *c1;
  void               *temp;

  /* array must not be empty or a view */
  SC_ASSERT (SC_ARRAY_IS_OWNER (array));
  SC_ASSERT (array->elem_count > 0);

  swaps = 0;
  new_count = array->elem_count - 1;

  /* extract root */
  parent = 0;
  p = array->array + (size * parent);
  memcpy (result, p, size);

  /* copy the last element to the top and reuse it as temp storage */
  temp = array->array + (size * new_count);
  if (new_count > 0) {
    memcpy (p, temp, size);
  }

  /* sift down the tree */
  while ((child = 2 * parent + 1) < new_count) {
    c = array->array + (size * child);

    /* check if child has a sibling and use that one if it is smaller */
    if ((child1 = 2 * parent + 2) < new_count) {
      c1 = array->array + (size * child1);
      comp = compar (c, c1);
      if (comp > 0) {
        child = child1;
        c = c1;
      }
    }

    /* sift down the parent if it is larger */
    comp = compar (p, c);
    if (comp <= 0) {
      break;
    }

    /* swap child and parent */
    memcpy (temp, c, size);
    memcpy (c, p, size);
    memcpy (p, temp, size);
    ++swaps;

    /* walk down the tree */
    parent = child;
    p = c;
  }

  /* we can resize down here only since we need the temp element above */
  sc_array_resize (array, new_count);

  return swaps;
}

/* mempool routines */

size_t
sc_mempool_memory_used (sc_mempool_t * mempool)
{
  return sizeof (sc_mempool_t) +
    obstack_memory_used (&mempool->obstack) +
    sc_array_memory_used (&mempool->freed, 0);
}

static void        *
sc_containers_malloc (size_t n)
{
  return sc_malloc (sc_package_id, n);
}

static void        *(*obstack_chunk_alloc) (size_t) = sc_containers_malloc;

static void
sc_containers_free (void *p)
{
  sc_free (sc_package_id, p);
}

static void         (*obstack_chunk_free) (void *) = sc_containers_free;

sc_mempool_t       *
sc_mempool_new (size_t elem_size)
{
  sc_mempool_t       *mempool;

  SC_ASSERT (elem_size > 0);
  SC_ASSERT (elem_size <= (size_t) INT_MAX);    /* obstack limited to int */

  mempool = SC_ALLOC (sc_mempool_t, 1);

  mempool->elem_size = elem_size;
  mempool->elem_count = 0;

  obstack_init (&mempool->obstack);
  sc_array_init (&mempool->freed, sizeof (void *));

  return mempool;
}

void
sc_mempool_destroy (sc_mempool_t * mempool)
{
  sc_array_reset (&mempool->freed);
  obstack_free (&mempool->obstack, NULL);

  SC_FREE (mempool);
}

void
sc_mempool_truncate (sc_mempool_t * mempool)
{
  sc_array_reset (&mempool->freed);
  obstack_free (&mempool->obstack, NULL);
  obstack_init (&mempool->obstack);
  mempool->elem_count = 0;
}

/* list routines */

size_t
sc_list_memory_used (sc_list_t * list, int is_dynamic)
{
  return (is_dynamic ? sizeof (sc_list_t) : 0) +
    (list->allocator_owned ? sc_mempool_memory_used (list->allocator) : 0);
}

sc_list_t          *
sc_list_new (sc_mempool_t * allocator)
{
  sc_list_t          *list;

  list = SC_ALLOC (sc_list_t, 1);

  list->elem_count = 0;
  list->first = NULL;
  list->last = NULL;

  if (allocator != NULL) {
    SC_ASSERT (allocator->elem_size == sizeof (sc_link_t));
    list->allocator = allocator;
    list->allocator_owned = 0;
  }
  else {
    list->allocator = sc_mempool_new (sizeof (sc_link_t));
    list->allocator_owned = 1;
  }

  return list;
}

void
sc_list_destroy (sc_list_t * list)
{

  if (list->allocator_owned) {
    sc_list_unlink (list);
    sc_mempool_destroy (list->allocator);
  }
  else {
    sc_list_reset (list);
  }
  SC_FREE (list);
}

void
sc_list_init (sc_list_t * list, sc_mempool_t * allocator)
{
  list->elem_count = 0;
  list->first = NULL;
  list->last = NULL;

  SC_ASSERT (allocator != NULL);
  SC_ASSERT (allocator->elem_size == sizeof (sc_link_t));

  list->allocator = allocator;
  list->allocator_owned = 0;
}

void
sc_list_reset (sc_list_t * list)
{
  sc_link_t          *lynk;
  sc_link_t          *temp;

  lynk = list->first;
  while (lynk != NULL) {
    temp = lynk->next;
    sc_mempool_free (list->allocator, lynk);
    lynk = temp;
    --list->elem_count;
  }
  SC_ASSERT (list->elem_count == 0);

  list->first = list->last = NULL;
}

void
sc_list_unlink (sc_list_t * list)
{
  list->first = list->last = NULL;
  list->elem_count = 0;
}

void
sc_list_prepend (sc_list_t * list, void *data)
{
  sc_link_t          *lynk;

  lynk = (sc_link_t *) sc_mempool_alloc (list->allocator);
  lynk->data = data;
  lynk->next = list->first;
  list->first = lynk;
  if (list->last == NULL) {
    list->last = lynk;
  }

  ++list->elem_count;
}

void
sc_list_append (sc_list_t * list, void *data)
{
  sc_link_t          *lynk;

  lynk = (sc_link_t *) sc_mempool_alloc (list->allocator);
  lynk->data = data;
  lynk->next = NULL;
  if (list->last != NULL) {
    list->last->next = lynk;
  }
  else {
    list->first = lynk;
  }
  list->last = lynk;

  ++list->elem_count;
}

void
sc_list_insert (sc_list_t * list, sc_link_t * pred, void *data)
{
  sc_link_t          *lynk;

  SC_ASSERT (pred != NULL);

  lynk = (sc_link_t *) sc_mempool_alloc (list->allocator);
  lynk->data = data;
  lynk->next = pred->next;
  pred->next = lynk;
  if (pred == list->last) {
    list->last = lynk;
  }

  ++list->elem_count;
}

void               *
sc_list_remove (sc_list_t * list, sc_link_t * pred)
{
  sc_link_t          *lynk;
  void               *data;

  if (pred == NULL) {
    return sc_list_pop (list);
  }

  SC_ASSERT (pred->next != NULL);

  lynk = pred->next;
  pred->next = lynk->next;
  data = lynk->data;
  if (list->last == lynk) {
    list->last = pred;
  }
  sc_mempool_free (list->allocator, lynk);

  --list->elem_count;
  return data;
}

void               *
sc_list_pop (sc_list_t * list)
{
  sc_link_t          *lynk;
  void               *data;

  SC_ASSERT (list->first != NULL);

  lynk = list->first;
  list->first = lynk->next;
  data = lynk->data;
  sc_mempool_free (list->allocator, lynk);
  if (list->first == NULL) {
    list->last = NULL;
  }

  --list->elem_count;
  return data;
}

/* hash table routines */

unsigned
sc_hash_function_string (const void *s, const void *u)
{
  int                 j;
  unsigned            h;
  unsigned            a, b, c;
  const char         *sp = (const char *) s;

  j = 0;
  h = 0;
  a = b = c = 0;
  for (;;) {
    if (*sp) {
      h += *sp++;
    }

    if (++j == 4) {
      a += h;
      h = 0;
    }
    else if (j == 8) {
      b += h;
      h = 0;
    }
    else if (j == 12) {
      c += h;
      sc_hash_mix (a, b, c);
      if (!*sp) {
        sc_hash_final (a, b, c);
        return c;
      }
      j = 0;
      h = 0;
    }
    else {
      h <<= 8;
    }
  }
}

size_t
sc_hash_memory_used (sc_hash_t * hash)
{
  return sizeof (sc_hash_t) +
    sc_array_memory_used (hash->slots, 1) +
    (hash->allocator_owned ? sc_mempool_memory_used (hash->allocator) : 0);
}

static const size_t sc_hash_minimal_size = (size_t) ((1 << 8) - 1);
static const size_t sc_hash_shrink_interval = (size_t) (1 << 8);

static void
sc_hash_maybe_resize (sc_hash_t * hash)
{
  size_t              i, j;
  size_t              new_size, new_count;
  sc_list_t          *old_list, *new_list;
  sc_link_t          *lynk, *temp;
  sc_array_t         *new_slots;
  sc_array_t         *old_slots = hash->slots;

  SC_ASSERT (old_slots->elem_count > 0);

  ++hash->resize_checks;
  if (hash->elem_count >= 4 * old_slots->elem_count) {
    new_size = 4 * old_slots->elem_count - 1;
  }
  else if (hash->elem_count <= old_slots->elem_count / 4) {
    new_size = old_slots->elem_count / 4 + 1;
    if (new_size < sc_hash_minimal_size) {
      return;
    }
  }
  else {
    return;
  }
  ++hash->resize_actions;

  /* allocate new slot array */
  new_slots = sc_array_new (sizeof (sc_list_t));
  sc_array_resize (new_slots, new_size);
  for (i = 0; i < new_size; ++i) {
    new_list = (sc_list_t *) sc_array_index (new_slots, i);
    sc_list_init (new_list, hash->allocator);
  }

  /* go through the old slots and move data to the new slots */
  new_count = 0;
  for (i = 0; i < old_slots->elem_count; ++i) {
    old_list = (sc_list_t *) sc_array_index (old_slots, i);
    lynk = old_list->first;
    while (lynk != NULL) {
      /* insert data into new slot list */
      j = hash->hash_fn (lynk->data, hash->user_data) % new_size;
      new_list = (sc_list_t *) sc_array_index (new_slots, j);
      sc_list_prepend (new_list, lynk->data);
      ++new_count;

      /* remove old list element */
      temp = lynk->next;
      sc_mempool_free (old_list->allocator, lynk);
      lynk = temp;
      --old_list->elem_count;
    }
    SC_ASSERT (old_list->elem_count == 0);
    old_list->first = old_list->last = NULL;
  }
  SC_ASSERT (new_count == hash->elem_count);

  /* replace old slots by new slots */
  sc_array_destroy (old_slots);
  hash->slots = new_slots;
}

sc_hash_t          *
sc_hash_new (sc_hash_function_t hash_fn, sc_equal_function_t equal_fn,
             void *user_data, sc_mempool_t * allocator)
{
  size_t              i;
  sc_hash_t          *hash;
  sc_list_t          *list;
  sc_array_t         *slots;

  hash = SC_ALLOC (sc_hash_t, 1);

  if (allocator != NULL) {
    SC_ASSERT (allocator->elem_size == sizeof (sc_link_t));
    hash->allocator = allocator;
    hash->allocator_owned = 0;
  }
  else {
    hash->allocator = sc_mempool_new (sizeof (sc_link_t));
    hash->allocator_owned = 1;
  }

  hash->elem_count = 0;
  hash->resize_checks = 0;
  hash->resize_actions = 0;
  hash->hash_fn = hash_fn;
  hash->equal_fn = equal_fn;
  hash->user_data = user_data;

  hash->slots = slots = sc_array_new (sizeof (sc_list_t));
  sc_array_resize (slots, sc_hash_minimal_size);
  for (i = 0; i < slots->elem_count; ++i) {
    list = (sc_list_t *) sc_array_index (slots, i);
    sc_list_init (list, hash->allocator);
  }

  return hash;
}

void
sc_hash_destroy (sc_hash_t * hash)
{
  if (hash->allocator_owned) {
    /* in this case we don't need to clean up each list separately: O(1) */
    sc_mempool_destroy (hash->allocator);
  }
  else {
    /* return all list elements to the allocator: requires O(N) */
    sc_hash_truncate (hash);
  }
  sc_array_destroy (hash->slots);

  SC_FREE (hash);
}

void
sc_hash_truncate (sc_hash_t * hash)
{
  size_t              i;
  size_t              count;
  sc_list_t          *list;
  sc_array_t         *slots = hash->slots;

  if (hash->elem_count == 0) {
    return;
  }

  if (hash->allocator_owned) {
    sc_hash_unlink (hash);
    sc_mempool_truncate (hash->allocator);
    return;
  }

  /* return all list elements to the outside memory allocator */
  for (i = 0, count = 0; i < slots->elem_count; ++i) {
    list = (sc_list_t *) sc_array_index (slots, i);
    count += list->elem_count;
    sc_list_reset (list);
  }
  SC_ASSERT (count == hash->elem_count);

  hash->elem_count = 0;
}

void
sc_hash_unlink (sc_hash_t * hash)
{
  size_t              i, count;
  sc_list_t          *list;
  sc_array_t         *slots = hash->slots;

  for (i = 0, count = 0; i < slots->elem_count; ++i) {
    list = (sc_list_t *) sc_array_index (slots, i);
    count += list->elem_count;
    sc_list_unlink (list);
  }
  SC_ASSERT (count == hash->elem_count);

  hash->elem_count = 0;
}

void
sc_hash_unlink_destroy (sc_hash_t * hash)
{
  if (hash->allocator_owned) {
    sc_mempool_destroy (hash->allocator);
  }
  sc_array_destroy (hash->slots);

  SC_FREE (hash);
}

int
sc_hash_lookup (sc_hash_t * hash, void *v, void ***found)
{
  size_t              hval;
  sc_list_t          *list;
  sc_link_t          *lynk;

  hval = hash->hash_fn (v, hash->user_data) % hash->slots->elem_count;
  list = (sc_list_t *) sc_array_index (hash->slots, hval);

  for (lynk = list->first; lynk != NULL; lynk = lynk->next) {
    /* check if an equal object is contained in the hash table */
    if (hash->equal_fn (lynk->data, v, hash->user_data)) {
      if (found != NULL) {
        *found = &lynk->data;
      }
      return 1;
    }
  }
  return 0;
}

int
sc_hash_insert_unique (sc_hash_t * hash, void *v, void ***found)
{
  int                 found_again;
  size_t              hval;
  sc_list_t          *list;
  sc_link_t          *lynk;

  hval = hash->hash_fn (v, hash->user_data) % hash->slots->elem_count;
  list = (sc_list_t *) sc_array_index (hash->slots, hval);

  /* check if an equal object is already contained in the hash table */
  for (lynk = list->first; lynk != NULL; lynk = lynk->next) {
    if (hash->equal_fn (lynk->data, v, hash->user_data)) {
      if (found != NULL) {
        *found = &lynk->data;
      }
      return 0;
    }
  }

  /* append new object to the list */
  sc_list_append (list, v);
  if (found != NULL) {
    *found = &list->last->data;
  }
  ++hash->elem_count;

  /* check for resize at specific intervals and reassign output */
  if (hash->elem_count % hash->slots->elem_count == 0) {
    sc_hash_maybe_resize (hash);
    if (found != NULL) {
      found_again = sc_hash_lookup (hash, v, found);
      SC_ASSERT (found_again);
    }
  }

  return 1;
}

int
sc_hash_remove (sc_hash_t * hash, void *v, void **found)
{
  size_t              hval;
  sc_list_t          *list;
  sc_link_t          *lynk, *prev;

  hval = hash->hash_fn (v, hash->user_data) % hash->slots->elem_count;
  list = (sc_list_t *) sc_array_index (hash->slots, hval);

  prev = NULL;
  for (lynk = list->first; lynk != NULL; lynk = lynk->next) {
    /* check if an equal object is contained in the hash table */
    if (hash->equal_fn (lynk->data, v, hash->user_data)) {
      if (found != NULL) {
        *found = lynk->data;
      }
      (void) sc_list_remove (list, prev);
      --hash->elem_count;

      /* check for resize at specific intervals and return */
      if (hash->elem_count % sc_hash_shrink_interval == 0) {
        sc_hash_maybe_resize (hash);
      }
      return 1;
    }
    prev = lynk;
  }
  return 0;
}

void
sc_hash_foreach (sc_hash_t * hash, sc_hash_foreach_t fn)
{
  size_t              slot;
  sc_list_t          *list;
  sc_link_t          *lynk;

  for (slot = 0; slot < hash->slots->elem_count; ++slot) {
    list = (sc_list_t *) sc_array_index (hash->slots, slot);
    for (lynk = list->first; lynk != NULL; lynk = lynk->next) {
      if (!fn (&lynk->data, hash->user_data)) {
        return;
      }
    }
  }
}

void
sc_hash_print_statistics (int package_id, int log_priority, sc_hash_t * hash)
{
  size_t              i;
  double              a, sum, squaresum;
  double              divide, avg, sqr, std;
  sc_list_t          *list;
  sc_array_t         *slots = hash->slots;

  sum = 0.;
  squaresum = 0.;
  for (i = 0; i < slots->elem_count; ++i) {
    list = (sc_list_t *) sc_array_index (slots, i);
    a = (double) list->elem_count;
    sum += a;
    squaresum += a * a;
  }
  SC_ASSERT ((size_t) sum == hash->elem_count);

  divide = (double) slots->elem_count;
  avg = sum / divide;
  sqr = squaresum / divide - avg * avg;
  std = sqrt (sqr);
  SC_GEN_LOGF (package_id, SC_LC_NORMAL, log_priority,
               "Hash size %lu avg %.3g std %.3g checks %lu %lu\n",
               (unsigned long) slots->elem_count, avg, std,
               (unsigned long) hash->resize_checks,
               (unsigned long) hash->resize_actions);
}

/* hash array routines */

size_t
sc_hash_array_memory_used (sc_hash_array_t * ha)
{
  return sizeof (sc_hash_array_t) +
    sc_array_memory_used (&ha->a, 0) + sc_hash_memory_used (ha->h);
}

static unsigned
sc_hash_array_hash_fn (const void *v, const void *u)
{
  const sc_hash_array_data_t *internal_data =
    (const sc_hash_array_data_t *) u;
  long                l = (long) v;
  void               *p;

  p = (l == -1L) ? internal_data->current_item :
    sc_array_index_long (internal_data->pa, l);

  return internal_data->hash_fn (p, internal_data->user_data);
}

static int
sc_hash_array_equal_fn (const void *v1, const void *v2, const void *u)
{
  const sc_hash_array_data_t *internal_data =
    (const sc_hash_array_data_t *) u;
  long                l1 = (long) v1;
  long                l2 = (long) v2;
  void               *p1, *p2;

  p1 = (l1 == -1L) ? internal_data->current_item :
    sc_array_index_long (internal_data->pa, l1);
  p2 = (l2 == -1L) ? internal_data->current_item :
    sc_array_index_long (internal_data->pa, l2);

  return internal_data->equal_fn (p1, p2, internal_data->user_data);
}

sc_hash_array_t    *
sc_hash_array_new (size_t elem_size, sc_hash_function_t hash_fn,
                   sc_equal_function_t equal_fn, void *user_data)
{
  sc_hash_array_t    *hash_array;

  hash_array = SC_ALLOC (sc_hash_array_t, 1);

  sc_array_init (&hash_array->a, elem_size);
  hash_array->internal_data.pa = &hash_array->a;
  hash_array->internal_data.hash_fn = hash_fn;
  hash_array->internal_data.equal_fn = equal_fn;
  hash_array->internal_data.user_data = user_data;
  hash_array->internal_data.current_item = NULL;
  hash_array->h = sc_hash_new (sc_hash_array_hash_fn, sc_hash_array_equal_fn,
                               &hash_array->internal_data, NULL);

  return hash_array;
}

void
sc_hash_array_destroy (sc_hash_array_t * hash_array)
{
  sc_hash_destroy (hash_array->h);
  sc_array_reset (&hash_array->a);

  SC_FREE (hash_array);
}

int
sc_hash_array_is_valid (sc_hash_array_t * hash_array)
{
  int                 found;
  size_t              zz, position;
  void               *v;

  for (zz = 0; zz < hash_array->a.elem_count; ++zz) {
    v = sc_array_index (&hash_array->a, zz);
    found = sc_hash_array_lookup (hash_array, v, &position);
    if (!found || position != zz) {
      return 0;
    }
  }

  return 1;
}

void
sc_hash_array_truncate (sc_hash_array_t * hash_array)
{
  sc_hash_truncate (hash_array->h);
  sc_array_reset (&hash_array->a);
}

int
sc_hash_array_lookup (sc_hash_array_t * hash_array, void *v,
                      size_t * position)
{
  int                 found;
  void              **found_void;

  hash_array->internal_data.current_item = v;
  found = sc_hash_lookup (hash_array->h, (void *) (-1L), &found_void);
  hash_array->internal_data.current_item = NULL;

  if (found) {
    if (position != NULL) {
      *position = (size_t) (*found_void);
    }
    return 1;
  }
  else {
    return 0;
  }
}

void               *
sc_hash_array_insert_unique (sc_hash_array_t * hash_array, void *v,
                             size_t * position)
{
  int                 added;
  void              **found_void;

  SC_ASSERT (hash_array->a.elem_count == hash_array->h->elem_count);

  hash_array->internal_data.current_item = v;
  added = sc_hash_insert_unique (hash_array->h, (void *) (-1L), &found_void);
  hash_array->internal_data.current_item = NULL;

  if (added) {
    if (position != NULL) {
      *position = hash_array->a.elem_count;
    }
    *found_void = (void *) hash_array->a.elem_count;
    return sc_array_push (&hash_array->a);
  }
  else {
    if (position != NULL) {
      *position = (size_t) (*found_void);
    }
    return NULL;
  }
}

void
sc_hash_array_rip (sc_hash_array_t * hash_array, sc_array_t * rip)
{
  sc_hash_destroy (hash_array->h);
  memcpy (rip, &hash_array->a, sizeof (sc_array_t));

  SC_FREE (hash_array);
}

void
sc_recycle_array_init (sc_recycle_array_t * rec_array, size_t elem_size)
{
  sc_array_init (&rec_array->a, elem_size);
  sc_array_init (&rec_array->f, sizeof (size_t));

  rec_array->elem_count = 0;
}

void
sc_recycle_array_reset (sc_recycle_array_t * rec_array)
{
  SC_ASSERT (rec_array->a.elem_count ==
             rec_array->elem_count + rec_array->f.elem_count);

  sc_array_reset (&rec_array->a);
  sc_array_reset (&rec_array->f);

  rec_array->elem_count = 0;
}

void               *
sc_recycle_array_insert (sc_recycle_array_t * rec_array, size_t * position)
{
  size_t              newpos;
  void               *newitem;

  if (rec_array->f.elem_count > 0) {
    newpos = *(size_t *) sc_array_pop (&rec_array->f);
    newitem = sc_array_index (&rec_array->a, newpos);
  }
  else {
    newpos = rec_array->a.elem_count;
    newitem = sc_array_push (&rec_array->a);
  }

  if (position != NULL) {
    *position = newpos;
  }
  ++rec_array->elem_count;

  return newitem;
}

void               *
sc_recycle_array_remove (sc_recycle_array_t * rec_array, size_t position)
{
  SC_ASSERT (rec_array->elem_count > 0);

  *(size_t *) sc_array_push (&rec_array->f) = position;
  --rec_array->elem_count;

  return sc_array_index (&rec_array->a, position);
}
