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
#include <stdarg.h>

__attribute__ ((format (printf, 2, 3)))
static int
main_fprintf (FILE *file, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);

  /* example of custom log function */
  if (fputs ("sc3_log ", file) < 0) {
    return -1;
  }
  return vfprintf (file, format, ap);

#if 0
  /* unneeded */
  va_end (ap);
  return 0;
#endif
}

static void
main_exit_failure (sc3_error_t **e, const char *prefix)
{
  char                flatmsg[SC3_BUFSIZE];

  sc3_error_destroy_noerr (e, flatmsg);
  fprintf (stderr, "%s: %s\n", prefix, flatmsg);

  exit (EXIT_FAILURE);
}

static int
work_error (sc3_error_t **e, sc3_log_t *log, const char *prefix)
{
  char                flatmsg[SC3_BUFSIZE];

  /* bad call convention is reported and accepted */
  if (e == NULL || *e == NULL) {
    sc3_logf (log, 0, SC3_LOG_THREAD0, SC3_LOG_ERROR,
              "NULL error: %s", prefix);
    return 0;
  }

  /* if needed, treat user/recoverable errors here */

  /* leak error we just report and then continue */
  if (sc3_error_is_leak (*e, NULL)) {
    sc3_error_destroy_noerr (e, flatmsg);
    sc3_logf (log, 0, SC3_LOG_THREAD0, SC3_LOG_ERROR,
              "Leak error: %s", prefix);
    return 0;
  }

  /* fatal error out of an sc3 call, unsafe to continue */
  sc3_error_destroy_noerr (e, flatmsg);
  sc3_logf (log, 0, SC3_LOG_THREAD0, SC3_LOG_ERROR,
            "Fatal error: %s", prefix);
  return 1;
}

static sc3_error_t *
work_init_allocator (sc3_allocator_t ** alloc, int align)
{
  SC3E (sc3_allocator_new (sc3_allocator_nothread (), alloc));
  SC3E (sc3_allocator_set_align (*alloc, align));
  SC3E (sc3_allocator_setup (*alloc));
  return NULL;
}

static sc3_error_t *
work_init_log (sc3_MPI_Comm_t mpicomm,
               sc3_log_t ** log, sc3_allocator_t * alloc, int indent)
{
  SC3E (sc3_log_new (alloc, log));
  SC3E (sc3_log_set_level (*log, SC3_LOG_INFO));
  SC3E (sc3_log_set_comm (*log, mpicomm));
  SC3E (sc3_log_set_indent (*log, indent));
  SC3E (sc3_log_set_function (*log, main_fprintf, 1));
  SC3E (sc3_log_setup (*log));
  return NULL;
}

static sc3_error_t *
work_init (int *pargc, char ***pargv, sc3_MPI_Comm_t mpicomm,
           sc3_allocator_t ** alloc, sc3_log_t ** log)
{
  SC3E (work_init_allocator (alloc, 16));
  SC3E (work_init_log (mpicomm, log, *alloc, 3));
  sc3_log (*log, 0, SC3_LOG_PROCESS0, SC3_LOG_TOP, "Leave work_init");
  return NULL;
}

static sc3_error_t *
work_work (sc3_allocator_t * alloc, sc3_log_t * log)
{
  sc3_log (log, 0, SC3_LOG_PROCESS0, SC3_LOG_TOP, "In work_work");
  sc3_log (log, 0, SC3_LOG_THREAD0, SC3_LOG_TOP, "In work_work");
  return NULL;
}

static sc3_error_t *
work_finalize (sc3_allocator_t ** alloc, sc3_log_t ** log)
{
  sc3_error_t *leak = NULL;

  sc3_log (*log, 0, SC3_LOG_PROCESS0, SC3_LOG_TOP, "Enter work_finalize");

  sc3_log_ref (*log);

  /* if we find any leaks, propagate them to the outside */
  SC3L (&leak, sc3_log_destroy (log));
  SC3L (&leak, sc3_allocator_destroy (alloc));
  return leak;
}

int
main (int argc, char **argv)
{
  int                 scdead = 0;
  sc3_MPI_Comm_t      mpicomm = SC3_MPI_COMM_WORLD;
  sc3_allocator_t    *alloc;
  sc3_log_t          *log;
  sc3_error_t        *e;

  /* Initialize MPI.  This is representative of any external startup code */
  if ((e = sc3_MPI_Init (&argc, &argv)) != NULL) {
    main_exit_failure (&e, "Main init");
  }

  /* Initialization of toplevel allocator and logger.
     We initialize logging and basic allocation here, on errors we exit.
     This is representative of entering sc3 code from any larger program */
  if (!scdead && (e = work_init (&argc, &argv, mpicomm,
                                 &alloc, &log)) != NULL) {
    main_exit_failure (&e, "Work init");
  }

  /* This is representative of calling sc3 code from any larger program */
  if (!scdead && (e = work_work (alloc, log)) != NULL) {
    scdead = work_error (&e, log, "Work work");
  }

  /* Free toplevel allocator and logger.
     This is representative of leaving sc3 code from any larger program */
  if (!scdead && (e = work_finalize (&alloc, &log)) != NULL) {
    scdead = work_error (&e, log, "Work finalize");
  }

  /* Application reporting on fatal sc3 error status */
  if (scdead) {
    fprintf (stderr, "%s", "Main fatal error out of sc3\n");
  }

  /* Finalize MPI.  This is representative of any external cleanup code */
  if ((e = sc3_MPI_Barrier (mpicomm)) != NULL ||
      (e = sc3_MPI_Finalize ()) != NULL) {
    main_exit_failure (&e, "Main finalize");
  }
  return 0;
}
