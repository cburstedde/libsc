/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

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

#include <sc_keyvalue.h>

int
main (int argc, char **argv)
{
  int                 num_failed_tests = 0;

  sc_keyvalue_t      *args;
  sc_keyvalue_t      *args2;

  const char         *dummy = "I am a dummy string";

  int                 intTest;
  double              doubleTest;
  const char         *stringTest;
  void               *pointerTest;

  /* Create a new argument set */
  args = sc_keyvalue_new (0,
                          "i:intTest", -17,
                          "g:doubleTest", 3.14159,
                          "s:stringTest", "Hello Test!",
                          "p:pointerTest", (void *) dummy, NULL);

  intTest = sc_keyvalue_get_int (args, "intTest", 0);
  doubleTest = sc_keyvalue_get_double (args, "doubleTest", 0.0);
  stringTest = sc_keyvalue_get_string (args, "stringTest", NULL);
  pointerTest = sc_keyvalue_get_pointer (args, "pointerTest", NULL);

  if (intTest != -17)
    num_failed_tests++;

  if (doubleTest != 3.14159)
    num_failed_tests++;

  if (strcmp (stringTest, "Hello Test!"))
    num_failed_tests++;

  if (pointerTest != (void *) dummy)
    num_failed_tests++;

  sc_keyvalue_destroy (args);
  args = NULL;

  /* Create a new argument set using the sc_keyvalue_set functions */
  args2 = sc_keyvalue_new (0, NULL);

  sc_keyvalue_set_int (args2, "intTest", -17);
  sc_keyvalue_set_double (args2, "doubleTest", 3.14159);
  sc_keyvalue_set_string (args2, "stringTest", "Hello Test!");
  sc_keyvalue_set_pointer (args2, "pointerTest", (void *) dummy);

  /* Direct verification that these objects now exist */
  if (!sc_keyvalue_exist (args2, "intTest"))
    num_failed_tests++;
  if (!sc_keyvalue_exist (args2, "doubleTest"))
    num_failed_tests++;
  if (!sc_keyvalue_exist (args2, "stringTest"))
    num_failed_tests++;
  if (!sc_keyvalue_exist (args2, "pointerTest"))
    num_failed_tests++;

  intTest = sc_keyvalue_get_int (args2, "intTest", 0);
  doubleTest = sc_keyvalue_get_double (args2, "doubleTest", 0.0);
  stringTest = sc_keyvalue_get_string (args2, "stringTest", NULL);
  pointerTest = sc_keyvalue_get_pointer (args2, "pointerTest", NULL);

  if (intTest != -17)
    num_failed_tests++;

  if (doubleTest != 3.14159)
    num_failed_tests++;

  if (strcmp (stringTest, "Hello Test!"))
    num_failed_tests++;

  if (pointerTest != (void *) dummy)
    num_failed_tests++;

  /* Test the unset functionality */
  sc_keyvalue_unset (args2, "i:intTest");
  sc_keyvalue_unset (args2, "g:doubleTest");
  sc_keyvalue_unset (args2, "s:stringTest");
  sc_keyvalue_unset (args2, "p:pointerTest");

  intTest = sc_keyvalue_get_int (args2, "intTest", 12);
  doubleTest = sc_keyvalue_get_double (args2, "doubleTest", 2.71828);
  stringTest =
    sc_keyvalue_get_string (args2, "stringTest", "Another test string?");
  pointerTest =
    sc_keyvalue_get_pointer (args2, "pointerTest", (void *) (&main));

  if (intTest != 12)
    num_failed_tests++;

  if (doubleTest != 2.71828)
    num_failed_tests++;

  if (strcmp (stringTest, "Another test string?"))
    num_failed_tests++;

  if (pointerTest != (void *) (&main))
    num_failed_tests++;

  /* Direct verification that these objects no longer exist */
  if (sc_keyvalue_exist (args2, "intTest"))
    num_failed_tests++;
  if (sc_keyvalue_exist (args2, "doubleTest"))
    num_failed_tests++;
  if (sc_keyvalue_exist (args2, "stringTest"))
    num_failed_tests++;
  if (sc_keyvalue_exist (args2, "pointerTest"))
    num_failed_tests++;

  sc_keyvalue_destroy (args2);

  sc_finalize ();

  return num_failed_tests ? 1 : 0;
}
