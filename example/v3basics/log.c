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

#include <sc3_log.h>

static void
main_exit_failure (const char *prefix, sc3_error_t * e)
{
  char                flatmsg[SC3_BUFSIZE];

  sc3_error_destroy_noerr (&e, flatmsg);
  fprintf (stderr, "%s: %s\n", prefix, flatmsg);

  exit (EXIT_FAILURE);
}

static void
main_report_leak (const char *prefix, sc3_error_t ** e)
{
  char                flatmsg[SC3_BUFSIZE];

  sc3_error_destroy_noerr (e, flatmsg);
  fprintf (stderr, "%s: %s\n", prefix, flatmsg);
}

static sc3_error_t *
main_init_allocator (sc3_allocator_t ** alloc, int align)
{
  SC3E (sc3_allocator_new (sc3_allocator_nothread (), alloc));
  SC3E (sc3_allocator_set_align (*alloc, align));
  SC3E (sc3_allocator_setup (*alloc));
  return NULL;
}

static sc3_error_t *
main_init_log (sc3_log_t ** log, sc3_allocator_t * alloc, int indent)
{
  SC3E (sc3_log_new (alloc, log));
  SC3E (sc3_log_set_level (*log, SC3_LOG_INFO));
  SC3E (sc3_log_set_comm (*log, SC3_MPI_COMM_WORLD));
  SC3E (sc3_log_set_indent (*log, indent));
  SC3E (sc3_log_setup (*log));
  return NULL;
}

static sc3_error_t *
main_init (int *pargc, char ***pargv,
           sc3_allocator_t ** alloc, sc3_log_t ** log)
{
  SC3E (main_init_allocator (alloc, 16));
  SC3E (main_init_log (log, *alloc, 3));
  sc3_log (*log, 0, SC3_LOG_PROCESS0, SC3_LOG_TOP, "Leave main_init");
  return NULL;
}

static sc3_error_t *
main_work (sc3_allocator_t * alloc, sc3_log_t * log)
{
  sc3_log (log, 0, SC3_LOG_PROCESS0, SC3_LOG_TOP, "In main_work");
  return NULL;
}

static sc3_error_t *
main_finalize (sc3_allocator_t ** alloc, sc3_log_t ** log)
{
  sc3_error_t *leak = NULL;

  sc3_log (*log, 0, SC3_LOG_PROCESS0, SC3_LOG_TOP, "Enter main_finalize");

  sc3_log_ref (*log);

  /* if we find any leaks, propagate them to the outside */
  SC3L (&leak, sc3_log_destroy (log));
  SC3L (&leak, sc3_allocator_destroy (alloc));
  return leak;
}

int
main (int argc, char **argv)
{
  sc3_allocator_t    *alloc;
  sc3_error_t        *e;
  sc3_log_t          *log;

  /* Initialize MPI.  This is representative of any external startup code */
  if ((e = sc3_MPI_Init (&argc, &argv)) != NULL) {
    main_exit_failure ("Main init", e);
  }
  /* Note we'll be relying below on the fact that e is NULL. */

  /* Initialization of toplevel allocator and logger.
     This is representative of entering sc3 code from any larger program */
  SC3E_NULL_SET (e, main_init (&argc, &argv, &alloc, &log));

  /* This is representative of calling sc3 code from any larger program */
  SC3E_NULL_SET (e, main_work (alloc, log));

  /* Free toplevel allocator and logger.
     This is representative of leaving sc3 code from any larger program */
  SC3E_NULL_SET (e, main_finalize (&alloc, &log));
  /* TODO this turns leaks into fatal errors */

  /* As a bonus, treat leaks separately */
  if (sc3_error_is_leak (e, NULL)) {
    main_report_leak ("Main report", &e);
  }

  /* Finalize MPI.  This is representative of any external cleanup code */
  if (e != NULL || (e = sc3_MPI_Finalize ()) != NULL) {
    main_exit_failure ("Main finalize", e);
  }
  return 0;
}
