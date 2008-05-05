/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2008 Carsten Burstedde, Lucas Wilcox.

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

/* *INDENT-OFF* */
const int sc_log_lookup_table[256] =
{ -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
   4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
   5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
   5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
   6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
   6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
   6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
   6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
};
/* *INDENT-ON* */

static int          malloc_count = 0;
static int          free_count = 0;

#if 0
/*@unused@*/

static void
test_printf (void)
{
  int64_t             i64 = 0;
  long                l = 0;
  size_t              s = 0;
  ssize_t             ss = 0;

  printf ("%lld %ld %zu %zd\n", (long long) i64, l, s, ss);
}
#endif /* 0 */

void               *
sc_malloc (size_t size)
{
  void               *ret;

  ret = malloc (size);

  if (size > 0) {
    SC_CHECK_ABORT (ret != NULL, "Allocation");
    ++malloc_count;
  }
  else {
    malloc_count += ((ret == NULL) ? 0 : 1);
  }

  return ret;
}

void               *
sc_calloc (size_t nmemb, size_t size)
{
  void               *ret;

  ret = calloc (nmemb, size);

  if (nmemb * size > 0) {
    SC_CHECK_ABORT (ret != NULL, "Allocation");
    ++malloc_count;
  }
  else {
    malloc_count += ((ret == NULL) ? 0 : 1);
  }

  return ret;
}

void               *
sc_realloc (void *ptr, size_t size)
{
  void               *ret;

  ret = realloc (ptr, size);

  if (ptr == NULL) {
    if (size > 0) {
      SC_CHECK_ABORT (ret != NULL, "Reallocation");
      ++malloc_count;
    }
    else {
      malloc_count += ((ret == NULL) ? 0 : 1);
    }
  }
  else {
    if (size > 0) {
      SC_CHECK_ABORT (ret != NULL, "Reallocation");
    }
    else {
      free_count += ((ret == NULL) ? 1 : 0);
    }
  }

  return ret;
}

char               *
sc_strdup (const char *s)
{
  size_t              len;
  char               *d;

  if (s == NULL) {
    return NULL;
  }

  len = strlen (s) + 1;
  d = sc_malloc (len);
  memcpy (d, s, len);

  return d;
}

void
sc_free (void *ptr)
{
  if (ptr != NULL) {
    ++free_count;
    free (ptr);
  }
}

void
sc_memory_check (void)
{
  SC_CHECK_ABORT (malloc_count == free_count, "Memory balance");
}

/* EOF sc.c */
