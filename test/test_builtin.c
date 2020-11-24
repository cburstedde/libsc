/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System
  Additional copyright (C) 2011 individual authors

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

/* this test checks the possibly builtin third-party libraries */

#include <sc_getopt.h>
#include <sc_obstack.h>

/* truthfully, the libraries below are not builtin anymore */
#include <sc_config.h>
#ifdef SC_HAVE_ZLIB
#include <zlib.h>
#endif
#ifdef SC_HAVE_LUA
#include <sc_lua.h>
#endif

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
  if (anint == 1234567) {
    fprintf (stderr, "Test with %d %d\n", aflag, anint);
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
  *(char *) mem = '\0';
  obstack_free (&obst, NULL);

  return 0;
}

#ifdef SC_HAVE_ZLIB

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

#endif /* SC_HAVE_ZLIB */

#ifdef SC_HAVE_LUA

static int
test_lua (void)
{
  lua_State          *L = luaL_newstate ();

  lua_close (L);

  return 0;
}

#endif /* SC_HAVE_LUA */

int
main (int argc, char **argv)
{
  int                 num_errors = 0;

  sc_init (sc_MPI_COMM_NULL, 1, 1, NULL, SC_LP_DEFAULT);

  num_errors += test_getopt (argc, argv);
  num_errors += test_obstack ();
#ifdef SC_HAVE_ZLIB
  num_errors += test_zlib ();
#endif
#ifdef SC_HAVE_LUA
  num_errors += test_lua ();
#endif

  sc_finalize ();

  return num_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}
