/*
  This file is part of the SC Library, version 3.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

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

#include <sc3_error.h>

#if 0
#define SC3_BASICS_DEALLOCATE
#endif

static int
unravel_error (sc3_error_t ** e)
{
  int                 j;
  int                 num_weird;
  int                 line;
  const char         *filename, *errmsg;

  if (e == NULL || *e == NULL) {
    return 1;
  }

  num_weird = 0;
  for (j = 0; *e != NULL; num_weird += sc3_error_pop (e) ? 1 : 0, ++j) {
    sc3_error_get_location (*e, &filename, &line);
    sc3_error_get_message (*e, &errmsg);
    printf ("Error stack %d:%s:%d: %s\n", j, filename, line, errmsg);
  }

  return num_weird;
}

static sc3_error_t *
child_function (int a, int *result)
{
  SC3E_RETVAL (result, 0);
  SC3A_CHECK (a < 50);

  *result = a + 1;
  return NULL;
}

static sc3_error_t *
parent_function (int a, int *result)
{
  SC3E_RETVAL (result, 0);
  SC3A_CHECK (a < 100);
  SC3E (child_function (a, result));

  *result *= 3;
  return NULL;
}

static sc3_error_t *
io_error (sc3_allocator_t * a,
          const char *filename, int line, const char *errmsg)
{
  sc3_error_args_t   *ea;
  sc3_error_t        *e;

  SC3E (sc3_error_args_new (a, &ea));
  SC3E (sc3_error_args_set_location (ea, filename, line));
  SC3E (sc3_error_args_set_message (ea, errmsg));
  SC3E (sc3_error_args_set_severity (ea, SC3_ERROR_RUNTIME));
  SC3E (sc3_error_new (&ea, &e));

  return e;
}

#define SC3_BASICS_IO_ERROR(a,m) (io_error (a, __FILE__, __LINE__, m))

static sc3_error_t *
run_io (sc3_allocator_t * a, int result)
{
  FILE               *file;

  if ((file = fopen ("sc3_basics_run_io.txt", "wb")) == NULL) {
    return SC3_BASICS_IO_ERROR (a, "File open failed");
  }
  if (fprintf (file, "Hello world from sc3_basics %d\n", result) < 0) {
    (void) fclose (file);
    return SC3_BASICS_IO_ERROR (a, "File fprintf failed");
  }
  if (fclose (file)) {
    return SC3_BASICS_IO_ERROR (a, "File close failed");
  }

  return NULL;
}

static sc3_error_t *
run_prog (sc3_allocator_t * origa, int input, int *result, int *num_io)
{
  sc3_error_t        *e;
  sc3_error_args_t   *ea;
  sc3_allocator_t    *a;
  sc3_allocator_args_t *aa;

  /* Test assertions */
  SC3E (parent_function (input, result));
  SC3A_CHECK (num_io != NULL);

  /* Make allocator for this context block */
  SC3E (sc3_allocator_args_new (origa, &aa));
  SC3E (sc3_allocator_new (&aa, &a));

  /* Test file input/output and recoverable errors */
  if ((e = run_io (a, *result)) != NULL) {
    SC3E_DEMAND (!sc3_error_is_fatal (e));
    ++*num_io;

#ifdef SC3_BASICS_DEALLOCATE
    /* do something with the runtime error */
    unravel_error (&e);

    /* return a new error to the outside */
    SC3E (sc3_error_args_new (origa, &ea));
    SC3E (sc3_error_args_set_location (ea, __FILE__, __LINE__));
    SC3E (sc3_error_args_set_message (ea, "Encountered I/O error"));
    SC3E (sc3_error_args_set_severity (ea, SC3_ERROR_RUNTIME));
    SC3E (sc3_error_new (&ea, &e));
#else
    /* return the original error to the outside */
    SC3E (sc3_error_args_new (origa, &ea));
    SC3E (sc3_error_args_set_location (ea, __FILE__, __LINE__));
    SC3E (sc3_error_args_set_message (ea, "Encountered I/O error"));
    SC3E (sc3_error_args_set_severity (ea, SC3_ERROR_RUNTIME));
    SC3E (sc3_error_args_set_stack (ea, &e));
    SC3E (sc3_error_new (&ea, &e));
#endif
  }

  /* If we return before here, we will never destroy the allocator.
     This is ok if we only do this on fatal errors. */

  /* TODO: If e is set and we return a new fatal error, we will never
     report on e, even if e is inderectly responsible for the error. */

#ifdef SC3_BASICS_DEALLOCATE
  /* The allocator is now done.
     Must not pass any allocated objects to the outside of this function. */
  SC3E (sc3_allocator_destroy (&a));
#else
  /* We allow allocated objects to be passed to the outside.
     These carry references to this allocator beyond this scope. */
  SC3E (sc3_allocator_unref (&a));
#endif

  /* Make sure not to mess with this error variable in between */
  return e;
}

static int
main_error_check (sc3_error_t ** ep, int *num_fatal, int *num_weird)
{
  if (num_fatal == NULL || num_weird == NULL) {
    return -1;
  }

  if (ep != NULL && *ep != NULL) {
    if (sc3_error_is_fatal (*ep))
      ++ * num_fatal;

    /* unravel error stack and print messages */
    *num_weird += unravel_error (ep);
    return -1;
  }
  return 0;
}

int
main (int argc, char **argv)
{
  const int           inputs[3] = { 167, 84, 23 };
  int                 input;
  int                 result;
  int                 num_fatal, num_weird, num_io;
  int                 i;
  sc3_error_t        *e;
  sc3_allocator_t    *a;
  sc3_allocator_args_t *aa;

  num_fatal = num_weird = num_io = 0;

  SC3E_SET (e, sc3_allocator_args_new (NULL, &aa));
  SC3E_NULL_SET (e, sc3_allocator_new (&aa, &a));
  if (main_error_check (&e, &num_fatal, &num_weird)) {
    goto main_end;
  }

  for (i = 0; i < 3; ++i) {
    input = inputs[i];
    SC3E_SET (e, run_prog (a, input, &result, &num_io));
    if (!main_error_check (&e, &num_fatal, &num_weird)) {
      printf ("Clean execution with input %d result %d\n", input, result);
    }
  }

  SC3E_SET (e, sc3_allocator_destroy (&a));
  (void) main_error_check (&e, &num_fatal, &num_weird);

main_end:
  printf ("Fatal errors %d weird %d IO %d\n", num_fatal, num_weird, num_io);

  return EXIT_SUCCESS;
}
