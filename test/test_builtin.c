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

/* this test checks the possibly builtin third-party libraries */

/* sc.h comes first in every compilation unit */
#include <sc.h>
#include <sc_lua.h>
#include <sc_zlib.h>

static int
test_getopt (int argc, char **argv)
{
  int                 aflag, anint;
  int                 copt, lindex;
  const struct option sopt[] = {
    {"add", 1, 0, 0},
    {"append", 0, 0, 0},
    {"delete", 1, 0, 0},
    {"verbose", 0, 0, 0},
    {"create", 1, 0, 'c'},
    {"file", 1, 0, 0},
    {0, 0, 0, 0}
  };

  aflag = 0;
  anint = 0;
  while ((copt = getopt_long (argc, argv, "cnt:", sopt, &lindex)) != -1) {
    switch (copt) {
    case 0:
      printf ("option %s", sopt[lindex].name);
      if (optarg)
        printf (" with arg %s", optarg);
      printf ("\n");
      break;
    case 'c':
      break;
    case 'n':
      aflag = 1;
      break;
    case 't':
      anint = atoi (optarg);
      break;
    default:
      fprintf (stderr, "Usage: %s [-t integer] [-n] [-c]\n", argv[0]);
      return 1;
    }
  }
  return 0;
}

static int
test_obstack (void)
{
  void               *mem;
  struct obstack      obst;
  /*@ignore@ */
  static void        *(*obstack_chunk_alloc) (size_t) = malloc;
  static void         (*obstack_chunk_free) (void *) = free;
  /*@end@ */

  obstack_init (&obst);
  mem = obstack_alloc (&obst, 47);
  mem = obstack_alloc (&obst, 47135);
  mem = obstack_alloc (&obst, 473);
  obstack_free (&obst, NULL);

  return 0;
}

static int
test_zlib (void)
{
  const char          b1[] = "This is one string";
  const char          b2[] = "This is another string";
  const size_t        l1 = strlen (b1);
  const size_t        l2 = strlen (b2);
  char                b3[BUFSIZ];
  uLong               adler0, adler1, adler2, adler3a, adler3b;

  adler0 = adler32 (0L, Z_NULL, 0);
  adler1 = adler32 (adler0, (const Bytef *) b1, l1);
  adler2 = adler32 (adler0, (const Bytef *) b2, l2);
  adler3a = adler32_combine (adler1, adler2, l2);

  snprintf (b3, BUFSIZ, "%s%s", b1, b2);
  adler3b = adler32 (adler0, (const Bytef *) b3, l1 + l2);

  return adler3a != adler3b;
}

static int
test_lua (void)
{
  lua_State          *L = lua_open ();

  lua_close (L);

  return 0;
}

int
main (int argc, char **argv)
{
  int                 num_errors = 0;

  sc_init (MPI_COMM_NULL, true, true, NULL, SC_LP_DEFAULT);

  num_errors += test_getopt (argc, argv);
  num_errors += test_obstack ();
  num_errors += test_zlib ();
  num_errors += test_lua ();

  sc_finalize ();

  return num_errors > 0;
}
