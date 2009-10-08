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

  int                 mpiret;

  sc_keyvalue_t      *args;
  sc_keyvalue_t      *args2;

  const char         *dummy = "I am a dummy string";
  const char         *wrong = "I am the wrong string";
  const char         *again = "Try this all over again";

  int                 intTest;
  double              doubleTest;
  const char         *stringTest;
  void               *pointerTest;

  /* Initialization stuff */
  mpiret = MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

  /* Create a new argument set */
  args = sc_keyvalue_newf (0,
                           "i:intTest", -17,
                           "g:doubleTest", 3.14159,
                           "s:stringTest", "Hello Test!",
                           "p:pointerTest", (void *) dummy, NULL);

  intTest = sc_keyvalue_get_int (args, "intTest", 0);
  doubleTest = sc_keyvalue_get_double (args, "doubleTest", 0.0);
  stringTest = sc_keyvalue_get_string (args, "stringTest", wrong);
  pointerTest = sc_keyvalue_get_pointer (args, "pointerTest", NULL);

  if (intTest != -17) {
    SC_VERBOSE ("Test 1 failure on int\n");
    num_failed_tests++;
  }
  if (doubleTest != 3.14159) {
    SC_VERBOSE ("Test 1 failure on double\n");
    num_failed_tests++;
  }
  if (strcmp (stringTest, "Hello Test!")) {
    SC_VERBOSE ("Test 1 failure on string\n");
    num_failed_tests++;
  }
  if (pointerTest != (void *) dummy) {
    SC_VERBOSE ("Test 1 failure on pointer\n");
    num_failed_tests++;
  }

  sc_keyvalue_destroy (args);
  args = NULL;

  /* Create a new argument set using the sc_keyvalue_set functions */
  args2 = sc_keyvalue_new ();

  sc_keyvalue_set_int (args2, "intTest", -17);
  sc_keyvalue_set_double (args2, "doubleTest", 3.14159);
  sc_keyvalue_set_string (args2, "stringTest", "Hello Test!");
  sc_keyvalue_set_pointer (args2, "pointerTest", (void *) dummy);

  /* Direct verification that these objects now exist */
  if (sc_keyvalue_exists (args2, "intTest") != SC_KEYVALUE_ENTRY_INT) {
    SC_VERBOSE ("Test exist failure on int\n");
    num_failed_tests++;
  }
  if (sc_keyvalue_exists (args2, "doubleTest") != SC_KEYVALUE_ENTRY_DOUBLE) {
    SC_VERBOSE ("Test exist failure on double\n");
    num_failed_tests++;
  }
  if (sc_keyvalue_exists (args2, "stringTest") != SC_KEYVALUE_ENTRY_STRING) {
    SC_VERBOSE ("Test exist failure on string\n");
    num_failed_tests++;
  }
  if (sc_keyvalue_exists (args2, "pointerTest") != SC_KEYVALUE_ENTRY_POINTER) {
    SC_VERBOSE ("Test exist failure on pointer\n");
    num_failed_tests++;
  }

  intTest = sc_keyvalue_get_int (args2, "intTest", 0);
  doubleTest = sc_keyvalue_get_double (args2, "doubleTest", 0.0);
  stringTest = sc_keyvalue_get_string (args2, "stringTest", wrong);
  pointerTest = sc_keyvalue_get_pointer (args2, "pointerTest", NULL);

  if (intTest != -17) {
    SC_VERBOSE ("Test 2 failure on int\n");
    num_failed_tests++;
  }
  if (doubleTest != 3.14159) {
    SC_VERBOSE ("Test 2 failure on double\n");
    num_failed_tests++;
  }
  if (strcmp (stringTest, "Hello Test!")) {
    SC_VERBOSE ("Test 2 failure on string\n");
    num_failed_tests++;
  }
  if (pointerTest != (void *) dummy) {
    SC_VERBOSE ("Test 2 failure on pointer\n");
    num_failed_tests++;
  }

  /* Test the unset functionality */
  if (sc_keyvalue_unset (args2, "intTest") != SC_KEYVALUE_ENTRY_INT) {
    SC_VERBOSE ("Test unset failure on int\n");
    num_failed_tests++;
  }
  if (sc_keyvalue_unset (args2, "doubleTest") != SC_KEYVALUE_ENTRY_DOUBLE) {
    SC_VERBOSE ("Test unset failure on double\n");
    num_failed_tests++;
  }
  if (sc_keyvalue_unset (args2, "stringTest") != SC_KEYVALUE_ENTRY_STRING) {
    SC_VERBOSE ("Test unset failure on string\n");
    num_failed_tests++;
  }
  if (sc_keyvalue_unset (args2, "pointerTest") != SC_KEYVALUE_ENTRY_POINTER) {
    SC_VERBOSE ("Test unset failure on pointer\n");
    num_failed_tests++;
  }

  intTest = sc_keyvalue_get_int (args2, "intTest", 12);
  doubleTest = sc_keyvalue_get_double (args2, "doubleTest", 2.71828);
  stringTest =
    sc_keyvalue_get_string (args2, "stringTest", "Another test string?");
  pointerTest =
    sc_keyvalue_get_pointer (args2, "pointerTest", (void *) again);

  if (intTest != 12) {
    SC_VERBOSE ("Test 3 failure on int\n");
    num_failed_tests++;
  }
  if (doubleTest != 2.71828) {
    SC_VERBOSE ("Test 3 failure on double\n");
    num_failed_tests++;
  }
  if (strcmp (stringTest, "Another test string?")) {
    SC_VERBOSE ("Test 3 failure on string\n");
    num_failed_tests++;
  }
  if (pointerTest != again) {
    SC_VERBOSE ("Test 3 failure on pointer\n");
    num_failed_tests++;
  }

  /* Direct verification that these objects no longer exist */
  if (sc_keyvalue_exists (args2, "intTest")) {
    SC_VERBOSE ("Test 4 failure on int\n");
    num_failed_tests++;
  }
  if (sc_keyvalue_exists (args2, "doubleTest")) {
    SC_VERBOSE ("Test 4 failure on double\n");
    num_failed_tests++;
  }
  if (sc_keyvalue_exists (args2, "stringTest")) {
    SC_VERBOSE ("Test 4 failure on string\n");
    num_failed_tests++;
  }
  if (sc_keyvalue_exists (args2, "pointerTest")) {
    SC_VERBOSE ("Test 4 failure on pointer\n");
    num_failed_tests++;
  }

  /* Test empty cases for exists and unset */
  if (sc_keyvalue_exists (args2, "notakey") != SC_KEYVALUE_ENTRY_NONE) {
    SC_VERBOSE ("Test failure on nonexist 1\n");
    num_failed_tests++;
  }
  if (sc_keyvalue_unset (args2, "notanotherkey") != SC_KEYVALUE_ENTRY_NONE) {
    SC_VERBOSE ("Test failure on nonexist 2\n");
    num_failed_tests++;
  }

  sc_keyvalue_destroy (args2);

  /* Shutdown procedures */
  sc_finalize ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return num_failed_tests ? 1 : 0;
}
