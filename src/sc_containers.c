/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2007-2009 Carsten Burstedde, Lucas Wilcox.

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

/* sc.h comes first in every compilation unit */
#include <sc.h>
#include <sc_containers.h>
#include <zlib.h>

/* array routines */

sc_array_t         *
sc_array_new (size_t elem_size)
{
  sc_array_t         *array;

  array = SC_ALLOC_ZERO (sc_array_t, 1);

  sc_array_init (array, elem_size);

  return array;
}

sc_array_t         *
sc_array_new_view (sc_array_t * array, size_t offset, size_t length)
{
  sc_array_t         *view;

  SC_ASSERT (offset + length <= array->elem_count);

  view = sc_array_new (array->elem_size);
  view->elem_count = length;
  view->byte_alloc = -1;
  view->array = array->array + array->elem_size * offset;

  return view;
}

sc_array_t         *
sc_array_new_data (void *base, size_t elem_size, size_t elem_count)
{
  sc_array_t         *view;

  view = sc_array_new (elem_size);
  view->elem_count = elem_count;
  view->byte_alloc = -1;
  view->array = base;

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

  SC_ASSERT (SC_ARRAY_IS_OWNER (array));

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
sc_array_sort (sc_array_t * array, int (*compar) (const void *, const void *))
{
  qsort (array->array, array->elem_count, array->elem_size, compar);
}

bool
sc_array_is_sorted (sc_array_t * array,
                    int (*compar) (const void *, const void *))
{
  const size_t        count = array->elem_count;
  size_t              zz;
  void               *vold, *vnew;

  if (count <= 1) {
    return true;
  }

  vold = sc_array_index (array, 0);
  for (zz = 1; zz < count; ++zz) {
    vnew = sc_array_index (array, zz);
    if (compar (vold, vnew) > 0) {
      return false;
    }
    vold = vnew;
  }

  return true;
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

unsigned
sc_array_checksum (sc_array_t * array)
{
  uInt                bytes;
  uLong               crc;

  crc = adler32 (0L, Z_NULL, 0);
  if (array->elem_count == 0) {
    return (unsigned) crc;
  }

  bytes = (uInt) (array->elem_count * array->elem_size);
  crc = adler32 (crc, (const Bytef *) array->array, bytes);

  return (unsigned) crc;
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

  mempool = SC_ALLOC_ZERO (sc_mempool_t, 1);

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

sc_list_t          *
sc_list_new (sc_mempool_t * allocator)
{
  sc_list_t          *list;

  list = SC_ALLOC_ZERO (sc_list_t, 1);

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

  lynk = sc_mempool_alloc (list->allocator);
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

  lynk = sc_mempool_alloc (list->allocator);
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

  lynk = sc_mempool_alloc (list->allocator);
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
    new_list = sc_array_index (new_slots, i);
    sc_list_init (new_list, hash->allocator);
  }

  /* go through the old slots and move data to the new slots */
  new_count = 0;
  for (i = 0; i < old_slots->elem_count; ++i) {
    old_list = sc_array_index (old_slots, i);
    lynk = old_list->first;
    while (lynk != NULL) {
      /* insert data into new slot list */
      j = hash->hash_fn (lynk->data, hash->user_data) % new_size;
      new_list = sc_array_index (new_slots, j);
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

  hash = SC_ALLOC_ZERO (sc_hash_t, 1);

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
    list = sc_array_index (slots, i);
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
    list = sc_array_index (slots, i);
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
    list = sc_array_index (slots, i);
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

bool
sc_hash_lookup (sc_hash_t * hash, void *v, void ***found)
{
  size_t              hval;
  sc_list_t          *list;
  sc_link_t          *lynk;

  hval = hash->hash_fn (v, hash->user_data) % hash->slots->elem_count;
  list = sc_array_index (hash->slots, hval);

  for (lynk = list->first; lynk != NULL; lynk = lynk->next) {
    /* check if an equal object is contained in the hash table */
    if (hash->equal_fn (lynk->data, v, hash->user_data)) {
      if (found != NULL) {
        *found = &lynk->data;
      }
      return true;
    }
  }
  return false;
}

bool
sc_hash_insert_unique (sc_hash_t * hash, void *v, void ***found)
{
  bool                found_again;
  size_t              hval;
  sc_list_t          *list;
  sc_link_t          *lynk;

  hval = hash->hash_fn (v, hash->user_data) % hash->slots->elem_count;
  list = sc_array_index (hash->slots, hval);

  /* check if an equal object is already contained in the hash table */
  for (lynk = list->first; lynk != NULL; lynk = lynk->next) {
    if (hash->equal_fn (lynk->data, v, hash->user_data)) {
      if (found != NULL) {
        *found = &lynk->data;
      }
      return false;
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

  return true;
}

bool
sc_hash_remove (sc_hash_t * hash, void *v, void **found)
{
  size_t              hval;
  sc_list_t          *list;
  sc_link_t          *lynk, *prev;

  hval = hash->hash_fn (v, hash->user_data) % hash->slots->elem_count;
  list = sc_array_index (hash->slots, hval);

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
      return true;
    }
    prev = lynk;
  }
  return false;
}

void
sc_hash_foreach (sc_hash_t * hash, sc_hash_foreach_t fn)
{
  size_t              slot;
  sc_list_t          *list;
  sc_link_t          *lynk;

  for (slot = 0; slot < hash->slots->elem_count; ++slot) {
    list = sc_array_index (hash->slots, slot);
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
    list = sc_array_index (slots, i);
    a = (double) list->elem_count;
    sum += a;
    squaresum += a * a;
  }
  SC_ASSERT ((size_t) sum == hash->elem_count);

  divide = (double) slots->elem_count;
  avg = sum / divide;
  sqr = squaresum / divide - avg * avg;
  std = sqrt (sqr);
  SC_LOGF (package_id, SC_LC_NORMAL, log_priority,
           "Hash size %lu avg %.3g std %.3g checks %lu %lu\n",
           (unsigned long) slots->elem_count, avg, std,
           (unsigned long) hash->resize_checks,
           (unsigned long) hash->resize_actions);
}

static unsigned
sc_hash_array_hash_fn (const void *v, const void *u)
{
  const sc_hash_array_data_t *internal_data = u;
  long                l = (long) v;
  void               *p;

  p = (l == -1L) ? internal_data->current_item :
    sc_array_index_long (internal_data->pa, l);

  return internal_data->hash_fn (p, internal_data->user_data);
}

static              bool
sc_hash_array_equal_fn (const void *v1, const void *v2, const void *u)
{
  const sc_hash_array_data_t *internal_data = u;
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

  hash_array = SC_ALLOC_ZERO (sc_hash_array_t, 1);

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

bool
sc_hash_array_is_valid (sc_hash_array_t * hash_array)
{
  bool                found;
  size_t              zz, position;
  void               *v;

  for (zz = 0; zz < hash_array->a.elem_count; ++zz) {
    v = sc_array_index (&hash_array->a, zz);
    found = sc_hash_array_lookup (hash_array, v, &position);
    if (!found || position != zz) {
      return false;
    }
  }

  return true;
}

void
sc_hash_array_truncate (sc_hash_array_t * hash_array)
{
  sc_hash_truncate (hash_array->h);
  sc_array_reset (&hash_array->a);
}

bool
sc_hash_array_lookup (sc_hash_array_t * hash_array, void *v,
                      size_t * position)
{
  bool                found;
  void              **found_void;

  hash_array->internal_data.current_item = v;
  found = sc_hash_lookup (hash_array->h, (void *) (-1L), &found_void);
  hash_array->internal_data.current_item = NULL;

  if (found) {
    if (position != NULL) {
      *position = (size_t) (*found_void);
    }
    return true;
  }
  else {
    return false;
  }
}

void               *
sc_hash_array_insert_unique (sc_hash_array_t * hash_array, void *v,
                             size_t * position)
{
  bool                added;
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
